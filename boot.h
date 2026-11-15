#ifndef BOOT_H
#define BOOT_H

void early_boot_handle_button(void) __banked;
void rtl8224_enable(void) __banked;
void nic_setup(void) __banked;
void rtl8373_revision(void) __banked;
void init_smi(void) __banked;
void setup_i2c(void) __banked;
void set_hostname_default(void) __banked;

#endif
