#define MACHINE_CHIPSET RTL8372
#include "machine_defs.h"

PORTS(NOP, NOP, NOP, 5, 1, 2, 3, 4, 6)

__code const struct machine machine = {
    MACHINE_COMMON_SETTING,
    .log_to_phys_port = { MAC_USE_0, MAC_USE_1, MAC_USE_2, MAC_USE_3, MAC_USE_4, MAC_USE_5, MAC_USE_6, MAC_USE_7, MAC_USE_8},
    // Left port (J2)
    .sds_settings[0].usage = SDS_EPHY,
    .sds_settings[0].sds_settings_t.ephy.type = RTL8221B,
    .sds_settings[0].sds_settings_t.ephy.phy_addr = 0,
    .sds_settings[0].sds_settings_t.ephy.reset_pin = GPIO_NA,

    // SFP port (J3)
    .sds_settings[1].usage = SDS_SFP,
    .sds_settings[1].sds_settings_t.sfp.pin_detect = GPIO38,
    .sds_settings[1].sds_settings_t.sfp.pin_los = GPIO_NA,
    .sds_settings[1].sds_settings_t.sfp.pin_tx_disable = GPIO_NA,
    .sds_settings[1].sds_settings_t.sfp.i2c = I2CBUS( GPIO39_I2C_SDA4, GPIO40_I2C_SCL3_MDC1 ),

    .mac_flash_offset = 0x1FC000,
    .reset_pin = GPIO48_I2C_SCL1,
    .high_leds = { .mux =  LED_28_SYS | LED_29, .enable = LED_27 | LED_28_SYS | LED_29 },
    .port_led_set = { 0, 0, 0, 0, 0, 0, 0, 0, 1},
    .led_sets = {
        { /* RJ45: */
            /* Green */
            LEDS_2G5 | LEDS_LINK | LEDS_ACT,
            /* Amber */
            LEDS_1G | LEDS_100M | LEDS_10M | LEDS_LINK | LEDS_ACT,
            LEDS_DUPLEX,
            LEDS_2G5 | LEDS_LINK | LEDS_ACT
        }, { /* SFP PORT: */
            /* Green */
            LEDS_2G5 | LEDS_1G | LEDS_100M | LEDS_LINK | LEDS_ACT,
            /* Blue */
            LEDS_10G | LEDS_LINK | LEDS_ACT,
            LEDS_2G5 | LEDS_LINK,
            LEDS_COL | LEDS_DUPLEX
        },
    },
    .led_mux_custom = 1,
    .led_mux = {
        0x00,0x01,0x04,0x05,0x08,0x09,0x0c,0x3f,0x0d,0x10,0x11,0x0e,0x14,0x11,0x12,0x15,0x15,0x16,0x18,0x19,0x1a,0x19,0x1d,0x1e,0x1c,0x1d,0x20,0x21
    },
};

MACHINE_CUSTOM_INIT(
	uint16_t pval;

	reg_bit_set(RTL837X_REG_LED_GLB_IO_EN, 6);
	reg_bit_set(RTL837X_REG_LED_MODE, 17);
	reg_bit_clear(RTL837X_REG_LED_MODE, 9);
	reg_bit_clear(RTL837X_REG_LED_MODE, 7);

	// OEM firmware sets these companion SDS0 polarity bits for the RTL8221B.
	sds_read(0, 0, 0);
	pval = SFR_DATA_U16;
	sds_write_v(0, 0, 0, pval | 0x100);

	sds_read(0, 6, 2);
	pval = SFR_DATA_U16;
	sds_write_v(0, 6, 2, pval | 0x4000);
)
