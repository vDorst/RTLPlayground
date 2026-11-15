#ifndef _RTL837X_INIT_H_
#define _RTL837X_INIT_H_

#include <stdbool.h>

void rtl8372_init(void) __banked;
void rtl8373_init(void) __banked;

void sds_pcs_config(uint8_t sds, enum sds_modes interface, bool en_inband) __banked;

#endif
