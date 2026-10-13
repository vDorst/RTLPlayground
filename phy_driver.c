#include "phy_drv/marvell.h"
#include "rtl837x_common.h"
#include "phy_driver.h"

extern __code struct phy_driver phy_drivers[PHY_DRIVER_CNT];

enum PHYID phy_probe(__xdata uint32_t id) {
	print_string("\nP: ID = ");
	print_long(id);
	for (uint8_t i = 0; i < PHY_DRIVER_CNT; i++) {
		print_string(" M: MASK = "); print_long(phy_drivers[i].phy_id_mask);
		print_string(" I: ID = "); print_long(phy_drivers[i].phy_id);
		print_string(" R: ID = "); print_long(id & phy_drivers[i].phy_id_mask);
		if ((id & phy_drivers[i].phy_id_mask) == phy_drivers[i].phy_id) {
			print_string("\nID MATCH\n");
			return i;
		}
	}
	print_string("\nID NO-MATCH\n");
	return PHY_NONE;
}

__code struct phy_driver phy_drivers[PHY_DRIVER_CNT] = {
	{
    	.phy_id = MARVELL_PHY_ID_88E1111_FINISAR,
		.phy_id_mask = MARVELL_PHY_ID_MASK,
		.name = { "Marvell 88E1111 (Finisar)" },
		.probe = marvell_probe,
	},
};