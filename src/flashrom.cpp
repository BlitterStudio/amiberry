/*
* UAE - The Un*x Amiga Emulator
*
* Simple 29Fxxx flash ROM chip emulator
* I2C EEPROM (24C08)
* MICROWIRE EEPROM (9346)
*
* (c) 2014 Toni Wilen
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "zfile.h"
#include "flashrom.h"
#include "memory.h"
#include "newcpu.h"
#include "gui.h"
#include "uae.h"

#define FLASH_LOG 0
#define EEPROM_LOG 0

/* I2C EEPROM */

#define NVRAM_PAGE_SIZE 16

typedef enum bitbang_i2c_state {
    STOPPED = 0,
    SENDING_BIT7,
    SENDING_BIT6,
    SENDING_BIT5,
    SENDING_BIT4,
    SENDING_BIT3,
    SENDING_BIT2,
    SENDING_BIT1,
    SENDING_BIT0,
    WAITING_FOR_ACK,
    RECEIVING_BIT7,
    RECEIVING_BIT6,
    RECEIVING_BIT5,
    RECEIVING_BIT4,
    RECEIVING_BIT3,
    RECEIVING_BIT2,
    RECEIVING_BIT1,
    RECEIVING_BIT0,
    SENDING_ACK,
    SENT_NACK
} bitbang_i2c_state;

typedef enum eeprom_state {
	I2C_DEVICEADDR,
	I2C_WORDADDR,
	I2C_DATA
} eeprom_state;

struct bitbang_i2c_interface {
    bitbang_i2c_state state;
    int last_data;
    int last_clock;
    int device_out;
    uint8_t buffer;
    int current_addr;

	eeprom_state estate;
	int eeprom_addr;
	int size;
	int write_offset;
	int addressbitmask;
	uae_u8 *memory;
	struct zfile *zf;
};

static void nvram_write (struct bitbang_i2c_interface *i2c, int offset, int len)
{
	if (i2c->zf) {
		zfile_fseek(i2c->zf, offset, SEEK_SET);
		zfile_fwrite(i2c->memory + offset, len, 1, i2c->zf);
	}
}

static void bitbang_i2c_enter_stop(bitbang_i2c_interface *i2c)
{
#if EEPROM_LOG
    write_log(_T("STOP\n"));
#endif
	if (i2c->write_offset >= 0)
		nvram_write(i2c, i2c->write_offset, 16);
	i2c->write_offset = -1;
    i2c->current_addr = -1;
    i2c->state = STOPPED;
	i2c->estate = I2C_DEVICEADDR;
}

/* Set device data pin.  */
static int bitbang_i2c_ret(bitbang_i2c_interface *i2c, int level)
{
    i2c->device_out = level;
    //DPRINTF("%d %d %d\n", i2c->last_clock, i2c->last_data, i2c->device_out);
    return level & i2c->last_data;
}

/* Leave device data pin unodified.  */
static int bitbang_i2c_nop(bitbang_i2c_interface *i2c)
{
    return bitbang_i2c_ret(i2c, i2c->device_out);
}

