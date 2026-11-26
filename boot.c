#pragma codeseg BANK3
#pragma constseg BANK3

#include <8051.h>
#include <stdint.h>
#include <stdbool.h>

#include "rtl837x_sfr.h"
#include "rtl837x_regs.h"
#include "rtl837x_common.h"
#include "rtl837x_flash.h"
#include "rtl837x_pins.h"
#include "rtl837x_phy.h"
#include "rtl837x_port.h"
#include "rtl837x_init.h"
#include "machine.h"
#include "phy.h"
#include "boot.h"
#include "timer.h"

extern __code const struct machine machine;
extern __xdata struct machine_runtime machine_detected;
extern __xdata uint8_t sfr_data[4];
#define RTL8224_RELEASE_TICKS (SYS_TICK_HZ * 5 / 2)
#define BOOT_HOLD_MIN_TICKS (10 * SYS_TICK_HZ)
#define BOOT_HOLD_MAX_TICKS (30 * SYS_TICK_HZ)
#define BOOT_BLINK_TICKS (SYS_TICK_HZ / 10)
#define BOOT_PAUSE_TICKS (SYS_TICK_HZ / 2)
__xdata struct timer rtl8224_release;
extern __xdata uint8_t tx_seq;
extern __code const uint8_t * __code const hex;

void early_boot_handle_button(void) __banked
{
	if (machine.reset_pin == GPIO_NA)
		return;

	gpio_input_setup(machine.reset_pin);
	gpio_set_pull_mode(machine.reset_pin, GPIO_PULL_UP);

	// Debounce after init
	delay(100);
	// If the button is not already held at boot, continue normally.
	if (gpio_pin_test(machine.reset_pin))
		return;

	set_sys_led_state(SYS_LED_FAST);
	print_string("\n[Reset button held at boot]\n");

	if (gpio_pin_test(machine.reset_pin))
		return;

	__xdata struct timer hold_max;
	__xdata struct timer hold_min;
	__xdata struct timer blink;
	__xdata uint8_t blink_step = 0;

	timer_set(&hold_max, BOOT_HOLD_MAX_TICKS);
	timer_set(&hold_min, BOOT_HOLD_MIN_TICKS);
	timer_set(&blink, BOOT_BLINK_TICKS);
	set_sys_led_state(SYS_LED_ON);

	while (!gpio_pin_test(machine.reset_pin)) {
		if (timer_expired(&hold_max)) {
			print_string("[Button held >30s at boot; continuing normal boot]\n");
			return;
		}

		// Double blink pattern while button is held:
		// ON (100ms), OFF (100ms), ON (100ms), OFF (500ms)
		if (timer_expired(&blink)) {
			blink_step = (blink_step + 1) & 0x3;
			set_sys_led_state((blink_step == 0 || blink_step == 2) ? SYS_LED_ON : SYS_LED_OFF);
			timer_set(&blink, blink_step == 3 ? BOOT_PAUSE_TICKS : BOOT_BLINK_TICKS);
		}

		PCON |= 1;
	}

	set_sys_led_state(SYS_LED_ON);

	if (timer_expired(&hold_min)) {
		print_string("[Button held 10s-30s at boot; restoring default config]\n");
		set_sys_led_state(SYS_LED_FAST);
		flash_default_config();
		delay(3UL * SYS_TICK_HZ);
	}
}

void rtl8224_enable(void) __banked
{
	// Set Pin 4 low
	reg_bit_clear(RTL837X_REG_GPIO_32_63_OUTPUT, 4);
	// Configure Pin as output
	reg_bit_set(RTL837X_REG_GPIO_32_63_DIRECTION, 4);
	delay(4);
	// Set pin 4 high
	reg_bit_set(RTL837X_REG_GPIO_32_63_OUTPUT, 4);
	timer_set(&rtl8224_release, RTL8224_RELEASE_TICKS);
}



