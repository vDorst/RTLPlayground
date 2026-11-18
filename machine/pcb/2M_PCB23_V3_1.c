#include "machine_defs.h"

PORTS(1, 2, 3, 4, 5, 6, 7, 8, 9)

// 2M-PCB23-V3.2 is wired like V3.1 (SFP detection, LEDs, factory MAC location)
__code const struct machine machine = {
	MACHINE_COMMON_SETTING,
	.log_to_phys_port = { MAC_USE_0, MAC_USE_1, MAC_USE_2, MAC_USE_3, MAC_USE_4, MAC_USE_5, MAC_USE_6, MAC_USE_7, MAC_USE_8},
	// RTL8224
	.sds_settings[0].usage = SDS_EPHY,
	.sds_settings[0].sds_settings_t.ephy.type = RTL8224,
	.sds_settings[0].sds_settings_t.ephy.phy_addr = 0,
	.sds_settings[0].sds_settings_t.ephy.reset_pin = GPIO_NA,
	// SFP-PORT
	.sds_settings[1].usage = SDS_SFP,
	.sds_settings[1].sds_settings_t.sfp.pin_detect = GPIO38,
	.sds_settings[1].sds_settings_t.sfp.pin_los = GPIO_NA,
	.sds_settings[1].sds_settings_t.sfp.pin_tx_disable = GPIO_NA,
	.sds_settings[1].sds_settings_t.sfp.i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),
	.reset_pin = GPIO48_I2C_SCL1,
	.high_leds = { .mux = LED_27 | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
	.led_sets = {
		{   /* RJ45: First LED, yellow, second LED: green */
			LEDS_2G5 | LEDS_LINK,
            LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
			0,
			0,
		}, { /* SFP PORT, SINGLE GREEN LED */
			LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G,
			0,
			0,
			0,
		}},
	.led_mux_custom = 1,
	.led_mux = {0x00, 0x01, 0x04, 0x05, 0x08, 0x09, 0x0c, 0x09, 0x0d, 0x10,
				0x11, 0x0e, 0x14, 0x11, 0x12, 0x15, 0x15, 0x16, 0x18, 0x19,
				0x1a, 0x19, 0x1d, 0x1e, 0x1c, 0x1d, 0x20, 0x21},
	.mac_flash_offset = 0x1FC000,
};
