

// RTL8272 

// > phyget 5 01 2
// PHYGET: 5:1.0x0002: VAL: 0x001c

// > phyget 5 01 3
// PHYGET: 5:1.0x0003: VAL: 0xcad0


int8_t rtl8224_config_init (void) __banked
{
    return -1;
}

static int rtl822x_c45_config_aneg(struct phy_device *phydev)
{
	bool changed = false;
	int ret, val;

	if (phydev->autoneg == AUTONEG_DISABLE)
		return genphy_c45_pma_setup_forced(phydev);

	ret = genphy_c45_an_config_aneg(phydev);
	if (ret < 0)
		return ret;
	if (ret > 0)
		changed = true;

	val = linkmode_adv_to_mii_ctrl1000_t(phydev->advertising);

	/* Vendor register as C45 has no standardized support for 1000BaseT */
	ret = phy_modify_mmd_changed(phydev, MDIO_MMD_VEND2,
				     RTL822X_VND2_C22_REG(MII_CTRL1000),
				     ADVERTISE_1000FULL, val);
	if (ret < 0)
		return ret;
	if (ret > 0)
		changed = true;

	return genphy_c45_check_and_restart_aneg(phydev, changed);
}