void nic_setup(void) __banked
{
	// Enable NIC
	// r6040:00000100 R6040-00001100
	reg_bit_set(RTL837X_REG_HW_CONF, 0xc);

	// This sets the size of the RX buffer, the filling level is in 0x7874
	// R7848-000004ff
	REG_SET(RTL837X_REG_NIC_RXBUFF_RX, RTL837X_NIC_RX_LAST_SLOT);

	// R7844-000007fe
	REG_SET(RTL837X_REG_NIC_BUFFSIZE_TX, 0x7fe);

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
	// CPU-port is CPU-Tag aware (bit 9)
	REG_SET(RTL837X_REG_CPU_TAG_AWARE_PMASK, 0x200);

	// Insert CPU-tag for internally received packets (bit 0), MODE is 0, i.e. ALL packets (bits 8-9)
	reg_read_m(RTL837X_REG_CPU_TAG);
	sfr_mask_data(0, 1, 1);
	sfr_mask_data(1, 3, 0);
	reg_write_m(RTL837X_REG_CPU_TAG);

	// Force MAC mode of the CPU port (port 9)
	// r6368:00000194 R6368-00000197
	reg_read_m(RTL837X_REG_MAC_FORCE_MODE + 9 * 4);
	sfr_mask_data(0, 0, 3); // Set bits 0, 1: Force link
	reg_write_m(RTL837X_REG_MAC_FORCE_MODE+ 9 * 4);

	// Sequence number of TX packets
	tx_seq = 0;
	REG_SET(RTL837X_NIC_INT_MSK, NIC_INT_RXIE);
}



void rtl8373_revision(void) __banked
{
	reg_read_m(RTL837X_REG_CHIP_INFO);
	sfr_mask_data(2, 0x0a, 0x0a); 	// Enable reading version
	reg_write_m(RTL837X_REG_CHIP_INFO);
	delay(50);

	reg_read_m(RTL837X_REG_CHIP_INFO);
	print_string("CPU revision: "); print_byte(sfr_data[2]); print_byte(sfr_data[2]); write_char('\n');
	sfr_mask_data(2, 0x0a, 0x00); 	// Enable reading version
	reg_write_m(RTL837X_REG_CHIP_INFO);
}


/*
 * The SoC manages Link-State for steering the LEDs and can set PHY-settings
 * automatically through Realtek's SMI (Simple Managagement) Interface, a
 * proprietary version of MDIO which for example allows for more PHYs on the same
 * bus.
 * Configure polling via SMI and the interface setup during boot.
 */
