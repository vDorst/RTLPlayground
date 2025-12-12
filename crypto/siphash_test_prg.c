#include <8051.h>
#include <stdint.h>

// #define REGDBG 1
// #define RXTXDBG 1

#include "../rtl837x_sfr.h"
#include "../rtl837x_regs.h"
#include "siphash_test_prg.h"
#include "../rtl837x_flash.h"
#include "../rtl837x_phy.h"
#include "../rtl837x_port.h"
#include "../rtl837x_stp.h"
#include "siphash.h"


#define SYS_TICK_HZ 100
#define SERIAL_BAUD_RATE 115200

/* All RTL839x switches have an external 25MHz Oscillator,
   VALID RTL8372/3 CPU frequencies found in switches are:
   0x07735940 = 125,000,000
   0x03b9aca0 =  62,500,000
   0x01dcd650 =  31,250,000
   0x013d6200 =  20,800,000
   For the following frequencies, divider settings are known
   and can be selected on all known HW (Register 0x6040)
*/
#define CLOCK_HZ 125000000
//#define CLOCK_HZ 20800000

// Derive the divider settings for the internal clock
#if CLOCK_HZ == 20800000
#define CLOCK_DIV 3
#elif CLOCK_HZ == 31250000
#define CLOCK_DIV 2
#elif CLOCK_HZ == 62500000
#define CLOCK_DIV 1
#elif CLOCK_HZ == 125000000
#define CLOCK_DIV 0
#endif


volatile __xdata uint32_t ticks;
volatile __xdata uint8_t sec_counter;
volatile __xdata uint16_t sleep_ticks;
__xdata uint8_t stp_clock;

#define STP_TICK_DIVIDER 3

#define SBUF_SIZE 32

// Buffer for serial input, SBUF_SIZE must be power of 2 < 256
__xdata volatile uint8_t sbuf_ptr;
__xdata uint8_t sbuf[SBUF_SIZE];
__xdata uint8_t sfr_data[4];

extern __xdata uint8_t cmd_buffer[SBUF_SIZE];
extern __xdata uint8_t gpio_last_value[8];

__code uint8_t * __code greeting = "\nA minimal prompt to explore the RTL8372:\n";
__code uint8_t * __code hex = "0123456789abcdef";

__xdata uint8_t flash_buf[256];

__code uint16_t bit_mask[16] = {
	0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
	0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000
};


__xdata uint8_t was_offline;
__xdata uint8_t linkbits_last[4];
__xdata uint8_t sfp_pins_last;


#define ETHERTYPE_OFFSET (12 + VLAN_TAG_SIZE + RTL_TAG_SIZE)

void isr_timer0(void) __interrupt(1)
{
	TR0 = 0;		// Stop timer 0
	TH0 = (0x10000 - (CLOCK_HZ / SYS_TICK_HZ / 32)) >> 8;
	TL0 = (0x10000 - (CLOCK_HZ / SYS_TICK_HZ / 32)) % 0xff;

	ticks++;
	if (sleep_ticks > 0)
		sleep_ticks--;
	sec_counter++;

	TR0 = 1;		// Re-start timer 0
}


void isr_serial(void) __interrupt(4)
{
	if (RI == 1) {
		sbuf[sbuf_ptr] = SBUF;
		sbuf_ptr = (sbuf_ptr + 1) & (SBUF_SIZE - 1);
		RI = 0;
	}
}


void write_char(char c)
{
	do {
	} while (TI == 0);
	TI = 0;
	if (c =='\n') {
		SBUF = '\r';
		do {
		} while (TI == 0);
		TI = 0;
	}
	SBUF = c;
}


void print_string(__code char *p)
{
	while (*p)
		write_char(*p++);
}

void print_string_x(__xdata char *p)
{
	while (*p)
		write_char(*p++);
}


void memcpy(__xdata void * __xdata dst, __xdata const void * __xdata src, uint16_t len)
{
	__xdata uint8_t *d = dst;
	__xdata const uint8_t *s = src;
	while (len--)
		*d++ = *s++;
}

