#include "phy_drv/marvell.h"
#include "rtl837x_common.h"
#include "phy_driver.h"

extern __code const struct phy_driver phy_drivers[PHY_DRIVER_CNT];

enum PHYID phy_probe(__xdata uint32_t * id) {
	print_string("\nP: ID = ");
	print_long(*id);
	for (uint8_t i = 0; i < PHY_DRIVER_CNT; i++) {
		print_string(" M: MASK = "); print_long(phy_drivers[i].phy_id_mask);
		print_string(" I: ID = "); print_long(phy_drivers[i].phy_id);
		print_string(" R: ID = "); print_long(*id & phy_drivers[i].phy_id_mask);
		if ((*id & phy_drivers[i].phy_id_mask) == phy_drivers[i].phy_id) {
			print_string("\nID MATCH\n");
			return i;
		}
	}
	print_string("\nID NO-MATCH\n");
	return PHY_NONE;
}

__code const struct phy_driver phy_drivers[PHY_DRIVER_CNT] = {
	{
		.phy_id = 0x001ccad0,
		.phy_id_mask = 0xffffffffUL,
		.name		= { "RTL8224 2.5Gbps PHY" },
		// .flags		= PHY_POLL_CABLE_TEST,
		// .probe		= rtl8224_probe,
		// .config_init	= rtl8224_config_init,
		// .get_features	= rtl822x_c45_get_features,
		//.config_aneg	= rtl822x_c45_config_aneg,
		// .read_status	= rtl822x_c45_read_status,
		// .suspend	= genphy_c45_pma_suspend,
		// .resume		= rtlgen_c45_resume,
		// .cable_test_start = rtl8224_cable_test_start,
		// .cable_test_get_status = rtl8224_cable_test_get_status,
	},
	{
    	.phy_id = MARVELL_PHY_ID_88E1111_FINISAR,
		.phy_id_mask = MARVELL_PHY_ID_MASK,
		.name = { "Marvell 88E1111 (Finisar)" },
		.config_init = m88e1111gbe_config_init,
		.config_aneg = m88e1111_config_aneg,
	},
	// {
	// 	.phy_id = MARVELL_PHY_ID_CUX36100,
	// 	.phy_id_mask = MARVELL_PHY_ID_MASK,
	// 	.name = { "Marvell CUX3610" },
	// 	.config_init = m88e1111gbe_config_init,
	// 	// .config_aneg = m88e1111_config_aneg,
	// },
};

int8_t phy_config_init(uint8_t sds, enum PHYID id) __banked {
	print_byte(id);
	__code const struct phy_driver * pd = &phy_drivers[id];

	if ((uint8_t)id < PHY_DRIVER_CNT && pd->config_init != 0)
		return pd->config_init(sds);

	print_string("\nNo config_init\n");

	return -1;
}

int8_t phy_config_aneg(uint8_t sds, enum PHYID id) __banked {
	if ((uint8_t)id < PHY_DRIVER_CNT && phy_drivers[id].config_aneg != 0)
		return phy_drivers[id].config_aneg(sds);

	return -1;
}
