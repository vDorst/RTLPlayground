#include <stdint.h>
#include "rtl837x_common.h"
#include "rtl837x_sfr.h"
#include "rtl837x_regs.h"
#include "rtl837x_port.h"
#include "rtl837x_phy.h"
#include "phy.h"
#include "machine.h"
#include "boot.h"

extern __xdata uint8_t sfr_data[4];
extern __code const struct machine machine;
extern __xdata struct machine_runtime machine_detected;

#pragma codeseg BANK2
#pragma constseg BANK2

struct sds_patch_entry {
    uint8_t page;
    uint8_t reg;
    uint16_t value;
};

#define LAST {0xff, 0xff, 0xffff}

static __code const struct sds_patch_entry patch_an_10p3125g_a[] = {
	{0x21, 0x10, 0x4480},
	{0x21, 0x13, 0x0400},
	{0x21, 0x18, 0x6D02},
	{0x21, 0x1B, 0x424E},
	{0x21, 0x1D, 0x0002},
	{0x36, 0x1C, 0x1390},
	{0x2E, 0x04, 0x0080},
	{0x2E, 0x06, 0x0408},
	{0x2E, 0x07, 0x020D},
	{0x2E, 0x09, 0x0601},
	{0x2E, 0x0B, 0x222C},
	{0x2E, 0x0C, 0x9217},
	{0x2E, 0x16, 0x0743},
	{0x2E, 0x1D, 0xABB0},
	LAST
};

static __code const struct sds_patch_entry patch_an_10p3125g_b[] = {
	{0x21, 0x10, 0x4480},
	{0x21, 0x13, 0x0400},
	{0x21, 0x18, 0x6D02},
	{0x21, 0x1B, 0x424E},
	{0x21, 0x1D, 0x0002},
	{0x36, 0x1C, 0x1390},
	{0x36, 0x14, 0x003F},
	{0x36, 0x10, 0x0200},
	{0x2E, 0x04, 0x0080},
	{0x2E, 0x06, 0x0408},
	{0x2E, 0x07, 0x020D},
	{0x2E, 0x09, 0x0601},
	{0x2E, 0x0B, 0x222C},
	{0x2E, 0x0C, 0xA217},
	{0x2E, 0x0D, 0xFE40},
	{0x2E, 0x15, 0xF5C1},
	{0x2E, 0x16, 0x0443},
	{0x2E, 0x1D, 0xABB0},
	LAST
};

static __code const struct sds_patch_entry patch_an_3p125g_a[] = {
	{0x21, 0x10, 0x6480},
	{0x21, 0x13, 0x0400},
	{0x21, 0x18, 0x6D02},
	{0x21, 0x1B, 0x424E},
	{0x21, 0x1D, 0x0002},
	{0x36, 0x1C, 0x1390},
	{0x28, 0x04, 0x0080},
	{0x28, 0x09, 0x0601},
	{0x28, 0x0B, 0x232C},
	{0x28, 0x0C, 0x9217},
	{0x28, 0x0F, 0x5B50},
	{0x28, 0x16, 0x0443},
	{0x28, 0x1D, 0xABB0},
	LAST
};

static __code const struct sds_patch_entry patch_an_3p125g_b[] = {
	{0x21, 0x10, 0x6480},
	{0x21, 0x13, 0x0400},
	{0x21, 0x18, 0x6D02},
	{0x21, 0x1B, 0x424E},
	{0x21, 0x1D, 0x0002},
	{0x36, 0x1C, 0x1390},
	{0x36, 0x14, 0x003F},
	{0x36, 0x10, 0x0200},
	{0x28, 0x04, 0x0080},
	{0x28, 0x07, 0x1201},
	{0x28, 0x09, 0x0601},
	{0x28, 0x0B, 0x232C},
	{0x28, 0x0C, 0x9217},
	{0x28, 0x0F, 0x5B50},
	{0x28, 0x15, 0xE7F1},
	{0x28, 0x16, 0x0443},
	{0x28, 0x1D, 0xABB0},
	LAST
};

