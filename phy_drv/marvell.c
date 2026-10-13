#include <../phy_driver.h>
#include <marvell.h>

int8_t marvell_probe(uint8_t sds) {
	return sds == 1;
}
