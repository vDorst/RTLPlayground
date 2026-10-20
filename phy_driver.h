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
	// Inits the PHY and returns the interface or negative error number.
	int8_t (*config_init)(uint8_t sds) __banked;

	// Activate auto neg.
	bool (*config_aneg)(uint8_t sds) __banked;
};

enum PHYID phy_probe(__xdata uint32_t id);

#endif