void memcpyc(register __xdata uint8_t *dst, register __code uint8_t *src, register uint16_t len)
{
	while (len--)
		*dst++ = *src++;
}


void memset(register __xdata uint8_t *dst, register __xdata uint8_t v, register uint8_t len)
{
	while (len--)
		*dst++ = v;
}

uint16_t strtox(register __xdata uint8_t *dst, register __code const char *s)
{
	__xdata uint8_t *b = dst;
	while (*s)
		*dst++ = *s++;
	*dst = 0;
	return dst - b;
}

uint16_t strlen(register __code const char *s)
{
	uint16_t l = 0;
	while (s[l])
		l++;
	return l;
}


uint16_t strlen_x(register __xdata const char *s)
{
	uint16_t l = 0;
	while (s[l])
		l++;
	write_char(';');
	print_short(l);
	write_char(';');
	return l;
}


void print_short(uint16_t a)
{
	print_string("0x");
	for (signed char i = 12; i >= 0; i -= 4) {
		write_char(hex[(a >> i) & 0xf]);
	}
}


void print_long(__xdata uint32_t a)
{
	print_string("0x");
	for (signed char i = 28; i >= 0; i -= 4) {
		write_char(hex[(a >> i) & 0xf]);
	}
}

void print_byte(uint8_t a)
{
	write_char(hex[(a >> 4) & 0xf]);
	write_char(hex[a & 0xf]);
}


/*
 * External IRQ 0 Service Routine: Called on link change?
 * Note that all registers are being put on the STACK because of calling a subroutine
 */
void isr_ext0(void) __interrupt(0)
{
	EX0 = 0;	// Disable interrupt for the moment
	write_char('X');
	IT0 = 1;	// Trigger on falling edge of external interrupt
	EX0 = 1;	// Re-enable interrupt
}


/*
 * External IRQ 1 Service Routine, triggered by the NIC recieving a packet
 * Note that all registers are being put on the STACK because of calling
 * a subroutine (write_char), we shold do better...
 */
void isr_ext1(void) __interrupt(2)
{
	// This flag should only be reset after all packets have been read
	EX1 = 0;
	write_char('Y');
	EX1 = 1;
}

/*
 * External IRQ 2 Service Routine
 * Note that all registers are being put on the STACK because of calling a subroutine
 */
void isr_ext2(void) __interrupt(8)
{
	EXIF &= 0xef;	// Clear IRQ flag (bit 7) in EXIF
	write_char('Z');
	PCON |= 1; // Enter Idle mode until interrupt occurs
}

/*
 * External IRQ 3 Service Routine
 * Note that all registers are being put on the STACK because of calling a subroutine
 */
void isr_ext3(void) __interrupt(9)
{
	EXIF &= 0xdf;	// Clear IRQ flag (bit 6) in EXIF
	write_char('W');
}


void setup_timer0(void)
{
	TMOD = 0x11;  // Timer 1: Mode 1, Timer 0: Mode 1, i.e. 16 bit counters, no auto-reload
	// The TH0 registers contain the high/low byte that is loaded into
	// timer0 when T0 overflows to 0x10000
	TH0 = (0x10000 - (CLOCK_HZ / SYS_TICK_HZ / 32)) >> 8;
	TL0 = (0x10000 - (CLOCK_HZ / SYS_TICK_HZ / 32)) % 0xff;

	TCON = 0x10;	// Start timer 0
	CKCON &= 0xc7;
	ET0 = 1;	// Enable timer interrupts
}


void reg_read(uint16_t reg_addr)
{
	SFR_REG_ADDR_U16 = reg_addr;
	SFR_EXEC_GO = SFR_EXEC_READ_REG;
	do {
	} while (SFR_EXEC_STATUS != 0);
	/* The result is now in SFR A4, A5, A6, A7 */
}


