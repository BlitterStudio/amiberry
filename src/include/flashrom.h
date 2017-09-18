#ifndef UAE_FLASHROM_H
#define UAE_FLASHROM_H

#include "uae/types.h"

/* FLASH */

void *flash_new(uae_u8 *rom, int flashsize, int allocsize, uae_u8 devicecode, struct zfile *zf, int flags);
void flash_free(void *fdv);

bool flash_write(void *fdv, uaecptr addr, uae_u8 v);
uae_u32 flash_read(void *fdv, uaecptr addr);
bool flash_active(void *fdv, uaecptr addr);
int flash_size(void *fdv);

/* I2C EEPROM */

#define BITBANG_I2C_SDA 0
#define BITBANG_I2C_SCL 1

void *eeprom_new(uae_u8 *rom, int size, struct zfile *zf);
void eeprom_free(void *i2c);
void eeprom_reset(void *i2c);
int eeprom_i2c_set(void *i2c, int line, int level);

#define FLASHROM_EVERY_OTHER_BYTE 1
#define FLASHROM_EVERY_OTHER_BYTE_ODD 2
#define FLASHROM_PARALLEL_EEPROM 4

#endif /* UAE_FLASHROM_H */
