#include <stdint.h>
#include <stdbool.h>
#include "marvell.h"
#include "../phy_driver.h"
#include "../sfp.h"
#include "../rtl837x_regs.h"
#include "../rtl837x_sfr.h"
#include "../rtl837x_common.h"

static bool marvell_set_page(uint8_t sds, uint8_t page)
{
	return i2c_mdio_phy_write_c45(sds, SFP_PHY_ADDR, -1, MII_MARVELL_PHY_PAGE, page);
}

bool marvell_phy_modify(uint8_t sds, uint8_t reg, uint16_t mask, uint16_t set) __reentrant {
	if (!i2c_mdio_phy_read_c45(sds, SFP_PHY_ADDR, -1, reg))
		return false;
	uint16_t val = SFR_DATA_U16LE;
	print_string("READ: "); print_short(val); print_string(" -> ");
	val &= ~(mask);
	val |= set;
	print_short(val);
	write_char('\n');
	if (val == SFR_DATA_U16LE)
		return true;
	return i2c_mdio_phy_write_c45(sds, SFP_PHY_ADDR, -1, reg, val);
}

int8_t m88e1111_config_init(uint8_t sds) __banked {
	print_string("\nmci1\n");
	if (!marvell_phy_modify(sds,MII_M1111_PHY_EXT_SR, MII_M1111_HWCFG_MODE_MASK |
			  MII_M1111_HWCFG_FIBER_COPPER_AUTO |
			  MII_M1111_HWCFG_FIBER_COPPER_RES, MII_M1111_HWCFG_MODE_SGMII_NO_CLK))
		return -1;

	print_string("\nmci2\n");
	if (!marvell_set_page(sds, MII_MARVELL_COPPER_PAGE))
		return -1;

	print_string("\nmci3\n");
	return SDS_SGMII;
}


int8_t m88e1111gbe_config_init(uint8_t sds) __banked {
	return m88e1111_config_init(sds);
}


static bool genphy_config_aneg(uint8_t sds) {
	return marvell_phy_modify(sds, 0x00, BMCR_ISOLATE, BMCR_ANENABLE | BMCR_ANRESTART);
}

static bool genphy_soft_reset(uint8_t sds) {
	return marvell_phy_modify(sds, 0x00, BMCR_ISOLATE, BMCR_RESET | BMCR_ANRESTART);
}

static bool genphy_restart_aneg(uint8_t sds) {
	return marvell_phy_modify(sds, 0x00, BMCR_ISOLATE, BMCR_ANENABLE | BMCR_ANRESTART);
}

static bool genphy_check_and_restart_aneg(uint8_t sds, bool restart)
{
	if (!restart) {
		if (!i2c_mdio_phy_read_c45(sds, SFP_PHY_ADDR, -1, 0x00))
			return false;
		uint16_t val = SFR_DATA_U16LE;

		if (!(val & BMCR_ANENABLE) || (val & BMCR_ISOLATE))
			restart = true;
	}

	if (restart)
		return genphy_restart_aneg(sds);

	return true;
}

static bool marvell_config_aneg(uint8_t sds)
{
	if (!genphy_config_aneg(sds))
		goto err;

	return genphy_soft_reset(sds);
err:
	return false;
}

bool m88e1111_config_aneg(uint8_t sds) __banked {
	if (!marvell_set_page(sds, MII_MARVELL_COPPER_PAGE))
		goto err;

	if (!genphy_config_aneg(sds))
		goto err;

	if (!marvell_set_page(sds, MII_MARVELL_FIBER_PAGE))
		goto err;

	if (!genphy_check_and_restart_aneg(sds, false))
		goto err;

	return marvell_set_page(sds, MII_MARVELL_COPPER_PAGE);
err:
	return false;

}