/* Returns data line level.  */
int eeprom_i2c_set(void *fdv, int line, int level)
{
	struct bitbang_i2c_interface *i2c = (bitbang_i2c_interface*)fdv;
    int data;

    if (line == BITBANG_I2C_SDA) {
		if (level < 0)
			level = i2c->last_data;
        if (level == i2c->last_data) {
            return bitbang_i2c_nop(i2c);
        }
        i2c->last_data = level;
        if (i2c->last_clock == 0) {
            return bitbang_i2c_nop(i2c);
        }
        if (level == 0) {
#if EEPROM_LOG
            write_log(_T("START\n"));
#endif
			/* START condition.  */
            i2c->state = SENDING_BIT7;
            i2c->current_addr = -1;
        } else {
            /* STOP condition.  */
            bitbang_i2c_enter_stop(i2c);
        }
        return bitbang_i2c_ret(i2c, 1);
    } else {
		if (level < 0)
			level = i2c->last_clock;
	}

    data = i2c->last_data;
    if (i2c->last_clock == level) {
        return bitbang_i2c_nop(i2c);
    }
    i2c->last_clock = level;
    if (level == 0) {
        /* State is set/read at the start of the clock pulse.
           release the data line at the end.  */
        return bitbang_i2c_ret(i2c, 1);
    }
    switch (i2c->state) {
    case STOPPED:
    case SENT_NACK:
        return bitbang_i2c_ret(i2c, 1);

	// Writing to EEPROM
	case SENDING_BIT7:
	case SENDING_BIT6:
	case SENDING_BIT5:
	case SENDING_BIT4:
	case SENDING_BIT3:
	case SENDING_BIT2:
	case SENDING_BIT1:
	case SENDING_BIT0:
        i2c->buffer = (i2c->buffer << 1) | data;
        /* will end up in WAITING_FOR_ACK */
        i2c->state = (bitbang_i2c_state)((int)i2c->state + 1);
        return bitbang_i2c_ret(i2c, 1);

    case WAITING_FOR_ACK:
		if (i2c->estate == I2C_DEVICEADDR) {
            i2c->current_addr = i2c->buffer;
#if EEPROM_LOG
			write_log(_T("Device address 0x%02x\n"), i2c->current_addr);
#endif
			if ((i2c->current_addr & 0xf0) != 0xa0) {
				write_log (_T("WARNING: I2C_DEVICEADDR: device address != 0xA0\n"));
				i2c->state = STOPPED;
				return bitbang_i2c_ret(i2c, 0);
			}
			if (i2c->current_addr & 1) {
				i2c->estate = I2C_DATA;
			} else {
				i2c->estate = I2C_WORDADDR;
				i2c->eeprom_addr = ((i2c->buffer >> 1) & i2c->addressbitmask) << 8;
			}
		} else if (i2c->estate == I2C_WORDADDR) {
			i2c->estate = I2C_DATA;
			i2c->eeprom_addr &= i2c->addressbitmask << 8;
			i2c->eeprom_addr |= i2c->buffer;
#if EEPROM_LOG
			write_log(_T("EEPROM address %04x\n"), i2c->eeprom_addr);
#endif
		} else if (!(i2c->current_addr & 1)) {
#if EEPROM_LOG
            write_log(_T("Sent %04x 0x%02x\n"), i2c->eeprom_addr, i2c->buffer);
#endif
			if (i2c->write_offset < 0)
				i2c->write_offset = i2c->eeprom_addr;
			i2c->memory[i2c->eeprom_addr] = i2c->buffer;
			i2c->eeprom_addr = (i2c->eeprom_addr & ~(NVRAM_PAGE_SIZE - 1)) | (i2c->eeprom_addr + 1) & (NVRAM_PAGE_SIZE - 1);
			gui_flicker_led (LED_MD, 0, 2);
        }
        if (i2c->current_addr & 1) {
            i2c->state = RECEIVING_BIT7;
        } else {
            i2c->state = SENDING_BIT7;
        }
        return bitbang_i2c_ret(i2c, 0);

	// Reading from EEPROM
    case RECEIVING_BIT7:
        i2c->buffer = i2c->memory[i2c->eeprom_addr];
		//i2c->buffer = i2c_recv(i2c->bus);
#if EEPROM_LOG
        write_log(_T("RX byte %04X 0x%02x\n"), i2c->eeprom_addr, i2c->buffer);
#endif
		i2c->eeprom_addr++;
		i2c->eeprom_addr &= i2c->size - 1;
		gui_flicker_led (LED_MD, 0, 1);
        /* Fall through... */
    case RECEIVING_BIT6:
    case RECEIVING_BIT5:
    case RECEIVING_BIT4:
    case RECEIVING_BIT3:
    case RECEIVING_BIT2:
    case RECEIVING_BIT1:
    case RECEIVING_BIT0:
        data = i2c->buffer >> 7;
        /* will end up in SENDING_ACK */
        i2c->state = (bitbang_i2c_state)((int)i2c->state + 1);
        i2c->buffer <<= 1;
        return bitbang_i2c_ret(i2c, data);

    case SENDING_ACK:
        i2c->state = RECEIVING_BIT7;
        if (data != 0) {
#if EEPROM_LOG > 1
			write_log(_T("NACKED\n"));
#endif
			i2c->state = SENT_NACK;
            //i2c_nack(i2c->bus);
        } else {
			;
#if EEPROM_LOG > 1
            write_log(_T("ACKED\n"));
#endif
		}
        return bitbang_i2c_ret(i2c, 1);
    }
    target_startup_msg(_T("Internal error"), _T("eeprom_i2c_set: Unhandled case."));
    uae_restart(1, NULL);
}