static __code const struct sds_patch_entry patch_an_1p25g_a[] = {
	{0x21, 0x10, 0x6480},
	{0x21, 0x13, 0x0400},
	{0x21, 0x18, 0x6D02},
	{0x21, 0x1B, 0x424E},
	{0x21, 0x1D, 0x0002},
	{0x36, 0x1C, 0x1390},
	{0x36, 0x14, 0x003F},
	{0x36, 0x10, 0x0300},
	{0x24, 0x04, 0x0080},
	{0x24, 0x07, 0x1201},
	{0x24, 0x09, 0x0601},
	{0x24, 0x0B, 0x232C},
	{0x24, 0x0C, 0x9217},
	{0x24, 0x0F, 0x5B50},
	{0x24, 0x15, 0xE7C1},
	{0x24, 0x16, 0x0443},
	{0x24, 0x1D, 0xABB0},
	LAST
};

static __code const struct sds_patch_entry patch_an_1p25g_b[] = {
	{0x21, 0x10, 0x6480},
	{0x21, 0x13, 0x0400},
	{0x21, 0x18, 0x6D02},
	{0x21, 0x1B, 0x424E},
	{0x21, 0x1D, 0x0002},
	{0x36, 0x1C, 0x1390},
	{0x36, 0x14, 0x003F},
	{0x36, 0x10, 0x0300},
	{0x24, 0x04, 0x0080},
	{0x24, 0x07, 0x1201},
	{0x24, 0x09, 0x0601},
	{0x24, 0x0B, 0x232C},
	{0x24, 0x0C, 0x9217},
	{0x24, 0x0F, 0x5B50},
	{0x24, 0x15, 0xE7C1},
	{0x24, 0x16, 0x0443},
	{0x24, 0x1D, 0xABB0},
	LAST
};

static __code const struct sds_patch_entry patch_an_125m_a[] = {
	{0x21, 0x10, 0x6480},
	{0x21, 0x13, 0x0400},
	{0x21, 0x18, 0x6D02},
	{0x21, 0x1B, 0x424E},
	{0x21, 0x1D, 0x0002},
	{0x36, 0x1C, 0x1390},
	{0x26, 0x04, 0x0080},
	{0x26, 0x09, 0x0601},
	{0x26, 0x0B, 0x232C},
	{0x26, 0x0C, 0x9217},
	{0x26, 0x0F, 0x5B50},
	{0x26, 0x16, 0x0443},
	{0x26, 0x1D, 0xABB0},
	LAST
};

static __code const struct sds_patch_entry patch_an_125m_b[] = {
	{0x21, 0x10, 0x6480},
	{0x21, 0x13, 0x0400},
	{0x21, 0x18, 0x6D02},
	{0x21, 0x1B, 0x424E},
	{0x21, 0x1D, 0x0002},
	{0x36, 0x1C, 0x1390},
	{0x36, 0x14, 0x003F},
	{0x36, 0x10, 0x0300},
	{0x26, 0x04, 0x0080},
	{0x26, 0x07, 0x1201},
	{0x26, 0x09, 0x0601},
	{0x26, 0x0B, 0x232C},
	{0x26, 0x0C, 0x9217},
	{0x26, 0x0F, 0x5B50},
	{0x26, 0x15, 0xE7C1},
	{0x26, 0x16, 0x0443},
	{0x26, 0x1D, 0xABB0},
	LAST
};

static __code const struct sds_patch_entry patch_data_mac[] = {
	{0x06, 0x12, 0x5078},
	{0x07, 0x06, 0x9401},
	{0x07, 0x08, 0x9401},
	{0x07, 0x0A, 0x9401},
	{0x07, 0x0C, 0x9401},
	{0x1F, 0x0B, 0x0003},
	{0x06, 0x03, 0xC45C},
	{0x06, 0x1F, 0x2100},
	LAST
};

static __code const struct sds_patch_entry patch_data_phy[] = {
	{0x06, 0x12, 0x5078},
	{0x06, 0x03, 0xc45c},
	{0x06, 0x1E, 0x000C},
	{0x06, 0x1F, 0x2100},
	LAST
};

#define BIT(b) ((uint16_t)1<<b)


#define SDS_PAGE_CTRL00  0x00
#define SDS_REG_CTRL00_REG00     0x00
#define SDS_REG_CTRL00_REG02     0x02
 #define SDS_CTRL00_REG02_XSG_SP_SDS_FRC_AN    BIT(8)
 #define SDS_CTRL00_REG02_XSG_SP_SDS_FRC_AN_EN BIT(9)
#define SDS_REG_CTRL00_REG04     0x04

