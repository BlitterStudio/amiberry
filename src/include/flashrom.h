#ifndef UAE_FLASHROM_H
#define UAE_FLASHROM_H

#include "uae/types.h"

/* I2C EEPROM */

#define BITBANG_I2C_SDA 0
#define BITBANG_I2C_SCL 1

void *eeprom_new(uae_u8 *rom, int size, struct zfile *zf);
void eeprom_free(void *i2c);
void eeprom_reset(void *i2c);
int eeprom_i2c_set(void *i2c, int line, int level);

#endif /* UAE_FLASHROM_H */