void eeprom_reset(void *fdv)
{
	struct bitbang_i2c_interface *i2c = (bitbang_i2c_interface*)fdv;
	if (!i2c)
		return;
	i2c->last_data = 1;
	i2c->last_clock = 1;
	i2c->device_out = 1;
	i2c->eeprom_addr = 0;
	i2c->write_offset = -1;
	i2c->estate = I2C_DEVICEADDR;
}

void *eeprom_new(uae_u8 *memory, int size, struct zfile *zf)
{
    bitbang_i2c_interface *s;

    s = xcalloc(bitbang_i2c_interface, 1);

	eeprom_reset(s);

	s->memory = memory;
	s->size = size;
	s->zf = zf;
	s->addressbitmask = (size / 256) - 1;

    return s;
}

void eeprom_free(void *fdv)
{
	struct bitbang_i2c_interface *i2c = (bitbang_i2c_interface*)fdv;
	xfree(i2c);
}

/* FLASH */

struct flashrom_data
{
	uae_u8 *rom;
	int flashsize;
	int allocsize;
	int mask;
	int state;
	int modified;
	int sectorsize;
	uae_u8 devicecode;
	int flags;
	struct zfile *zf;
};

void *flash_new(uae_u8 *rom, int flashsize, int allocsize, uae_u8 devicecode, struct zfile *zf, int flags)
{
	struct flashrom_data *fd = xcalloc(struct flashrom_data, 1);
	fd->flashsize = flashsize;
	fd->allocsize = allocsize;
	fd->mask = fd->flashsize - 1;
	fd->zf = zf;
	fd->rom = rom;
	fd->flags = flags;
	fd->devicecode = devicecode;
	fd->sectorsize = devicecode == 0x20 ? 16384 : 65536;
	return fd;
}

void flash_free(void *fdv)
{
	struct flashrom_data *fd = (struct flashrom_data*)fdv;
	if (!fd)
		return;
	if (fd->zf && fd->modified) {
		zfile_fseek(fd->zf, 0, SEEK_SET);
		if (fd->flags & FLASHROM_EVERY_OTHER_BYTE) {
			zfile_fseek(fd->zf, (fd->flags & FLASHROM_EVERY_OTHER_BYTE_ODD) ? 1 : 0, SEEK_SET);
			for (int i = 0; i < fd->allocsize; i++) {
				zfile_fwrite(&fd->rom[i * 2], 1, 1, fd->zf);
				zfile_fseek(fd->zf, 1, SEEK_CUR);
			}
		} else {
		  zfile_fwrite(fd->rom, fd->allocsize, 1, fd->zf);
	  }
	}
	xfree(fdv);
}

int flash_size(void *fdv)
{
	struct flashrom_data *fd = (struct flashrom_data*)fdv;
	if (!fd)
		return 0;
	return fd->flashsize;
}

bool flash_active(void *fdv, uaecptr addr)
{
	struct flashrom_data *fd = (struct flashrom_data*)fdv;
	if (!fd)
		return false;
	return fd->state != 0;
}

