#include "machine_defs.h"

__code const struct machine machine = {
	MACHINE_COMMON_SETTING,
	.log_to_phys_port = {NOP, NOP, NOP, 6, 1, 2, 3, 4, 5},
	// Right SFP port (J2)
	.sds_settings[0].usage = SDS_SFP,
	.sds_settings[0].sds_settings_t.sfp.pin_detect = GPIO50_I2C_SCL2_UART1_TX,
	.sds_settings[0].sds_settings_t.sfp.pin_los = GPIO_NA,
	.sds_settings[0].sds_settings_t.sfp.pin_tx_disable = GPIO_NA,
	.sds_settings[0].sds_settings_t.sfp.i2c = I2CBUS( GPIO41_I2C_SDA3_MDIO1, GPIO40_I2C_SCL3_MDC1 ), /* GPIO 40 */
	// Left SFP port (J4)
	.sds_settings[1].usage = SDS_SFP,
	.sds_settings[1].sds_settings_t.sfp.pin_detect = GPIO30_ACL_BIT3_EN,
	.sds_settings[1].sds_settings_t.sfp.pin_los = GPIO_NA,
	.sds_settings[1].sds_settings_t.sfp.pin_tx_disable = GPIO_NA,
	.sds_settings[1].sds_settings_t.sfp.i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ), /* GPIO 39 */
	.reset_pin = GPIO54_ACL_BIT2_EN,
	.high_leds = { .mux = LED_27 | LED_29, .enable = LED_28_SYS | LED_29 },
	.port_led_set = { 0, 0, 0, 1, 0, 0, 0, 0, 1},
	.led_sets = {
		{ /* RJ45: Green LED */
			LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT | LEDS_10G,
			0,
			/* Amber LED */
			LEDS_2G5 | LEDS_LINK | LEDS_ACT,
			0
		}, { /* SFP PORT: SINGLE GREEN LED */
			LEDS_10M | LEDS_100M | LEDS_1G | LEDS_2G5 | LEDS_10G | LEDS_LINK | LEDS_ACT,
			0,
			0,
			0,
		},
	},
	.mac_flash_offset = 0x1FC000,
};