#define SDS_PAGE_FRC 0x20
#define SDS_REG_FRC 0x00
#define SDS_FRC_RX_EN_ON_MASK    BIT(4)
#define SDS_FRC_RX_EN_VAL_MASK   BIT(5)
#define SDS_FRC_PDOWN_ON_MASK    BIT(6)
#define SDS_FRC_PDOWN_VAL_MASK   BIT(7)
#define SDS_FRC_CMU_EN_ON_MASK   BIT(10)
#define SDS_FRC_CMU_EN_VAL_MASK  BIT(11)

#define SDS_PAGE_CTRL01             0x01
#define SDS_REG_CTRL01_XSG_STS      0x1d 
#define SDS_CTRL01_XSG_STS_SYNC_OK   BIT(0)
#define SDS_CTRL01_XSG_STS_LINK_OK   BIT(4)
#define SDS_CTRL01_XSG_STS_SIG_OK    BIT(8)

#define SDS_PAGE_CTRL02       0x02
#define SDS_REG_CTRL02_XSG_AN   0x04
 #define SDS_CTRL02_XSG_AN_10_100_AsymmetricPause_MASK BIT(11)
 #define SDS_CTRL02_XSG_AN_10_100_Pause_MASK           BIT(10)
 #define SDS_CTRL02_XSG_AN_1G_AsymmetricPause_MASK BIT(8)
 #define SDS_CTRL02_XSG_AN_1G_Pause_MASK           BIT(7)
 #define SDS_CTRL02_XSG_AN_1G_HalfDuplex_MASK      BIT(6)
 #define SDS_CTRL02_XSG_AN_1G_FullDuplex_MASK      BIT(5)

#define SDS_PAGE_CTRL05             0x05
#define SDS_REG_CTRL05_10GR_STS     0x00
#define SDS_CTRL05_10GR_STS_SYNC_OK   BIT(0)
#define SDS_CTRL05_10GR_STS_HI_BER    BIT(1)
#define SDS_CTRL05_10GR_STS_LINK_OK   BIT(12)

#define SDS_PAGE_NWAY_AN   0x07
#define SDS_REG_NWAY_AN    17

#define SDS_PAGE_CTRL1F       0x1f
#define SDS_REG_CTRL1F_REG00  0x00
#define SDS_REG_CTRL1F_REG05  0x05
#define SDS_REG_CTRL1F_10GR_AN  0x0B
#define SDS_CTRL1F_10GR_AN_Pause_MASK     BIT(2)
#define SDS_CTRL1F_10GR_AN_AsymmetricPause_MASK  BIT(3)

void static serdes_off(uint8_t sds) {

	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK);
	// Delay 20mS
	delay(4);
	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK, SDS_FRC_RX_EN_ON_MASK);
	// Delay 50mS
	delay(10);

	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_PDOWN_ON_MASK | SDS_FRC_PDOWN_VAL_MASK, SDS_FRC_PDOWN_ON_MASK);
	// Delay 20mS
	delay(4);
	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_PDOWN_ON_MASK | SDS_FRC_PDOWN_VAL_MASK, SDS_FRC_PDOWN_ON_MASK | SDS_FRC_PDOWN_VAL_MASK);
	// Delay 50mS
	delay(10);

	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_CMU_EN_ON_MASK | SDS_FRC_CMU_EN_VAL_MASK, SDS_FRC_CMU_EN_ON_MASK | SDS_FRC_CMU_EN_VAL_MASK);
	// Delay 20mS
	delay(4);
	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_CMU_EN_ON_MASK | SDS_FRC_CMU_EN_VAL_MASK, SDS_FRC_CMU_EN_ON_MASK);
	// Delay 50mS
	delay(10);
}

void static serdes_on(uint8_t sds) {

	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_CMU_EN_ON_MASK | SDS_FRC_CMU_EN_VAL_MASK, SDS_FRC_CMU_EN_ON_MASK);
	// Delay 20mS
	delay(4);
	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_CMU_EN_ON_MASK | SDS_FRC_CMU_EN_VAL_MASK, SDS_FRC_CMU_EN_ON_MASK | SDS_FRC_CMU_EN_VAL_MASK);
	// Delay 50mS
	delay(10);
	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_CMU_EN_ON_MASK | SDS_FRC_CMU_EN_VAL_MASK, 0);
	// Delay 20mS
	delay(4);

	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_PDOWN_ON_MASK | SDS_FRC_PDOWN_VAL_MASK, SDS_FRC_PDOWN_ON_MASK | SDS_FRC_PDOWN_VAL_MASK);
	// Delay 20mS
	delay(4);
	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_PDOWN_ON_MASK | SDS_FRC_PDOWN_VAL_MASK, SDS_FRC_PDOWN_ON_MASK);
	// Delay 50mS
	delay(10);
	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_PDOWN_ON_MASK | SDS_FRC_PDOWN_VAL_MASK, 0);
	// Delay 20mS
	delay(4);

	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK, SDS_FRC_RX_EN_ON_MASK);
	// Delay 20mS
	delay(4);
	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK);
	// Delay 50mS
	delay(10);
	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK, 0);
	// Delay 50mS
	delay(10);
}

