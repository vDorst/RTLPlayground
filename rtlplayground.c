#include <8051.h>
#include <stdint.h>

// #define REGDBG 1
// #define RXTXDBG 1

#include "rtl837x_sfr.h"
#include "rtl837x_regs.h"
#include "rtl837x_common.h"
#include "rtl837x_flash.h"
#include "rtl837x_phy.h"
#include "rtl837x_port.h"
#include "rtl837x_stp.h"
#include "cmd_parser.h"
#include "uip/uipopt.h"
#include "uip/uip.h"
#include "uip/uip_arp.h"

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

__xdata uint8_t idle_ready;

__code uint8_t ownIP[] = { 192, 168, 2, 2 };
__code struct uip_eth_addr uip_ethaddr = {{ 0x1c, 0x2a, 0xa3, 0x23, 0x00, 0x02 }};
__code uint8_t gatewayIP[] = { 192, 168, 2, 22};
__code uint8_t netmask[] = { 255, 255, 255, 0};

__xdata uint8_t isRTL8373;

volatile __xdata uint32_t ticks;
volatile __xdata uint8_t sec_counter;
volatile __xdata uint16_t sleep_ticks;
__xdata uint8_t stp_clock;

#define STP_TICK_DIVIDER 3


// Buffer for serial input, SBUF_SIZE must be power of 2 < 256
__xdata volatile uint8_t sbuf_ptr;
__xdata uint8_t sbuf[SBUF_SIZE];
__xdata uint8_t sfr_data[4];

extern __xdata uint8_t cmd_buffer[SBUF_SIZE];
extern __xdata uint8_t gpio_last_value[8];

__code uint8_t * __code greeting = "\nA minimal prompt to explore the RTL8372:\n";
__code uint8_t * __code hex = "0123456789abcdef";

__xdata uint8_t flash_buf[256];

// NIC buffers for packet RX/TX
__xdata uint8_t rx_headers[16]; // Packet header(s) on RX
__xdata uint8_t uip_buf[UIP_CONF_BUFFER_SIZE+2];

__xdata uint16_t rx_packet_vlan;
__xdata uint8_t tx_seq;

__xdata uint8_t minPort;
__xdata uint8_t maxPort;
__xdata uint8_t nSFPPorts;
__xdata uint8_t cpuPort;
__xdata uint8_t stpEnabled;

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

/*
 * Transfer Network Interface RX data from the ASIC to the 8051 XMEM
 * data will be stored in the rx_header structure
 * len is the length of data to be transferred
 */
void nic_rx_header(uint16_t ring_ptr)
{
	uint16_t buffer = (uint16_t) &rx_headers[0];
	SFR_NIC_DATA_U16LE = buffer;
	SFR_NIC_RING_U16LE = ring_ptr;
	SFR_NIC_CTRL = 1;
	do { } while (SFR_NIC_CTRL != 0);
}


/*
 * Transfer Network Interface RX data from the ASIC to the 8051 XMEM
 * the description of the packet must be in the rx_headers data structure
 * data will be returned in the xmem buffer points to
 * ring_ptr is the current position of the RX Ring on the ASIC side
 */
void nic_rx_packet(register uint16_t buffer, register uint16_t ring_ptr)
{
	SFR_NIC_DATA_U16LE = buffer;
	SFR_NIC_RING_U16LE = ring_ptr;

	uint16_t len = (((uint16_t)rx_headers[5]) << 8) | rx_headers[4];
	len += 7;
	len >>= 3;
#ifdef RXTXDBG
	print_string(" len: ");
	print_short(len);
#endif
	SFR_NIC_CTRL = len;
	do { } while (SFR_NIC_CTRL != 0);
}


/*
 * Transfers data in XMEM to the ASIC for transmission by the nic
 */
void nic_tx_packet(uint16_t ring_ptr)
{
//	uint16_t buffer = (uint16_t) tx_buf;
	uint16_t buffer = (uint16_t) uip_buf + VLAN_TAG_SIZE;
	SFR_NIC_DATA_U16LE = buffer;
	
	ring_ptr <<= 3;
	ring_ptr |= 0x8000;
	SFR_NIC_RING_U16LE = ring_ptr;
	
	uint16_t len = (((uint16_t)uip_buf[VLAN_TAG_SIZE + 5]) << 8) | uip_buf[VLAN_TAG_SIZE + 4];
	len += 0xf;
	len >>= 3;
	SFR_NIC_CTRL = len;
	do { } while (SFR_NIC_CTRL != 0);
}


