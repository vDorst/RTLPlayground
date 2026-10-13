
#ifndef _MARVELL_H
#define _MARVELL_H

#include <stdint.h>
#include <stdbool.h>

#define BIT(nr)			((uint16_t)(1) << (nr))


#define BMCR_RESV		0x003f
#define BMCR_SPEED1000		0x0040
#define BMCR_CTST		0x0080
#define BMCR_FULLDPLX		0x0100
#define BMCR_ANRESTART		0x0200
#define BMCR_ISOLATE		0x0400
#define BMCR_PDOWN		0x0800
#define BMCR_ANENABLE		0x1000
#define BMCR_SPEED100		0x2000
#define BMCR_LOOPBACK		0x4000
#define BMCR_RESET		0x8000
#define BMCR_SPEED10		0x0000


/* Marvel 88E1111 in Finisar SFP module with modified PHY ID */
#define MARVELL_PHY_ID_88E1111_FINISAR	0x01ff0cc0

/* Mask used for ID comparisons */
#define MARVELL_PHY_ID_MASK		0xfffffff0

#define MII_MARVELL_PHY_PAGE		22
#define MII_MARVELL_COPPER_PAGE		0x00
#define MII_MARVELL_FIBER_PAGE		0x01
#define MII_MARVELL_MSCR_PAGE		0x02
#define MII_MARVELL_LED_PAGE		0x03
#define MII_MARVELL_VCT5_PAGE		0x05
#define MII_MARVELL_MISC_TEST_PAGE	0x06
#define MII_MARVELL_VCT7_PAGE		0x07
#define MII_MARVELL_WOL_PAGE		0x11
#define MII_MARVELL_MODE_PAGE		0x12

#define MII_M1111_PHY_EXT_SR		0x1b

#define MII_M1111_HWCFG_MODE_MASK		0xf
#define MII_M1111_HWCFG_MODE_SGMII_NO_CLK	0x4
#define MII_M1111_HWCFG_MODE_COPPER_1000X_AN	0x8
#define MII_M1111_HWCFG_SERIAL_AN_BYPASS	BIT(12)
#define MII_M1111_HWCFG_FIBER_COPPER_RES	BIT(13)
#define MII_M1111_HWCFG_FIBER_COPPER_AUTO	BIT(15)


int8_t m88e1111gbe_config_init(uint8_t sds) __banked;
int8_t m88e1111_config_init(uint8_t sds) __banked;
bool m88e1111_config_aneg(uint8_t sds) __banked;

#endif