void static serdes_reset_x(uint8_t sds) {
	uint16_t val;
	sds_read(sds, SDS_PAGE_FRC, SDS_REG_FRC);
	val = SFR_DATA_U16;

	if ((val & (SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK)) == SDS_FRC_RX_EN_ON_MASK)
		return;

	sds_read(sds, SDS_PAGE_CTRL01, SDS_REG_CTRL01_XSG_STS);
	val = SFR_DATA_U16;

	bool sig_ok = (val & SDS_CTRL01_XSG_STS_SIG_OK) != 0;
	if (!sig_ok) {
		print_string("No reset because not active\n");
		return;
	}
	
	bool link_ok = (val & SDS_CTRL01_XSG_STS_LINK_OK) != 0;
	bool sync_ok = (val & SDS_CTRL01_XSG_STS_SYNC_OK) != 0;

	if (sync_ok == false)
		goto reset;
	else
		if (link_ok == false)
			goto reset;
	return;

reset:
	print_string("\tReset_x SERDES\n");

	sds_modify(sds, 0x00, 0x00, BIT(1), BIT(1));
	// Delay 20mS
	delay(4);
	sds_modify(sds, 0x00, 0x00, BIT(1), 0);
	// Delay 20mS
	delay(4);
	sds_modify(sds, 0x00, 0x00, BIT(1), BIT(1));
	// Delay 20mS
	delay(4);
}

void static serdes_reset_r(uint8_t sds) {
	uint16_t val;

	sds_read(sds, SDS_PAGE_FRC, SDS_REG_FRC);
	val = SFR_DATA_U16;

	if ((val & (SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK)) == SDS_FRC_RX_EN_ON_MASK)
		return;

	sds_modify(sds, 0x21, 0x00, BIT(2), BIT(2));
	sds_modify(sds, 0x36, 0x05, ((uint16_t)0xf) << 11, ((uint16_t)0x8) << 11);
	sds_write_v(sds, 0x1f, 0x02, 0x1f);

	// Get RX status
	sds_read(sds, 0x1f, 0x15);
	val = SFR_DATA_U16;

	bool rx_idle = (val & BIT(7)) == BIT(7);
	bool nsq = (val & BIT(6)) == BIT(6);

	if (!(nsq == true || rx_idle == false)) {
		print_string("No reset because not active\n");
		return;
	}

	sds_read(sds, SDS_PAGE_CTRL05, SDS_REG_CTRL05_10GR_STS);
	val = SFR_DATA_U16;

	bool sync_ok = (val & SDS_CTRL05_10GR_STS_SYNC_OK) == SDS_CTRL05_10GR_STS_SYNC_OK;
	if (sync_ok == false)
		goto reset;
	else {
		bool link_ok = (val & SDS_CTRL05_10GR_STS_LINK_OK) == SDS_CTRL05_10GR_STS_LINK_OK;
		if (link_ok == false)
			goto reset;
		bool hi_ber = (val & SDS_CTRL05_10GR_STS_HI_BER) == SDS_CTRL05_10GR_STS_HI_BER;
		if (hi_ber == true)
			goto reset;
	}
	return;

reset:
	print_string("\tReset_r SERDES\n");

	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK);
	// Delay 20mS
	delay(4);
	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK, SDS_FRC_RX_EN_ON_MASK);
	// Delay 20mS
	delay(4);
	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK);
	// Delay 20mS
	delay(4);
	sds_modify(sds, SDS_PAGE_FRC, SDS_REG_FRC, SDS_FRC_RX_EN_VAL_MASK | SDS_FRC_RX_EN_ON_MASK, 0);
	// Delay 20mS
	delay(4);
}