/* Read flash using the MMIO capabilities of the DW8051 core
 * Bank is < 0x3f and is the MSB
 * addr gives the address in the bank
 * Note that the address in the flash memory is not simply 0xbbaddr, because
 * the size of a bank is merely 0xc000.
 */
uint8_t read_flash(uint8_t bank, __code uint8_t *addr)
{
	uint8_t v;
	uint8_t current_bank = PSBANK;

	PSBANK = bank;
	v = *addr;
	PSBANK = current_bank;
	return v;
}


void print_long_x(__xdata uint8_t v[])
{
	write_char('0'); write_char('x');
	for (uint8_t i=0; i < 4; i++) {
		write_char(hex[v[i] >> 4]);
		write_char(hex[v[i] & 0xf]);
	}
}

/*
 * Read a SerDes register in the SoC
 * Input must be: sds_id = 0/1, page < 128,  reg <= 0xff
 * The result is in SFR A6 and A7 (SFR_DATA_8, SFR_DATA_0)
 */
void sds_read(uint8_t sds_id, uint8_t page, uint8_t reg)
{
#ifdef REGDBG
	print_string("q"); print_byte(sds_id); print_byte(page); print_byte(reg);
#endif
	SFR_93 = reg;			// 93
	SFR_94 = page << 1 | sds_id;	// 94
	SFR_EXEC_GO = SFR_EXEC_READ_SDS;
	do {
	} while (SFR_EXEC_STATUS != 0);

#ifdef REGDBG
	write_char(':'); print_byte(SFR_DATA_8); print_byte(SFR_DATA_0); write_char(' ');
#endif
}


/*
 * Write a SerDes register in the SoC
 * Input must be: sds_id = 0/1, page < 128,  reg <= 0xff
 * The value written must be in SFR A6 and A7 (SFR_DATA_8, SFR_DATA_0)
 */
