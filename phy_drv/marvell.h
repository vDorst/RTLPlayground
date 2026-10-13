
#ifndef _MARVELL_H
#define _MARVELL_H

#include <stdint.h>

/* Marvel 88E1111 in Finisar SFP module with modified PHY ID */
#define MARVELL_PHY_ID_88E1111_FINISAR	0x01ff0cc0

/* Mask used for ID comparisons */
#define MARVELL_PHY_ID_MASK		0xfffffff0

int8_t marvell_probe(uint8_t sds);

#endif