void static serdes_an_patch(uint8_t sds, enum sds_modes interface) {
	__code const struct sds_patch_entry *ptr;

	bool isN = machine_detected.isN;

	switch (interface) {
		case SDS_10G_QXGMII:
		case SDS_10GBASER:
		case SDS_10G_USXGMII:
			if (!isN) {
				print_string_no_syslog("Patch: patch_an_10p3125g_a: ");
				ptr = patch_an_10p3125g_a;
			} else {
				print_string_no_syslog("Patch: patch_an_10p3125g_b: ");
				ptr = patch_an_10p3125g_b;
			}
			break;
		case SDS_2G5_SGMII:
		case SDS_2500BASEX:
			if (!isN) {
				print_string_no_syslog("Patch: patch_an_3p125g_a: ");
				ptr = patch_an_3p125g_a;
			} else {
				print_string_no_syslog("Patch: patch_an_3p125g_b: ");
				ptr = patch_an_3p125g_b;
			}
			break;
		
		case SDS_SGMII:
		case SDS_1000BASEX:
			if (!isN) {
				print_string_no_syslog("Patch: patch_an_1p25g_a: ");
				ptr = patch_an_1p25g_a;
			} else {
				print_string_no_syslog("Patch: patch_an_1p25g_b: ");
				ptr = patch_an_1p25g_b;
			}
			break;
		case SDS_100FX:
			if (!isN) {
				print_string_no_syslog("Patch: patch_an_125m_a: ");
				ptr = patch_an_125m_a;
			} else {
				print_string_no_syslog("Patch: patch_an_125m_b: ");
				ptr = patch_an_125m_b;
			}
			break;
		default:
			return;
	}

	// Apply the patch.
	uint8_t idx = 0;
	while(1) {
		if (ptr[idx].page == 0xFF)
			break;
		sds_write_v(sds, ptr[idx].page, ptr[idx].reg, ptr[idx].value);
		idx++;
	}

	print_byte(idx);
	write_char('\n');
}

void static serdes_mac_patch(uint8_t sds) {
	print_string_no_syslog("Patch: patch_data_mac: ");

	__code const struct sds_patch_entry *ptr = patch_data_mac;

	uint8_t idx = 0;
	while(1) {
		if (ptr[idx].page == 0xFF)
			break;
		sds_write_v(sds, ptr[idx].page, ptr[idx].reg, ptr[idx].value);
		idx++;
	}

	print_byte(idx);
	write_char('\n');
}

void print_sds_mode(enum sds_modes interface) {
	__code const char * msg;
	switch (interface) {
		case SDS_OFF:
			msg = "OFF\n";
			break;
		case SDS_SGMII:
			msg = "SGMII\n";
			break;
		case SDS_1000BASEX:
			msg = "1000BX\n";
			break;
		case SDS_2G5_SGMII:
			msg = "2G5_SGMII\n";
			break;
		case SDS_2500BASEX:
			msg = "2500BASEX\n";
			break;
		case SDS_10GBASER:
			msg = "10GBASER\n";
			break;
		case SDS_10G_USXGMII:
			msg = "USXGMII\n";
			break;
		case SDS_10G_QXGMII:
			msg = "QXGMII\n";
			break;
		case SDS_100FX:
			msg = "100FX\n";
			break;
		default:
			msg = "UNKNOWN\n";
	}
	print_string(msg);
}

