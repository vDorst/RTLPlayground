#ifndef _RTL837X_I2C_H_
#define _RTL837X_I2C_H_

#include <stdint.h>
#include <stdbool.h>

bool i2c_read(uint8_t slot, uint8_t dev, uint8_t reg, uint8_t len) __banked __reentrant;

#endif // _RTL837X_I2C_H_