void reg_read_m(uint16_t reg_addr)
{
#ifdef REGDBG
	if (EA) { write_char('r'); print_byte(reg_addr >> 8); print_byte(reg_addr); write_char(':'); }
#endif
	SFR_REG_ADDR_U16 = reg_addr;
	SFR_EXEC_GO = SFR_EXEC_READ_REG;
	do {
	} while (SFR_EXEC_STATUS != 0);
	sfr_data[0] = SFR_DATA_24;
	sfr_data[1] = SFR_DATA_16;
	sfr_data[2] = SFR_DATA_8;
	sfr_data[3] = SFR_DATA_0;
#ifdef REGDBG
	if (EA) { print_byte(sfr_data[0]);  print_byte(sfr_data[1]);  print_byte(sfr_data[2]);  print_byte(sfr_data[3]); write_char(' '); }
#endif
}


void reg_write(uint16_t reg_addr)
{
	/* Data to write must be in SFR A4, A5, A6, A7 */
	SFR_REG_ADDR_U16 = reg_addr;
	SFR_EXEC_GO = SFR_EXEC_WRITE_REG;
	do {
	} while (SFR_EXEC_STATUS != 0);
}


void reg_write_m(uint16_t reg_addr)
{
#ifdef REGDBG
	if (EA) {
		write_char('R'); print_byte(reg_addr >> 8); print_byte(reg_addr); write_char('-');
		print_byte(sfr_data[0]);  print_byte(sfr_data[1]);  print_byte(sfr_data[2]);  print_byte(sfr_data[3]); write_char(' ');
	}
#endif
	SFR_REG_ADDR_U16 = reg_addr;
	SFR_DATA_24 = sfr_data[0] ;
	SFR_DATA_16 = sfr_data[1];
	SFR_DATA_8 = sfr_data[2];
	SFR_DATA_0 = sfr_data[3];

	SFR_EXEC_GO = SFR_EXEC_WRITE_REG;
	do {
	} while (SFR_EXEC_STATUS != 0);
}


/*
 * This sets a bit in the 32bit wide switch register reg_addr
 */
void reg_bit_set(uint16_t reg_addr, char bit)
{
	uint8_t bit_mask = 1 << (bit & 0x7);

	bit >>= 3;
	reg_read_m(reg_addr);
	sfr_data[3-bit] |= bit_mask;
	reg_write_m(reg_addr);
}


/*
 * This sets a bit in the 32bit wide switch register reg_addr
 */
void reg_bit_clear(uint16_t reg_addr, char bit)
{
	uint8_t bit_mask = 1 << (bit & 0x7);

	bit >>= 3;
	reg_read_m(reg_addr);
	bit_mask = ~bit_mask;
	sfr_data[3-bit] &= bit_mask;
	reg_write_m(reg_addr);
}

/*
 * This masks the sfr data fields, first &-ing with ~mask, then setting the bits in set
 */
void sfr_mask_data(uint8_t n, uint8_t mask, uint8_t set)
{
	uint8_t b = sfr_data[3-n];
	b &= ~mask;
	b |= set;
	sfr_data[3-n] = b;
}

/*
 * This zeros all the sfr data fields
 */
void sfr_set_zero(void) {
	uint8_t idx = 4;
	while (idx) {
		idx -= 1;
		sfr_data[idx] = 0;
	}
}


void print_long_x(__xdata uint8_t v[])
{
	write_char('0'); write_char('x');
	for (uint8_t i=0; i < 4; i++) {
		write_char(hex[v[i] >> 4]);
		write_char(hex[v[i] & 0xf]);
	}
}


void print_sfr_data(void)
{
	write_char('0');
	write_char('x');
	write_char(hex[sfr_data[0] >> 4]);
	write_char(hex[sfr_data[0] & 0xf]);
	write_char(hex[sfr_data[1] >> 4]);
	write_char(hex[sfr_data[1] & 0xf]);
	write_char(hex[sfr_data[2] >> 4]);
	write_char(hex[sfr_data[2] & 0xf]);
	write_char(hex[sfr_data[3] >> 4]);
	write_char(hex[sfr_data[3] & 0xf]);
}