void sds_write_v(uint8_t sds_id, uint8_t page, uint8_t reg, uint16_t v)
{
#ifdef REGDBG
	print_string("Q"); print_byte(sds_id); print_byte(page); print_byte(reg);
	write_char(':'); print_byte(v >> 8); print_byte(v); write_char(' ');
#endif
	SFR_DATA_U16 = v;
	SFR_93 = reg;
	SFR_94 = page << 1 | sds_id;
	SFR_EXEC_GO = SFR_EXEC_WRITE_SDS;
	do {
	} while (SFR_EXEC_STATUS != 0);
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


void print_sds_reg(uint8_t sds_id, uint8_t page, uint8_t reg)
{
	sds_read(sds_id, page, reg);
	print_phy_data();
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


void sds_config_mac(uint8_t sds, uint8_t mode)
{
	reg_read_m(RTL837X_REG_SDS_MODES);
	sfr_data[0] = 0;
	sfr_data[1] = 0;
	switch (sds) {
	case 0:
		sfr_mask_data(0, 0x1f, mode);
		break;
	case 1:
		sfr_mask_data(0, 0xe0, mode << 5);
		sfr_mask_data(1, 0x03, mode >> 3);
		break;
	case 2:
		sfr_mask_data(1, 0xfc, 0x02 << 2);
	}
	if (isRTL8373) // Set 3rd SERDES Mode to 0x2 for RTL8224
		sfr_mask_data(1, 0xfc, 0x02 << 2);
	else
		sfr_data[2] &= 0x03;
	reg_write_m(RTL837X_REG_SDS_MODES);
	print_string("\nRTL837X_REG_SDS_MODES: ");
	print_reg(RTL837X_REG_SDS_MODES);
	print_string("\n");
}


// Delay for given number of ticks without doing housekeeping
void delay(uint16_t t)
{
	sleep_ticks = t;
	while (sleep_ticks > 0)
		PCON |= 1;
}


/*
 * Configure the SerDes of the SoC for a particular mode
 * to connect to an SFP module or a PHY
 * Valid modes are SDS_10GR, SDS_QXGMII, SDS_HISGMII, SDS_HSG, SDS_SGMII and SDS_1000BX_FIBER
 * The SerDes ID may be 0 or 1 for RTL8272 and 0-2 for RTL8373
 */
void sds_config(uint8_t sds, uint8_t mode)
{
	print_string("sds_config sds: "); print_byte(sds); print_string(", mode: "); print_byte(mode); write_char('\n');
	sds_config_mac(sds, mode);

	if (mode == SDS_10GR || mode == SDS_QXGMII) // 10G Fiber, 10G connection to RTL8224
		sds_write_v(sds, 0x21, 0x10, 0x4480); // Q002110:6480
	else
		sds_write_v(sds, 0x21, 0x10, 0x6480); // Q002110:6480
	sds_write_v(sds, 0x21, 0x13, 0x0400); // Q002113:0400
	sds_write_v(sds, 0x21, 0x18, 0x6d02); // Q002118:6d02
	sds_write_v(sds, 0x21, 0x1b, 0x424e); // Q00211b:424e
	sds_write_v(sds, 0x21, 0x1d, 0x0002); // Q00211d:0002
	sds_write_v(sds, 0x36, 0x1c, 0x1390); // Q00361c:1390
	sds_write_v(sds, 0x36, 0x14, 0x003f); // Q003614:003f

	uint8_t page = 0;
	uint16_t v = 0;
	print_string("\nTrying to set SDS mode to 0x");
	print_byte(mode);
	print_string("\n");

	switch (mode) {
	case SDS_SGMII:
	case SDS_1000BX_FIBER:
		v = 0x0300;
		page = 0x24;
		break;
	case SDS_HISGMII:
	case SDS_HSG:
		v = 0x0200;
		page = 0x28;
		break;
	case SDS_10GR:
	case SDS_QXGMII:
		v = 0x0200;
		page = 0x2e;
		break;
	default:
		print_string("Error in SDS Mode\n");
		return;
	}
	sds_write_v(sds, 0x36, 0x10, v); // Q003610:0200

	if (page == 0x2e) {  // 10G Fiber
		sds_write_v(sds, page, 0x04, 0x0080); // Q012e04:0080
		sds_write_v(sds, page, 0x06, 0x0408); // Q012e06:0408
		sds_write_v(sds, page, 0x07, 0x020d); // Q012e07:020d
		sds_write_v(sds, page, 0x09, 0x0601); // Q012e09:0601
		sds_write_v(sds, page, 0x0b, 0x222c); // Q012e0b:222c
		sds_write_v(sds, page, 0x0c, 0xa217); // Q012e0c:a217
		sds_write_v(sds, page, 0x0d, 0xfe40); // Q012e0d:fe40
		sds_write_v(sds, page, 0x15, 0xf5c1); // Q012e15:f5c1
	} else {
		sds_write_v(sds, page, 0x04, 0x0080); // Q002804:0080
		sds_write_v(sds, page, 0x07, 0x1201); // Q002807:1201
		sds_write_v(sds, page, 0x09, 0x0601); // Q002809:0601
		sds_write_v(sds, page, 0x0b, 0x232c); // Q00280b:232c
		sds_write_v(sds, page, 0x0c, 0x9217); // Q00280c:9217
		sds_write_v(sds, page, 0x0f, 0x5b50); // Q00280f:5b50
		sds_write_v(sds, page, 0x15, 0xe7c1); // Q002815:e7f1 BUG !
	}

	sds_write_v(sds, page, 0x16, 0x0443); // Q002816:0443 / Q012e16:0443
	sds_write_v(sds, page, 0x1d, 0xabb0); // Q00281d:abb0 / Q012e1d:abb0

	sds_write_v(sds, 0x06, 0x12, 0x5078); // Q000612:5078
	sds_write_v(sds, 0x07, 0x06, 0x9401); // Q000706:9401
	sds_write_v(sds, 0x07, 0x08, 0x9401); // Q000708:9401
	sds_write_v(sds, 0x07, 0x0a, 0x9401); // Q00070a:9401
	sds_write_v(sds, 0x07, 0x0c, 0x9401); // Q00070c:9401
	sds_write_v(sds, 0x1f, 0x0b, 0x0003); // Q001f0b:0003
	sds_write_v(sds, 0x06, 0x03, 0xc45c); // Q000603:c45c
	if (mode != SDS_QXGMII)
		sds_write_v(sds, 0x06, 0x1f, 0x2100); // Q00061f:2100

	if (sds == 0 && mode == SDS_1000BX_FIBER) {
		sds_write_v(sds, 0x02, 0x04, 0x0020); 	// Q000204:0020
		sds_write_v(sds, 0x00, 0x02, 0x73d0); 	// Q000002:73d0
		sds_write_v(sds, 0x00, 0x04, 0x074d); 	// Q000004:074d
		sds_write_v(sds, 0x20, 0x04, 0x0000); 	// Q002000:0000
		sds_write_v(sds, 0x1f, 0x00, 0x0000); 	// Q001f00:0000
	}
}


/*
 * Read a register of the EEPROM via I2C
 */
uint8_t sfp_read_reg(uint8_t slot, uint8_t reg)
{
	if (slot == 0) {
		reg_read_m(RTL837X_REG_I2C_CTRL);
		sfr_mask_data(1, 0xff, 0x72);
		reg_write_m(RTL837X_REG_I2C_CTRL);
	} else {
		reg_read_m(RTL837X_REG_I2C_CTRL);
		sfr_mask_data(1, 0xff, 0x6e);
		reg_write_m(RTL837X_REG_I2C_CTRL);
	}

	REG_WRITE(RTL837X_REG_I2C_IN, 0, 0, 0, reg);

	// Execute I2C Read
	reg_bit_set(RTL837X_REG_I2C_CTRL, 0);

	// Wait for execution to finish
	do {
		reg_read_m(RTL837X_REG_I2C_CTRL);
	} while (sfr_data[3] & 0x1);

	reg_read_m(RTL837X_REG_I2C_OUT);
	return sfr_data[3];
}


/*
 * Adds TX Header to uip_buf and calls nic_tx_packet to send the packet
 * over the wire
 */
void tcpip_output(void)
{
	// Add TX-TAG
	uip_buf[VLAN_TAG_SIZE] = tx_seq++;
	uip_buf[VLAN_TAG_SIZE + 1] = 0x07;    // Enable all checksums
	uip_buf[VLAN_TAG_SIZE + 5] = uip_len >> 8;
	uip_buf[VLAN_TAG_SIZE + 4] = uip_len;
	uip_buf[VLAN_TAG_SIZE + 2] = uip_buf[VLAN_TAG_SIZE + 3] = 0;
	uip_buf[VLAN_TAG_SIZE + 6] = uip_buf[VLAN_TAG_SIZE + 7] = 0;

	reg_read_m(0x7890);
	uint16_t ring_ptr = ((uint16_t)sfr_data[2]) << 8;
	ring_ptr |= sfr_data[3];

	print_string("TX: \n");
	for (uint8_t i = 0; i < 120; i++) {
		print_byte(uip_buf[i]);
		write_char(' ');
	}
	write_char('\n');

	// Move data over from xmem buffer to ASIC side using DMA
	nic_tx_packet(ring_ptr);

	reg_read_m(0x7884);  // actual bytes sent, for now we assume everything worked

	// Do actual TX of data on ASIC side
	sfr_data[0] = sfr_data[1] = sfr_data[2] = 0;
	sfr_data[3] = 0x1;
	reg_write_m(0x7850);
}


void handle_rx(void)
{
	reg_read_m(RTL837X_REG_RX_AVAIL);
	if (sfr_data[2] != 0 || sfr_data[3] != 0) {
		reg_read_m(RTL837X_REG_RX_RINGPTR);
		uint16_t ring_ptr = ((uint16_t)sfr_data[2]) << 8;
		ring_ptr |= sfr_data[3];
		ring_ptr <<= 3;
		nic_rx_header(ring_ptr);
#ifdef RXTXDBG
		__xdata uint8_t *ptr = rx_headers;
		print_string("RX on port "); print_byte(rx_headers[3] & 0xf);
		print_string(": ");
		for (uint8_t i = 0; i < 8; i++) {
			print_byte(*ptr++);
			write_char(' ');
		}
#endif
		nic_rx_packet((uint16_t) &uip_buf[0], ring_ptr + 8);

#ifdef RXTXDBG
		print_string("\n<< ");
		ptr = &uip_buf[0];
		for (uint8_t i = 0; i < 80; i++) {
			print_byte(*ptr++);
			write_char(' ');
		}
#endif
		sfr_data[0] = sfr_data[1] = sfr_data[2] = 0;
		sfr_data[3] = 0x1;
		reg_write_m(RTL837X_REG_RX_DONE);
		uip_len = (((uint16_t)rx_headers[5]) << 8) | rx_headers[4];
//		write_char('>'); print_byte(uip_buf[ETHERTYPE_OFFSET]); write_char('<');
//		write_char('>'); print_byte(uip_buf[ETHERTYPE_OFFSET + 1]); write_char('<');
		// Check for ARP packet
		rx_packet_vlan = uip_buf[12 + RTL_TAG_SIZE + 2] & 0xf;
		rx_packet_vlan <<= 8;
		rx_packet_vlan |= uip_buf[12 + RTL_TAG_SIZE + 3];
#ifdef RXTXDBG
		print_string(" RX-VLAN: "); print_short(rx_packet_vlan); write_char('\n');
#endif
		if (stpEnabled && uip_buf[0] == 0x01 && uip_buf[1] == 0x80 && uip_buf[2] == 0xc2 // STP packet?
			&& uip_buf[3] == 0x00 && uip_buf[4] == 0x00 && uip_buf[5] == 0x00) {
			stp_in();
			if (uip_len) {
				print_string("STP TX\n");
				tcpip_output();
			}
		} else if (uip_buf[ETHERTYPE_OFFSET] == 0x08 && uip_buf[ETHERTYPE_OFFSET + 1] == 0x06) { // ARP?
			uip_arp_arpin();
			if (uip_len) {
			    tcpip_output();
			}
		} else if (uip_buf[ETHERTYPE_OFFSET] == 0x08 && uip_buf[ETHERTYPE_OFFSET + 1] == 0x00) {
			uip_arp_ipin();
			uip_input();
			if (uip_len) {
				// Add ethernet frame
				uip_arp_out();
				tcpip_output();
			}
		}
	}
}


void handle_tx(void)
{
	for(uint8_t i = 0; i < UIP_CONNS; i++) {
		uip_periodic(i);
		if(uip_len > 0) {
 			write_char('.'); print_short(i);
			uip_arp_out();
			tcpip_output();
		}
	}
}


static inline uint8_t sfp_rate_to_sds_config(register uint8_t rate)
{
	if (rate == 0xd)
		return SDS_1000BX_FIBER;
	if (rate == 0x1f)  // Ethernet 2.5 GBit
		return SDS_HSG;
	if (rate > 0x65 && rate < 0x70)
		return SDS_10GR;
	return 0xff;
}


void sfp_print_info(uint8_t sfp)
{
	for (uint8_t i = 20; i < 60; i++) {
		uint8_t c = sfp_read_reg(sfp, i);
		if (c)
			write_char(c);
	}
	print_string("\n");
}


void handle_sfp(void)
{
	reg_read_m(RTL837X_REG_GPIO_00_31_INPUT);
	if ((sfp_pins_last & 0x1) && (!(sfr_data[0] & 0x40))) {
		sfp_pins_last &= ~0x01;
		print_string("\n<MODULE INSERTED>  ");
		// Read Reg 11: Encoding, see SFF-8472 and SFF-8024
		// Read Reg 12: Signalling rate (including overhead) in 100Mbit: 0xd: 1Gbit, 0x67:10Gbit
		delay(100); // Delay, because some modules need time to wake up
		uint8_t rate = sfp_read_reg(0, 12);
		print_string("Rate: "); print_byte(rate);  // Normally 1, but 0 for DAC, can be ignored?
		print_string("  Encoding: "); print_byte(sfp_read_reg(0, 11));
		print_string("\n");
		print_string("\n");
		sfp_print_info(0);
		sds_config(1, sfp_rate_to_sds_config(rate));
	}
	if ((!(sfp_pins_last & 0x1)) && (sfr_data[0] & 0x40)) {
		sfp_pins_last |= 0x01;
		print_string("\n<MODULE REMOVED>\n");
	}

	reg_read_m(RTL837X_REG_GPIO_32_63_INPUT);
	if ((sfp_pins_last & 0x2) && (!(sfr_data[3] & 0x20))) {
		sfp_pins_last &= ~0x02;
		print_string("\n<SFP-RX OK>\n");
	}
	if ((!(sfp_pins_last & 0x2)) && (sfr_data[3] & 0x20)) {
		sfp_pins_last |= 0x02;
		print_string("\n<SFP-RX LOS>\n");
	}

	reg_read_m(RTL837X_REG_GPIO_32_63_INPUT);
	if ((sfp_pins_last & 0x10) && (!(sfr_data[1] & 0x04))) {
		sfp_pins_last &= ~0x10;
		print_string("\n<MODULE 2 INSERTED>  ");
		// Read Reg 11: Encoding, see SFF-8472 and SFF-8024
		// Read Reg 12: Signalling rate (including overhead) in 100Mbit: 0xd: 1Gbit, 0x67:10Gbit
		delay(100); // Delay, because some modules need time to wake up
		uint8_t rate = sfp_read_reg(1, 12);
		print_string("Rate: "); print_byte(rate);  // Normally 1, but 0 for DAC, can be ignored?
		print_string("  Encoding: "); print_byte(sfp_read_reg(1, 11));
		print_string("\n");
		sfp_print_info(1);
		sds_config(0, sfp_rate_to_sds_config(rate));
	}
	if ((!(sfp_pins_last & 0x10)) && (sfr_data[1] & 0x04)) {
		sfp_pins_last |= 0x10;
		print_string("\n<MODULE 2 REMOVED>\n");
	}
}


//
// An idle function that sleeps for 1 tick and does all the house-keeping
//
void idle(void)
{
	PCON |= 1;
	if (sec_counter >= 60) {
		sec_counter -= 60;
		reg_read_m(RTL837X_REG_SEC_COUNTER);
		uint8_t v = sfr_data[3];
#ifdef DEBUG
		print_string("  sec_counter: "); print_byte(v);
#endif
		v++;
		sfr_data[3] = v;
		if (!v) {
			v = sfr_data[2];
			v++;
			sfr_data[2] = v;
			if (!v) {
				v = sfr_data[1];
				v++;
				sfr_data[1] = v;
				if (!v) {
					v = sfr_data[0];
					v++;
					sfr_data[0] = v;
				}
			}
		}
		reg_write_m(RTL837X_REG_SEC_COUNTER);
		reg_read_m(RTL837X_REG_SEC_COUNTER);
#ifdef DEBUG
		print_string(" >>: ");
		print_long_x(sfr_data);
#endif
	}

	reg_read_m(RTL837X_REG_LINKS);
	if (!isRTL8373 && cmp_4(sfr_data, linkbits_last)) {
		print_string("\n<new link: ");
		print_long_x(sfr_data);
		print_string(", was ");
		print_long_x(linkbits_last);
		print_string(">\n");
		if (nSFPPorts != 2) {
			uint8_t p5 = sfr_data[2] >> 4;
			uint8_t p5_last = linkbits_last[2] >> 4;
			cpy_4(linkbits_last, sfr_data);
			if (p5_last != p5) {
				if (p5 == 0x5) // 2.5GBit Mode
					sds_config(0, SDS_HISGMII);
				else if (p5 == 0x2) // 1GBit
					sds_config(0, SDS_SGMII);
			}
		} else {
			cpy_4(linkbits_last, sfr_data);
		}
	}

	// Check for changes with SFP modules

	handle_sfp();

	/* Button pressed on KL-8xhm-x2:
	reg_read(RTL837X_REG_GPIO_32_63_INPUT);
	if (!(sfr_data[2] & 0x40))
		print_string("Button pressed\n");
	*/
	// Check new Packets RX
	handle_rx();
	// Check UIP for packets to transmit
	handle_tx();
	// If STP protocol enabled, decrease STP timers to trigger actions
	if (stpEnabled) {
		if (!stp_clock) {
			stp_clock = STP_TICK_DIVIDER;
			stp_timers();
		} else {
			stp_clock--;
		}
	}
}


// Sleep the given number of ticks and perform idle tasks if initialized
void sleep(uint16_t t)
{
	sleep_ticks = t;
	while (sleep_ticks > 0) {
		if (idle_ready)
			idle();
		else
			PCON |= 1;
	}
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

/*
 * Modify a register reg of phy phy_id, in page page
 * Set: bit mask of bits to set.
 * Mask: bit mask of bits to clear.

 * Note: We assume that the registers `SFR_SMI_REG_U16`, `SFR_SMI_PHY` and `SFR_SMI_DEV` 
 * keep there value, and dont have to be rewritten everytime.
 */
void phy_modify(uint8_t phy_id, uint8_t dev_id, uint16_t reg, uint16_t mask, uint16_t set)
{
	uint8_t smi_phy = dev_id << 3 | 2;

	// Read the data
	SFR_SMI_REG_U16 = reg;		// c2, c2
	SFR_SMI_PHY = phy_id;		// a5
	SFR_SMI_DEV = smi_phy;		// c4
	SFR_EXEC_GO = SFR_EXEC_READ_SMI;
	do {
	} while (SFR_EXEC_STATUS != 0);

	// Modify the reed data.
	// TODO: Check if we directly can modify SFR register directly.
	uint16_t data = SFR_DATA_U16 & ~(mask);
	data |= ~(set);

	uint16_t phy_mask = bit_mask[phy_id];

	// Write it back
	SFR_SMI_REG_U16 = data;
	SFR_SMI_PHYMASK = phy_mask;		// SFR_C5
	SFR_SMI_DEV = smi_phy | (phy_mask >> 8);
	SFR_EXEC_GO = SFR_EXEC_WRITE_SMI;
	do {
	} while (SFR_EXEC_STATUS != 0);
}

void nic_setup(void)
{
	// Enable NIC
	// r6040:00000100 R6040-00001100
	reg_bit_set(RTL837X_REG_HW_CONF, 0xc);

	// This sets the size of the RX buffer, the filling level is in 0x7874
	// R7848-000004ff
	REG_SET(0x7848, 0x4ff);

	// R7844-000007fe
	REG_SET(0x7844, 0x7fe);

	// Configure NIC RX to receive various types of packets
	// RTL837X_REG_RX_CTRL: Set bits 24-31 to 0x4, clear bits 16/17
	reg_read_m(RTL837X_REG_RX_CTRL);
	sfr_mask_data(3, 0xff, 0x04);
	sfr_mask_data(2, 0x03, 0);
	reg_write_m(RTL837X_REG_RX_CTRL);

	// Enable NIC TX (set bit 0)
	reg_bit_set(RTL837X_REG_TX_CTRL, 0);

	// Enable NIC RX (set bit 0)
	reg_bit_set(RTL837X_REG_RX_CTRL, 0);

	// Drop packets with invalid CRC
	reg_bit_clear(RTL837X_REG_RX_CTRL, 2);

	// R603c-00000200
	REG_SET(0x603c, 0x200);

	// r6720:00000500 R6720-00000501 R6720-00000501 r6720:00000501
	reg_read_m(0x6720);
	sfr_mask_data(0, 1, 1);
	sfr_mask_data(1, 3, 0);
	reg_write_m(0x6720);

	// r6368:00000194 R6368-00000197
	reg_read_m(0x6368);
	sfr_mask_data(0, 0, 3);
	reg_write_m(0x6368);

	// Sequence number of TX packets
	tx_seq = 0;
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

				write_char(' ');
				print_byte( gpio_last_value[(idx *4)] ^ SFR_DATA_24);
				gpio_last_value[(idx *4)] = SFR_DATA_24;
				print_byte( gpio_last_value[(idx *4) + 1] ^ SFR_DATA_16);
				gpio_last_value[(idx *4) + 1] = SFR_DATA_16;
				print_byte( gpio_last_value[(idx *4) + 2] ^ SFR_DATA_8);
				gpio_last_value[(idx *4) + 2] = SFR_DATA_8;
				print_byte( gpio_last_value[(idx *4) + 3] ^ SFR_DATA_0);
				gpio_last_value[(idx *4) + 3] = SFR_DATA_0;
				write_char('\n');
			}
		}
	}
}
