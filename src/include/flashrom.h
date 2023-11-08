#ifndef UAE_FLASHROM_H
#define UAE_FLASHROM_H

#include "uae/types.h"

/* FLASH */

void *flash_new(uae_u8 *rom, int flashsize, int allocsize, uae_u8 mfgcode, uae_u8 devcode, struct zfile *zf, int flags);
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
#define FLASHROM_SKIP_EVERY_OTHER_BYTE 8
#define FLASHROM_DATA_PROTECT 16

void *i2c_new(uae_u8 device_address, int size, uae_u8(*read_func)(uae_u8 addr), void(*write_func)(uae_u8 addr, uae_u8 v));
void i2c_free(void *i2c);
int i2c_set(void *i2c, int line, int level);
void i2c_reset(void *i2c);

/* MICROWIRE EEPROM */

void eeprom93xx_write(void *eepromp, int eecs, int eesk, int eedi);
uae_u16 eeprom93xx_read(void *eepromp);
void *eeprom93xx_new(const uae_u8 *memory, int nwords, struct zfile *zf);
void eeprom93xx_free(void *eepromp);
uae_u8 eeprom93xx_read_byte(void *eepromp, int offset);

#endif /* UAE_FLASHROM_H */