void sds_pcs_config(uint8_t sds, enum sds_modes interface, bool en_inband) __banked {
	print_string("Set SDS: "); print_byte(sds);
    print_string(", mode: ");
	print_sds_mode(interface);

	reg_bit_clear(RTL837X_REG_SDS_MODES, sds == 1 ? RTL837X_REG_SDS_MODES_MAC8_8221_BIT : RTL837X_REG_SDS_MODES_MAC3_8221_BIT);
	
	serdes_off(sds);

	// Set USX_SUB_MODE
	reg_read_m(RTL837X_REG_SDS_MODES);
	// USX_SUB_MODE
	if (sds == 0) {
		// SDD0: 1f << 10
		sfr_data[2] &= ~(0x1f << 2);
		if (interface == SDS_10G_QXGMII) {
			sfr_data[2] |= (2 << 2);
			interface = 0x0d;
		}
	} else {
		// SDS1: 1f << 16
		sfr_data[1] &= ~(0x1f << 0);
		// SDS_10G_QXGMII is not supported for SDS1.
	}

	// Set interface
	if (sds == 0) {
		// SDD0: 1f << 0
		sfr_data[3] &= ~(0x1f << 0);
		sfr_data[3] |= (interface << 0);
	} else {
		// SDS1: 1f << 5
		sfr_data[3] &= ~(0x1f << 5);
		sfr_data[3] |= (interface << 5);
		sfr_data[2] &= ~(0x1f >> 3);
		sfr_data[2] |= (interface >> 3);
	}
	reg_write_m(RTL837X_REG_SDS_MODES);

	if (interface == SDS_OFF)
		return;

	// apply patch
    serdes_an_patch(sds, interface);
    serdes_mac_patch(sds);

	// delay 250mS
	delay(50);

	// Set Auto Negotiation Pause/AsymPause
	switch (interface) {
		case SDS_100FX:
			sds_modify(sds, SDS_PAGE_CTRL1F, SDS_REG_CTRL1F_REG05, BIT(3) | BIT(2), BIT(3) | BIT(2));
			// Set pause_bits
			sds_modify(sds, SDS_PAGE_CTRL02, SDS_REG_CTRL02_XSG_AN, SDS_CTRL02_XSG_AN_10_100_AsymmetricPause_MASK | SDS_CTRL02_XSG_AN_10_100_Pause_MASK, SDS_CTRL02_XSG_AN_10_100_AsymmetricPause_MASK | SDS_CTRL02_XSG_AN_10_100_Pause_MASK);
			break;
		case SDS_1000BASEX:
		case SDS_2500BASEX:
			sds_modify(sds, SDS_PAGE_CTRL1F, SDS_REG_CTRL1F_REG05, BIT(3) | BIT(2), BIT(2));
			// Set pause_bits
			sds_modify(sds, SDS_PAGE_CTRL02, SDS_REG_CTRL02_XSG_AN, SDS_CTRL02_XSG_AN_1G_AsymmetricPause_MASK | SDS_CTRL02_XSG_AN_1G_Pause_MASK, SDS_CTRL02_XSG_AN_1G_AsymmetricPause_MASK | SDS_CTRL02_XSG_AN_1G_Pause_MASK);
		break;
		case SDS_10GBASER:
			sds_modify(sds, SDS_PAGE_CTRL1F, SDS_REG_CTRL1F_10GR_AN, SDS_CTRL1F_10GR_AN_AsymmetricPause_MASK | SDS_CTRL1F_10GR_AN_Pause_MASK, SDS_CTRL1F_10GR_AN_AsymmetricPause_MASK | SDS_CTRL1F_10GR_AN_Pause_MASK);
			break;
		default:
			break;
	}

	// Auto Negotiation
	switch (interface) {
		case SDS_SGMII:
		case SDS_2G5_SGMII:
		case SDS_1000BASEX:
		case SDS_2500BASEX:
			sds_modify(sds, SDS_PAGE_CTRL00, SDS_REG_CTRL00_REG02, SDS_CTRL00_REG02_XSG_SP_SDS_FRC_AN | SDS_CTRL00_REG02_XSG_SP_SDS_FRC_AN_EN, en_inband ? SDS_CTRL00_REG02_XSG_SP_SDS_FRC_AN | SDS_CTRL00_REG02_XSG_SP_SDS_FRC_AN_EN : SDS_CTRL00_REG02_XSG_SP_SDS_FRC_AN);
			sds_modify(sds, SDS_PAGE_CTRL00, SDS_REG_CTRL00_REG04, BIT(2), BIT(2));
			break;
		case SDS_10G_QXGMII:
		case SDS_10G_USXGMII:
			sds_modify(sds, SDS_PAGE_NWAY_AN, SDS_REG_NWAY_AN, 0xF, en_inband ? 0xF : 0);
			break;
		default:
			break;
	}

	serdes_on(sds);

	sds_modify(sds, SDS_PAGE_CTRL1F, SDS_REG_CTRL1F_REG00, 0xFFFF, 0xb);

	// delay 50mS
	delay(10);

	sds_modify(sds, SDS_PAGE_CTRL1F, SDS_REG_CTRL1F_REG00, 0xFFFF, 0x0);

	// delay 50mS
	delay(10);

	switch (interface) {
    	case SDS_10G_USXGMII:
		case SDS_10GBASER:
			serdes_reset_r(sds);
			break;
		default:
			serdes_reset_x(sds);
			break;
	}
}

/*
 * Configure the PHY-Side of the SDS-SDS link between SoC and PHY
 */