void print_phy_data(void)
{
	write_char('0');
	write_char('x');
	write_char(hex[SFR_DATA_8 >> 4]);
	write_char(hex[SFR_DATA_8 & 0xf]);
	write_char(hex[SFR_DATA_0 >> 4]);
	write_char(hex[SFR_DATA_0 & 0xf]);
}


void print_reg(uint16_t reg)
{
	reg_read_m(reg);
	print_sfr_data();
}

char cmp_4(__xdata uint8_t a[], __xdata uint8_t b[])
{
	for (uint8_t i = 0; i < 4; i++) {
		if (a[i] == b[i])
			continue;
		if (a[i] < b[i])
			return -1;
		else
			return 1;
	}
	return 0;
}

void cpy_4(__xdata uint8_t dest[], __xdata uint8_t source[])
{
	for (uint8_t i = 0; i < 4; i++)
		dest[i] = source[i];
}



// Delay for given number of ticks without doing housekeeping
void delay(uint16_t t)
{
	sleep_ticks = t;
	while (sleep_ticks > 0)
		PCON |= 1;
}




void reset_chip(void)
{
	REG_SET(RTL837X_REG_RESET, 1);
}

/*
 * Set dividers for a chosen CPU frequency
 */
void setup_clock(void)
{
	reg_read_m(RTL837X_REG_HW_CONF);
	sfr_mask_data(0, 0x30, 0);
#if CLOCK_DIV != 0
	 // Divider in bits 4 & 5
	sfr_mask_data(0, 0, CLOCK_DIV << 4);
#endif
	// Bit 8 is set in managed mode 125MHz to use fast SPI mode
	sfr_mask_data(1, 0, 0x01);
	reg_write_m(RTL837X_REG_HW_CONF);

	// Enable serial interface, set bit 0
	reg_read_m(RTL837X_PIN_MUX_1);
	sfr_mask_data(0, 0x1, 0x1);
	reg_write_m(RTL837X_PIN_MUX_1);
}


/*
 * Write a register reg of multipule phys, using a mask to select them, in page page
 * Data to be written is in v
 */
void phy_write_mask(uint16_t phy_mask, uint8_t dev_id, uint16_t reg, uint16_t v)
{
#ifdef REGDBG
	print_string("P"); print_byte(phy_mask>>8); print_byte(phy_mask); print_byte(dev_id); write_char('.'); print_byte(reg>>8); print_byte(reg); write_char(':');
	print_byte(v>>8); print_byte(v); write_char(' ');
#endif
	SFR_DATA_U16 = v;			    // SFR_A6, SFR_A7
	SFR_SMI_PHYMASK = phy_mask;		// SFR_C5
	SFR_SMI_REG_U16 = reg;			// SFR_C2, SFR_C3
	SFR_SMI_DEV = (phy_mask >> 8) | dev_id  << 3 | 2; // SFR_C4: bit 2 can also be set for some option
	SFR_EXEC_GO = SFR_EXEC_WRITE_SMI;
	do {
	} while (SFR_EXEC_STATUS != 0);
}

/*
 * Write a register reg of phy, using a mask to select them, in page page
 * Data to be written is in v
 */
void phy_write(uint8_t phy_id, uint8_t dev_id, uint16_t reg, uint16_t v)
{
	uint16_t phy_mask =  bit_mask[phy_id];
#ifdef REGDBG
	print_string("P"); print_byte(phy_mask>>8); print_byte(phy_mask); print_byte(dev_id); write_char('.'); print_byte(reg>>8); print_byte(reg); write_char(':');
	print_byte(v>>8); print_byte(v); write_char(' ');
#endif
	SFR_DATA_U16 = v;			    // SFR_A6, SFR_A7
	SFR_SMI_PHYMASK = phy_mask;		// SFR_C5
	SFR_SMI_REG_U16 = reg;			// SFR_C2, SFR_C3
	SFR_SMI_DEV = (phy_mask >> 8) | dev_id  << 3 | 2; // SFR_C4: bit 2 can also be set for some option
	SFR_EXEC_GO = SFR_EXEC_WRITE_SMI;
	do {
	} while (SFR_EXEC_STATUS != 0);
}