void init_smi(void) __banked
{
	print_string("\ninit_switch called\n");

	/* Set the SMI(i.e.I2C) type for PHY polling, 0b01 is 2.5/10G PHY. Disable (0b00) for the SFP-ports
	 * which are at port 8 and additionally at port 3 for a dual SFP device
	 */

	// Default: 0x00005555
	// Workaround for SDCC BUG 4070: SFR_DATA_U32 = 0x00005555;
	SFR_DATA_U16_UPPER = 0x0000;
	// Default value for every internal PHY port is 0b01.
	SFR_DATA_U16 = 0x5555;
	for (uint8_t sds = 0; sds < 2; sds++) {
		enum sds_type usage = machine.sds_settings[sds].usage;
		if (sds == 0) {
			if (usage == SDS_EPHY) {
				// Set bit 6,7 to 0b01
				SFR_DATA_0 = 0x55;
			} else if (usage == SDS_SFP) {
				// Set bit 6,7 to 0b00
				SFR_DATA_0 = 0x15;

			}
		} else { // sds == 1
			if (usage == SDS_EPHY) {
				// Set bit 16,17 to 0b01
				SFR_DATA_16 = 0x01;
			}
		}
	}
	reg_write(RTL837X_REG_SMI_MAC_TYPE);

	// Configure polling of all PHYs by the MAC to detect link-state changes
	// Default: 0x000000ff
	// Workaround for SDCC BUG 4070: SFR_DATA_U32 = 0x000000ff;
	SFR_DATA_U16_UPPER = 0x0000;
	SFR_DATA_U16 = 0x00ff;
	if (machine_detected.isRTL8373) {
		// poll all the first 8 internal phy's (mac 0-7).
		SFR_DATA_0 = 0xff;
	} else {
		if (machine.sds_settings[0].usage == SDS_EPHY)
			// Poll port 1-4 (mac 4-7) and phy on SDS0 (mac3)
			SFR_DATA_0 = 0xf8;
		else
			// Poll only port 1-4 (mac 4-7)
			SFR_DATA_0 = 0xf0;
	}
	if (machine.sds_settings[1].usage == SDS_EPHY)
		// Set bit 9
		SFR_DATA_8 = 0x01;

	reg_write(RTL837X_REG_SMI_PORT_POLLING);
	// Enable MDC
	reg_read_m(RTL837X_REG_SMI_CTRL);
	sfr_mask_data(1, 0, 0x70); 	// Set bits 12-14 to enable MDC for SMI0-SMI2
	reg_write_m(RTL837X_REG_SMI_CTRL);
	delay(50);

	// Program the PHY addresses associated with MAC/logical port number 0-5.
	// To access the PHY, with phy_{read,write} we are using logical port number not the real PHY-ID.
	if (machine_detected.isRTL8373) {
		// RTL8373, we assume that all the PHY_ADDR are start from 0 are counting up to 5.
		SFR_DATA_0 = 0x20;
		SFR_DATA_8 = 0x88;
		SFR_DATA_16 = 0x41;
	} else {
		// RTL8372, we assume that all the PHY_ADDR are start from 4 are counting up to 5.
		// Other unused macs are are set to zero.
		// When a external phy is connected to MAC 3 / SDS 0, the external PHY_ADDR is also programmed.
		uint8_t phy_addr_mac3 = 0x1f; // Don't use 0, this is a by default boardcast ID for RTL8221.

		SFR_DATA_0 = (0x1f << 5) | 0x1f;
		if (machine.sds_settings[0].usage == SDS_EPHY)
			phy_addr_mac3 = machine.sds_settings[0].sds_settings_t.ephy.phy_addr;
		SFR_DATA_8 = (phy_addr_mac3 << 7) | (0x1f >> 3);
		SFR_DATA_16 = (phy_addr_mac3 >> 1) | 0x40;
	}
	SFR_DATA_24 = 0x0a;
	reg_write(RTL837X_REG_SMI_PORT0_5_ADDR);

	// Program the PHY addresses for MAC 6-8.
	uint8_t phy_addr_mac8 = 0x1f;
	if (machine.sds_settings[1].usage == (uint8_t)SDS_EPHY)
		phy_addr_mac8 = machine.sds_settings[1].sds_settings_t.ephy.phy_addr;
	// Set address of external PHY connected MAC 8 / SDS 1
	//  [ 100 00 ] | 00 111 | 0 0110, port 8 = 0b10000 = 0x10
	REG_WRITE(RTL837X_REG_SMI_PORT6_9_ADDR, 0x00, 0x00, phy_addr_mac8 << 2, 0xe6);
}

//  RTL837X_REG_SMI_PORT0_5_ADDR 0x644C
// A408820
// 1010010000 00 0111 1111 1111 1111 
// a407fff

// a427fff

// regset 644C a427fff