void static sds_init(void)
{
	phy_read(0, PHY_MMD30, 0xd);
	uint16_t pval = SFR_DATA_U16;

	// PHY Initialization:
	REG_WRITE(0x2f8, 0, 0, pval >> 8, pval);
	// Delay 20mS
	delay(4);

	pval &= 0xfff0;
	pval |= 0x0a;
	REG_WRITE(0x2f4, 0, 0, pval >> 8, pval);
	// Delay 10mS
	delay(2);

	phy_write_mask(0x1, PHY_MMD30, 0xd, pval);

	phy_read(0, PHY_MMD30, 0xd);
	pval = SFR_DATA_U16;

	REG_WRITE(0x2f8, 0, 0, pval >> 8, pval);

	pval &= 0xfff0;
	REG_WRITE(0x2f4, 0, 0, pval >> 8, pval);

	phy_write_mask(0x1, PHY_MMD30, 0xd, pval);

	if (machine_detected.isN) {
		uint16_t pval;

		print_string("  N-settings");
		// Serdes 0 RX PN swap for 64B/66B
		sds_read(1, 6, 2);
		pval = SFR_DATA_U16;
		sds_write_v(1, 6, 2, pval | 0x2000);

		// Serdes 1 RX PN swap for 8B/10B
		sds_read(1, 0, 0);
		pval = SFR_DATA_U16;
		sds_write_v(1, 0, 0, pval | 0x200);

		// Serdes 0 RX PN swap for 64B/66B
		sds_read(0, 6, 2);
		pval = SFR_DATA_U16;
		sds_write_v(0, 6, 2, pval | 0x2000);

		for (uint8_t sds = 0; sds < 2; sds++) {
			enum sds_type usage = machine.sds_settings[sds].usage;
			if (usage == SDS_EPHY) {
				switch (machine.sds_settings[sds].sds_settings_t.ephy.type) {
					case RTL8224:
						// RTL8224: Serdes 0 RX PN swap for 64B/66B
						// We assume that RTL8373N always paired with RTL8224N.
						// This sds register value is 0x0000 at reset.
						// So only write to it.
						RTL8224_SDS_WRITE(0, 6, 2, 0x2000);
						break;
					default:
						break;

				}
			}
		}
	}

	print_string("\nsds_init done\n");
}


void static tm_init(void)
{
	reg_read_m(RTL837X_TM_CTRL2);
	sfr_mask_data(0, TM_CTRL2_EN_LATCH, 0);
	reg_write_m(RTL837X_TM_CTRL2);
}