bool flash_write(void *fdv, uaecptr addr, uae_u8 v)
{
	struct flashrom_data *fd = (struct flashrom_data*)fdv;
	int oldstate;
	uae_u32 addr2;
	int other_byte_mult = 1;

	if (!fd)
		return false;

	if (fd->flags & FLASHROM_EVERY_OTHER_BYTE) {
		addr >>= 1;
		other_byte_mult = 2;
	}

	oldstate = fd->state;

#if FLASH_LOG > 1
	write_log(_T("flash write %08x %02x (%d) PC=%08x\n"), addr, v, fd->state, m68k_getpc());
#endif

	addr &= fd->mask;
	addr2 = addr & 0xffff;

	if (fd->state == 7) {
		if (!(fd->flags & FLASHROM_PARALLEL_EEPROM)) {
		  fd->state = 100;
		} else {
			fd->state++;
			if (fd->state >= 7 + 64)
				fd->state = 100;
		}
		if (addr >= fd->allocsize)
			return false;
		if (fd->rom[addr * other_byte_mult] != v)
			fd->modified = 1;
		fd->rom[addr * other_byte_mult] = v;
		gui_flicker_led (LED_MD, 0, 2);
		return true;
	}

	if (v == 0xf0) {
		fd->state = 0;
		return false;
	}

	// unlock
	if (addr2 == 0x5555 && fd->state <= 2 && v == 0xaa)
		fd->state = 1;
	if (addr2 == 0x2aaa && fd->state == 1 && v == 0x55)
		fd->state = 2;

	// autoselect
	if (addr2 == 0x5555 && fd->state == 2 && v == 0x90)
		fd->state = 3;

	// program
	if (addr2 == 0x5555 && fd->state == 2 && v == 0xa0)
		fd->state = 7;

	// chip/sector erase
	if (addr2 == 0x5555 && fd->state == 2 && v == 0x80)
		fd->state = 4;
	if (addr2 == 0x5555 && fd->state == 4 && v == 0xaa)
		fd->state = 5;
	if (addr2 == 0x2aaa && fd->state == 5 && v == 0x55)
		fd->state = 6;
	if (addr2 == 0x5555 && fd->state == 6 && v == 0x10) {
		for (int i = 0; i < fd->allocsize; i++) {
			fd->rom[i * other_byte_mult] = 0xff;
		}
		fd->state = 200;
		fd->modified = 1;
#if FLASH_LOG
		write_log(_T("flash chip erased\n"), addr);
#endif
		gui_flicker_led (LED_MD, 0, 2);
		return true;
	} else if (fd->state == 6 && v == 0x30) {
		int saddr = addr & ~(fd->sectorsize - 1);
		if (saddr < fd->allocsize) {
			for (int i = 0; i < fd->sectorsize; i++) {
				fd->rom[(saddr + i) * other_byte_mult] = 0xff;
			}
		}
		fd->state = 200;
		fd->modified = 1;
#if FLASH_LOG
		write_log(_T("flash sector %d erased (%08x)\n"), saddr / fd->sectorsize, addr);
#endif
		gui_flicker_led (LED_MD, 0, 2);
		return true;
	}

	if (fd->state == oldstate)
		fd->state = 0;
	return false;
}

uae_u32 flash_read(void *fdv, uaecptr addr)
{
	struct flashrom_data *fd = (struct flashrom_data*)fdv;
	uae_u8 v = 0xff;
	int other_byte_mult = 1;
#if FLASH_LOG > 1
	uaecptr oaddr = addr;
#endif

	if (!fd)
		return 0;

	if (fd->flags & FLASHROM_EVERY_OTHER_BYTE) {
		addr >>= 1;
		other_byte_mult = 2;
	}

	addr &= fd->mask;
	if (fd->state == 3) {
		uae_u8 a = addr & 0xff;
		if (a == 0)
			v = 0x01;
		if (a == 1)
			v = fd->devicecode;
		if (a == 2)
			v = 0x00;
		gui_flicker_led (LED_MD, 0, 1);
	} else if (fd->state >= 200) {
		v = 0;
		if (fd->state & 1)
			v ^= 0x40;
		fd->state++;
		if (fd->state >= 210)
			fd->state = 0;
		v |= 0x08;
	} else if (fd->state > 7) {
		v = (fd->rom[addr * other_byte_mult] & 0x80) ^ 0x80;
		if (fd->state & 1)
			v ^= 0x40;
		fd->state++;
		if (fd->state >= 110)
			fd->state = 0;
	} else {
		fd->state = 0;
		if (addr >= fd->allocsize)
			v = 0xff;
		else
			v = fd->rom[addr * other_byte_mult];
	}
#if FLASH_LOG > 1
	write_log(_T("flash read %08x = %02X (%d) PC=%08x\n"), oaddr, v, fd->state, m68k_getpc());
#endif

	return v;
}