void setup_i2c(void) __banked
{
	REG_SET(RTL837X_REG_I2C_MST_IF_CTRL, 0);
	// Configure SFP EEPROM address (0x50) as I2C device address
	// Configure SFP readings address (0x51) as I2C device address
	REG_WRITE(RTL837X_REG_I2C_CTRL, 0x00, 0x1 << (I2C_MEM_ADDR_WIDTH-16),  0x50 >> 5, (0x50 << 3) & 0xff);

	REG_SET(RTL837X_REG_I2C_CTRL2, 0);

	// HW Control register, enable I2C depending on PIN configuration
	reg_read_m(RTL837X_PIN_MUX_1);
	for (uint8_t sfp = 0; sfp < 2; sfp++) {
		if (!is_slot_sfp(sfp))
			continue;
		uint8_t i2c = machine.sds_settings[sfp].sds_settings_t.sfp.i2c;
		uint8_t scl_bus = (i2c >> RTL837X_REG_I2C_SCL_SHIFT) & RTL837X_REG_I2C_SCL_MASK;
		uint8_t sda_bus = (i2c >> RTL837X_REG_I2C_SDA_SHIFT) & RTL837X_REG_I2C_SDA_MASK;
		print_string("Configuring I2C for SFP idx="); print_byte(sfp); print_string(" SCL="); print_byte(scl_bus); print_string(", SDA="); print_byte(sda_bus); write_char('\n');
		// `& RTL837X_REG_I2C_SCL_MASK` is needed to silens a compiler warning 110.
		switch (scl_bus & RTL837X_REG_I2C_SCL_MASK) {
			case 3:
				// Bit 5-6 0b10 -> SCL (implies enabled SDA on bus 3)
				sfr_mask_data(0, 0x60, 0x40);
				break;
			case 2: 
				// Bit 15-16 0b01 -> SCL
				sfr_mask_data(1, 0x80, 0x80);
				sfr_mask_data(2, 0x01, 0x00);
				break;
			case 1:
				// Bit 11-12 0b01 -> SCL
				sfr_mask_data(1, 0x18, 0x08);
				break;
			case 0:
				// Bit 7-8 0b01 -> SCL
				sfr_mask_data(0, 0x80, 0x80);
				sfr_mask_data(1, 0x01, 0x00);
				break;
		}

		switch (sda_bus) {
			case 4:
				// Bit 29 0b0 -> SDA
				sfr_mask_data(3, 0x20, 0x00);
				break;
			case 3:
				// Bit 5-6 0b10 -> SDA (implies enabled SCL on bus 3)
				sfr_mask_data(0, 0x60, 0x40);
				break;
			case 2:
				// Bit 17-18 0b01 -> SDA
				sfr_mask_data(2, 0x06, 0x02);
				break;
			case 1:
				// Bit 13-14 0b01 -> SDA
				sfr_mask_data(1, 0x60, 0x20);
				break;
			case 0:
				// Bit 9-10 0b01 -> SDA
				sfr_mask_data(1, 0x06, 0x02);
				break;
		}
	}
	reg_write_m(RTL837X_PIN_MUX_1);	
}


/* Give the switch a name carrying the tail of its MAC, so several of them on
 * one network are distinguishable out of the box. Called after the startup
 * config has been replayed and returns at once if that config already set a
 * name, so a configured switch does no work for it (suggested in review).
 *
 * Written without a loop on purpose. Locals - counters and pointers alike -
 * land in the 8051's internal-RAM overlay, and on an image with LACP and STP
 * both enabled that overlay is exhausted: a loop here makes the linker fail
 * with "Could not get 8 consecutive bytes in internal RAM for area OSEG".
 * Moving the code into its own function does not help; the overlay is shared
 * across the whole image. Hoisting the locals to xdata does not help either,
 * because itohex() is inline and brings its own frame. */
void set_hostname_default(void) __banked
{
	if (hostname[0] != NUL)
		return;

	strcpy((__xdata uint8_t *)hostname, "RTLPlayground-");
	hostname[14] = hex[uip_ethaddr.addr[3] >> 4];
	hostname[15] = hex[uip_ethaddr.addr[3] & 0xf];
	hostname[16] = hex[uip_ethaddr.addr[4] >> 4];
	hostname[17] = hex[uip_ethaddr.addr[4] & 0xf];
	hostname[18] = hex[uip_ethaddr.addr[5] >> 4];
	hostname[19] = hex[uip_ethaddr.addr[5] & 0xf];
	hostname[20] = NUL;
}