void rtl8373_init(void) __banked
{
	tm_init();

	print_string("\nrtl837x_init called\n");

	// r65d8:3ffbedff R65d8-3ffbedff

	if (machine.isRTL8373)
		// LED_GLB_ACTIVE
		reg_bit_set(0x65d8, 0x1d);

	sds_init();
	// Disable all SERDES for configuration
	REG_SET(RTL837X_REG_SDS_MODES, 0x000037ff);

	// r0a90:000000f3 R0a90-000000fc
	reg_read_m(RTL837X_CFG_PHY_MDI_REVERSE);
	sfr_mask_data(0, 0x0f, 0x0c);
	reg_write_m(RTL837X_CFG_PHY_MDI_REVERSE);

	if (machine_detected.isN) {
		print_string("  TX_POLARITY_SWAP\n");
		// FOR N-Version: #TX_POLARITY_SWAP
		reg_read_m(RTL837X_CFG_PHY_TX_POLARITY_SWAP);
			sfr_data[2] = 0x59;
			sfr_data[3] = 0x6a;
		reg_write_m(RTL837X_CFG_PHY_TX_POLARITY_SWAP);
	}

	for (uint8_t sds = 0; sds < 2; sds++) {
		enum sds_type usage = machine.sds_settings[sds].usage;
		switch (usage) {
			case SDS_EPHY:
				uint8_t mac = sds ? MAC_SDS1 : MAC_SDS0;
				uint8_t phy_type = machine.sds_settings[sds].sds_settings_t.ephy.type;
				// uint8_t speed = get_phy_max_speed(phy_type);
				switch (phy_type) {
					case RTL8224:
						phy_config_8224();
						sds_pcs_config(sds, SDS_10G_QXGMII, true);
						rtl8224_phy_enable();
						break;
					case RTL8261BE:
						phy_config_8261(mac, sds);
						sds_pcs_config(sds, SDS_10G_USXGMII, true);
						break;
					case RTL8221B:
						phy_config(mac);	// PHY configuration: External 8221B?
						// Allways start with SGMII.
						// Link-change will switch over to SDS_2G5_SGMII when 2.5G-link is needed.
						sds_pcs_config(sds, SDS_SGMII, true);
						break;
					default:
						print_string("\tERROR: Unknown phy_type\n");
				}
				break;
			case SDS_FIXED_LINK:
				enum sds_modes mode = machine.sds_settings[sds].sds_settings_t.fl.mode;
				sds_pcs_config(sds, mode, true);
				break;
			case SDS_SFP:
			default:
				sds_pcs_config(sds, SDS_OFF, false);
				break;
		}
	}

	// Set the internal PHY MDI settings
	reg_read_m(RTL837X_CFG_PHY_MDI_REVERSE);
	sfr_mask_data(0, 0x0f, MDI_CFG_VALUE(machine.mdi_cfg));
	reg_write_m(RTL837X_CFG_PHY_MDI_REVERSE);

	// Disable PHYs for configuration
	phy_write_mask(0xFF, PHY_MMD31, 0xa610, 0x2858);

	// Set bits 0x13 and 0x14 of 0x5fd4
	// r5fd4:0002914a R5fd4-001a914a
	reg_bit_set(0x5fd4, 0x13);
	reg_bit_set(0x5fd4, 0x14);

	// Configure ports MACHINE_MIN_PORT-8:
	//
	// r1538:00000e33 R1538-00000e37 r1538:00000e37 R1538-00000e37 r1538:00000e37 R1538-00000f37
	// [...]
	///
	uint16_t reg = 0x1238; // Port base register for the bits we set
	for (char i = 0; i <= MACHINE_MAX_PORT; i++) {
		if (machine.log_to_phys_port[i] != NOP)
			// Bit 6 (0x40) enables replacement of the RTL-VLAN tag with an 802.1Q VLAN tag
			REG_SET(reg, 0xf77);
		reg += 0x100;
	}
	REG_SET(reg, 0xf77);

	// r0b7c:000000d8 R0b7c-000000f8 r6040:00000030 R6040-00000031
	reg_bit_set(0xb7c, 5);

	// R7124-00001050 R7128-00001050 R712c-00001050 R7130-00001050 R7134-00001050 R7138-00001050
	// R713c-00001050 R7140-00001050 R7144-00001050 R7148-00001050

	// FlowControll and Backpressure, for all ports and CPU_PORT
	reg = 0x7124;
	for (char i = 0; i <= CPU_LOGICAL_PORT; i++) {
		if (machine.log_to_phys_port[i] != NOP)
			REG_SET(reg, 0x1050);
		reg += 4;
	}

	reg_bit_set(RTL837X_REG_HW_CONF, 0);

	// enable EEE for all ports at 2.5G and 10G, but don't reset the PHYs
	port_eee_enable_all(EEE_10G | EEE_2G5 | EEE_NORESET);
	
	// TODO: patch the PHYs

	// Re-enable PHY after configuration
	uint16_t phy_mask = 0x00F0;
	if (machine.isRTL8373)
		phy_mask |= 0xFF;
	else if (machine.sds_settings[0].usage == SDS_EPHY)
		phy_mask |= 0x08;
	if (machine.sds_settings[1].usage == SDS_EPHY)
		phy_mask |= 0x100;
	phy_write_mask(phy_mask, PHY_MMD31, 0xa610, 0x2058);

	// Enables MAC access, SMI_GLB_CTRL
	// SMI_POLLING bit 12..21 needed to be set when the MAC is used even without a PHY.
	// Otherwise, SFP-link is not detected/get a link.
	reg_read_m(SMI_GLB_CTRL);
	uint8_t set = 0x00;
		if (machine.isRTL8373)
		set = 0xF0; // mac 0-3
	else if (machine.log_to_phys_port[MAC_SDS0] != NOP)
		set = 0x80; // mac 3
	sfr_mask_data(1, 0xF0, set);
	set = 0x0f; // default mac 4-7
	if (machine.log_to_phys_port[MAC_SDS1] != NOP)
		set |= 0x10; // mac 8
	sfr_mask_data(2, 0x1f, set);
	reg_write_m(SMI_GLB_CTRL);

	print_string("\nrtl837x_init done\n");
}