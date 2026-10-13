#ifndef PHY_DRIVER
#define PHY_DRIVER
#include <stdint.h>
#include <stdbool.h>

#define PHY_DRIVER_CNT 1

enum PHYID {
	PHY_MARVELL_88E1111_FINISAR,
	PHY_RTL8224,
	PHY_NONE,
};

struct phy_driver {
	uint32_t phy_id;
	uint32_t phy_id_mask;
	char name[33];
	int8_t (*probe)(uint8_t sds);
};

enum PHYID phy_probe(__xdata uint32_t id);

#endif