/*
 * Read a phy register via MDIO clause 45
 * Input must be: phy_id < 64,  device_id < 32,  reg < 0x10000)
 * The result is in SFR A6 and A7 (SFR_DATA_8, SFR_DATA_0)
 */
void phy_read(uint8_t phy_id, uint8_t dev_id, uint16_t reg)
{
#ifdef REGDBG
	print_string("p"); print_byte(phy_id); print_byte(dev_id); write_char('.'); print_byte(reg>>8); print_byte(reg); write_char(':');
#endif
	SFR_SMI_REG_U16 = reg;		// c2, c2

	SFR_SMI_PHY = phy_id;		// a5
	SFR_SMI_DEV = dev_id << 3 | 2;	// c4

	SFR_EXEC_GO = SFR_EXEC_READ_SMI;
	do {
	} while (SFR_EXEC_STATUS != 0);
#ifdef REGDBG
	print_byte(SFR_DATA_8); print_byte(SFR_DATA_0); write_char(' ');
#endif
}

/* Set up serial port 0 using Timer 2 with an external trigger
 * as baud generator.
 * The external clock generator uses a crystal at 25MHz.
 */
void setup_serial(void)
{
	IE = 0;

	T2CON = 0x34; // Enable RCLK/TCLK (serial transmit/receive clock for T2), TR2 (Timer 2 RUN), disable CP/RL2 (bit 0)
	SCON = 0x50;  // Mode = 1: ASYNC 8N1 with T2 as baud-rate generator, REN_0 Receive enable

	// The RCAP2 registers contain the high/low byte that is loaded into
	// timer2 when T2 overflows to 0x10000
	RCAP2H = (0x10000 - (CLOCK_HZ / SERIAL_BAUD_RATE / 32)) >> 8;
	RCAP2L = (0x10000 - (CLOCK_HZ / SERIAL_BAUD_RATE / 32)) % 0xff;

	PCON |= 0x80; // Double the Baud Rate

	SCON = 0x50;
	TI = 1;
	RI = 0;

	ES = 1; // Enable serial IRQ
}


void bootloader(void)
{
    __xdata uint8_t key_mac_addr[16] = "MacAddr1MacAddr2";
    siphash_init(&key_mac_addr);


	ticks = 0;
	stp_clock = STP_TICK_DIVIDER;
	sbuf_ptr = 0;

	CKCON = 0;	// Initial Clock configuration
	SFR_97 = 0;	// HADDR?

	// Set in managed mode:
	SFR_b9 = 0x00;
	SFR_ba = 0x80;

	// Disable all interrupts (global and individually) by setting IE register (SFR A8) to 0
	IE = 0;
	EIE = 0;  // SFR e8: EIE. Disable all external IRQs

	// Disable all interrupts (global interrupt enable bit)
	EA = 0; // SFR A8.7 / IE.7

   // HW setup, serial, timer, external IRQs
	setup_clock();
	setup_serial();
	setup_timer0();

	EA = 1; // Enable all IRQs

	print_string("RST\n");

	// Wait for commands on serial connection
	// sbuf_ptr is moved forward by serial interrupt, l is the position until we have already
	// printed out the entered characters
	__xdata uint8_t l = sbuf_ptr; // We have printed out entered characters until l
	__xdata uint8_t line_start = sbuf_ptr; // This is where the current line starts
	while (1) {
		PCON |= 1;
		if (sec_counter >= 60) {
			sec_counter -= 60;
			for (uint8_t idx = 0; idx < 2; idx++) {
				reg_read(RTL837X_REG_GPIO_00_31_INPUT + (idx * 4));
				print_string("GPIO ");
				write_char(idx + '0');
				write_char(':');
				write_char(' ');

				print_byte(SFR_DATA_24);
				print_byte(SFR_DATA_16);
				print_byte(SFR_DATA_8);
				print_byte(SFR_DATA_0);
                print_string("Hoi\n");
            }
		}
	}
}
