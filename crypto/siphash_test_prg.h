#ifndef _RTL837X_STDIO_H_
#define _RTL837X_STDIO_H_

#include <stdint.h>

// Headers for calls in the common code area (HOME/BANK0)
void print_string(__code char *p);
void print_long(__xdata uint32_t a);
void print_short(uint16_t a);
void print_byte(uint8_t a);
void print_sfr_data(void);
void print_phy_data(void);
void phy_write_mask(uint16_t phy_mask, uint8_t dev_id, uint16_t reg, uint16_t v);
void phy_write(uint8_t phy_id, uint8_t dev_id, uint16_t reg, uint16_t v);
void phy_read(uint8_t phy_id, uint8_t dev_id, uint16_t reg);
void phy_modify(uint8_t phy_id, uint8_t dev_id, uint16_t reg, uint16_t mask, uint16_t set);
void reg_read(uint16_t reg_addr);
void reg_read_m(uint16_t reg_addr);
void reg_write(uint16_t reg_addr);
void reg_write_m(uint16_t reg_addr);
void delay(uint16_t t);
void sleep(uint16_t t);
void write_char(char c);
void print_reg(uint16_t reg);
uint8_t sfp_read_reg(uint8_t slot, uint8_t reg);
void reg_bit_set(uint16_t reg_addr, char bit);
void reg_bit_clear(uint16_t reg_addr, char bit);
void sfr_set_zero(void);
void memcpy(__xdata void * __xdata dst, __xdata const void * __xdata src, uint16_t len);
void memcpyc(register __xdata uint8_t *dst, register __code uint8_t *src, register uint16_t len);
void memset(register __xdata uint8_t *dst, register __xdata uint8_t v, register uint8_t len);
uint16_t strlen(register __code const char *s);
uint16_t strlen_x(register __xdata const char *s);
uint16_t strtox(register __xdata uint8_t *dst, register __code const char *s);
void print_string_x(__xdata char *p);

#endif
