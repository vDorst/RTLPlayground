#ifndef _MACHINE_DEFS_H_
#define _MACHINE_DEFS_H_

/*
 * Shared prelude for the per-machine board definitions in this directory.
 *
 * Each .c file in this directory defines exactly one board's
 * `__code const struct machine machine`, and is named after the MACHINE_* symbol
 * it belongs to. The Makefile picks the file matching $(MACHINE), so no #if/#elif
 * selection happens here.
 */

#include "machine.h"
#include "rtl837x_pins.h"
#include "rtl837x_leds.h"
#include "rtl837x_sfr.h"
#include "rtl837x_regs.h"
#include "rtl837x_common.h"

#define I2CBUS_INVALID (7)
// This is a MACRO to check if I2C_BUS_FROM_SDA_PIN or I2C_BUS_FROM_SCL_PIN don't return I2CBUS_INVALID.
// When I2CBUS_INVALID is returned, the compiler stops with
// "error 176: sizeof applied to an incomplete type" pointing at the offending .i2c line.
#define I2CBUS_VALID(bus) (0 * sizeof(char[(bus) != I2CBUS_INVALID ? 1 : 0]))
#define I2CBUS(sda, scl) \
	((uint8_t)((I2C_BUS_FROM_SCL_PIN(scl) << RTL837X_REG_I2C_SCL_SHIFT) | \
		   (I2C_BUS_FROM_SDA_PIN(sda) << RTL837X_REG_I2C_SDA_SHIFT) | \
		   I2CBUS_VALID(I2C_BUS_FROM_SCL_PIN(scl)) | \
		   I2CBUS_VALID(I2C_BUS_FROM_SDA_PIN(sda))))

#define I2C_BUS_FROM_SDA_PIN(sda_pin) \
	((sda_pin) == GPIO47_I2C_SDA0 ? 0 : \
	 (sda_pin) == GPIO49_I2C_SDA1 ? 1 : \
	 (sda_pin) == GPIO51_I2C_SDA2_UART1_RX ? 2 : \
	 (sda_pin) == GPIO41_I2C_SDA3_MDIO1 ? 3 : \
	 (sda_pin) == GPIO39_I2C_SDA4 ? 4 : I2CBUS_INVALID)

#define I2C_BUS_FROM_SCL_PIN(scl_pin) \
	((scl_pin) == GPIO46_I2C_SCL0 ? 0 : \
	 (scl_pin) == GPIO48_I2C_SCL1 ? 1 : \
	 (scl_pin) == GPIO50_I2C_SCL2_UART1_TX ? 2 : \
	 (scl_pin) == GPIO40_I2C_SCL3_MDC1 ? 3 : I2CBUS_INVALID)

#ifndef FRIENDLY_NAME
	#error FRIENDLY_NAME missing
#endif

#define MACHINE_COMMON_SETTING \
	.machine_name = FRIENDLY_NAME, \
	.isRTL8373 = MACHINE_CHIPSET == RTL8373

#define MACHINE_CUSTOM_INIT(body) \
    _Pragma("codeseg BANK2") \
    void machine_custom_init(void) __banked { body }

/* This macro is used to define with mac/serdes is connected to with port number.
 * But also generating the bitmask for various locations were needed.
 * MASK_PHYS bitmask for the internal PHYs
 * MASK_MACS bitmask for the used MACS including the CPU port.
 */
#define PORTS(mac0, mac1, mac2, mac3_sds0, mac4, mac5, mac6, mac7, mac8_sds1) \
	enum { \
		MAC_USE_0 = mac0, \
		MAC_USE_1 = mac1, \
		MAC_USE_2 = mac2, \
		MAC_USE_3 = mac3_sds0, \
		MAC_USE_4 = mac4, \
		MAC_USE_5 = mac5, \
		MAC_USE_6 = mac6, \
		MAC_USE_7 = mac7, \
		MAC_USE_8 = mac8_sds1, \
		MASK_PHYS = \
		(((mac0) != 0) ? (1u << 0) : 0u) | \
		(((mac1) != 0) ? (1u << 1) : 0u) | \
		(((mac2) != 0) ? (1u << 2) : 0u) | \
		(((mac3_sds0) != 0) ? (1u << 3) : 0u) | \
		(((mac4) != 0) ? (1u << 4) : 0u) | \
		(((mac5) != 0) ? (1u << 5) : 0u) | \
		(((mac6) != 0) ? (1u << 6) : 0u) | \
		(((mac7) != 0) ? (1u << 7) : 0u) | \
		(((mac8_sds1) != 0) ? (1u << 8) : 0u), \
		MASK_MACS = \
		(((mac0) != 0) ? (1u << 0) : 0u) | \
		(((mac1) != 0) ? (1u << 1) : 0u) | \
		(((mac2) != 0) ? (1u << 2) : 0u) | \
		(((mac3_sds0) != 0) ? (1u << 3) : 0u) | \
		(((mac4) != 0) ? (1u << 4) : 0u) | \
		(((mac5) != 0) ? (1u << 5) : 0u) | \
		(((mac6) != 0) ? (1u << 6) : 0u) | \
		(((mac7) != 0) ? (1u << 7) : 0u) | \
		(((mac8_sds1) != 0) ? (1u << 8) : 0u) | \
		(1u << 9) \
	};

#endif
