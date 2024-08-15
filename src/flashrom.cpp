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
#include "debug.h"
#include "gui.h"

#define FLASH_LOG 0
#define EEPROM_LOG 0

/* MICROWIRE EEPROM */

struct eeprom93xx_eeprom_t {
	uint8_t  tick;
	uint8_t  address;
	uint8_t  command;
	uint8_t  writeable;

	uint8_t eecs;
	uint8_t eesk;
	uint8_t eedo;

	uint8_t  addrbits;
	uint16_t size;
	uint16_t data;
	uint16_t contents[256];

	uae_u8 *memory;
	struct zfile *zf;
};

static const char *opstring[] = { "extended", "write", "read", "erase" };

void eeprom93xx_write(void *eepromp, int eecs, int eesk, int eedi)
{
	eeprom93xx_eeprom_t *eeprom = (eeprom93xx_eeprom_t*)eepromp;
	uint8_t tick = eeprom->tick;
	uint8_t eedo = eeprom->eedo;
	uint16_t address = eeprom->address;
	uint8_t command = eeprom->command;

#if EEPROM_LOG
	write_log("CS=%u SK=%u DI=%u DO=%u, tick = %u\n", eecs, eesk, eedi, eedo, tick);
#endif

	if (!eeprom->eecs && eecs) {
		/* Start chip select cycle. */
#if EEPROM_LOG
		write_log("Cycle start, waiting for 1st start bit (0)\n");
#endif
		tick = 0;
		command = 0x0;
		address = 0x0;
	}
	else if (eeprom->eecs && !eecs) {
		/* End chip select cycle. This triggers write / erase. */
		if (eeprom->writeable) {
			uint8_t subcommand = address >> (eeprom->addrbits - 2);
			if (command == 0 && subcommand == 2) {
				/* Erase all. */
				for (address = 0; address < eeprom->size; address++) {
					eeprom->contents[address] = 0xffff;
				}
			}
			else if (command == 3) {
				/* Erase word. */
				eeprom->contents[address] = 0xffff;
			}
			else if (tick >= 2 + 2 + eeprom->addrbits + 16) {
				if (command == 1) {
					/* Write word. */
					eeprom->contents[address] &= eeprom->data;
				}
				else if (command == 0 && subcommand == 1) {
					/* Write all. */
					for (address = 0; address < eeprom->size; address++) {
						eeprom->contents[address] &= eeprom->data;
					}
				}
			}
		}
		/* Output DO is tristate, read results in 1. */
		eedo = 1;
	}
	else if (eecs && !eeprom->eesk && eesk) {
		/* Raising edge of clock shifts data in. */
		if (tick == 0) {
			/* Wait for 1st start bit. */
			if (eedi == 0) {
#if EEPROM_LOG
				write_log("Got correct 1st start bit, waiting for 2nd start bit (1)\n");
#endif
				tick++;
			}
			else {
#if EEPROM_LOG
				write_log("wrong 1st start bit (is 1, should be 0)\n");
#endif
				tick = 2;
				//~ assert(!"wrong start bit");
			}
		}
		else if (tick == 1) {
			/* Wait for 2nd start bit. */
			if (eedi != 0) {
#if EEPROM_LOG
				write_log("Got correct 2nd start bit, getting command + address\n");
#endif
				tick++;
			}
			else {
#if EEPROM_LOG

				write_log("1st start bit is longer than needed\n");
#endif
			}
		}
		else if (tick < 2 + 2) {
			/* Got 2 start bits, transfer 2 opcode bits. */
			tick++;
			command <<= 1;
			if (eedi) {
				command += 1;
			}
		}
		else if (tick < 2 + 2 + eeprom->addrbits) {
			/* Got 2 start bits and 2 opcode bits, transfer all address bits. */
			tick++;
			address = ((address << 1) | eedi);
			if (tick == 2 + 2 + eeprom->addrbits) {
#if EEPROM_LOG
				write_log("%s command, address = 0x%02x (value 0x%04x)\n", opstring[command], address, eeprom->contents[address]);
#endif
				if (command == 2) {
					eedo = 0;
				}
				address = address % eeprom->size;
				if (command == 0) {
					/* Command code in upper 2 bits of address. */
					switch (address >> (eeprom->addrbits - 2)) {
					case 0:
#if EEPROM_LOG
						write_log("write disable command\n");
#endif
						eeprom->writeable = 0;
						break;
					case 1:
#if EEPROM_LOG
						write_log("write all command\n");
#endif
						break;
					case 2:
#if EEPROM_LOG
						write_log("erase all command\n");
#endif
						break;
					case 3:
#if EEPROM_LOG
						write_log("write enable command\n");
#endif
						eeprom->writeable = 1;
						break;
					}
				}
				else {
					/* Read, write or erase word. */
					eeprom->data = eeprom->contents[address];
				}
			}
		}
		else if (tick < 2 + 2 + eeprom->addrbits + 16) {
			/* Transfer 16 data bits. */
			tick++;
			if (command == 2) {
				/* Read word. */
				eedo = ((eeprom->data & 0x8000) != 0);
			}
			eeprom->data <<= 1;
			eeprom->data += eedi;
		}
		else {
#if EEPROM_LOG
			write_log("additional unneeded tick, not processed\n");
#endif
		}
	}
	/* Save status of EEPROM. */
	eeprom->tick = tick;
	eeprom->eecs = eecs;
	eeprom->eesk = eesk;
	eeprom->eedo = eedo;
	eeprom->address = (uae_u8)address;
	eeprom->command = command;
}

uae_u16 eeprom93xx_read(void *eepromp)
{
	eeprom93xx_eeprom_t *eeprom = (eeprom93xx_eeprom_t*)eepromp;
	/* Return status of pin DO (0 or 1). */
#if EEPROM_LOG
	write_log("CS=%u DO=%u\n", eeprom->eecs, eeprom->eedo);
#endif
	return eeprom->eedo;
}

uae_u8 eeprom93xx_read_byte(void *eepromp, int offset)
{
	eeprom93xx_eeprom_t *eeprom = (eeprom93xx_eeprom_t*)eepromp;
	if (offset & 1)
		return (uae_u8)eeprom->contents[offset / 2];
	else
		return eeprom->contents[offset / 2] >> 8;
}

void *eeprom93xx_new(const uae_u8 *memory, int nwords, struct zfile *zf)
{
	/* Add a new EEPROM (with 16, 64 or 256 words). */
	eeprom93xx_eeprom_t *eeprom;
	uint8_t addrbits;

	switch (nwords) {
	case 16:
	case 64:
		addrbits = 6;
		break;
	case 128:
	case 256:
		addrbits = 8;
		break;
	default:
		return NULL;
	}

	eeprom = (eeprom93xx_eeprom_t *)xcalloc(eeprom93xx_eeprom_t, 1);
	if (eeprom) {
		eeprom->size = nwords;
		eeprom->addrbits = addrbits;
		for (int i = 0; i < nwords; i++) {
			eeprom->contents[i] = (memory[i * 2 + 0] << 8) | memory[i * 2 + 1];
		}
		/* Output DO is tristate, read results in 1. */
		eeprom->eedo = 1;
	//	write_log("eeprom = 0x%p, nwords = %u\n", eeprom, nwords);
	}
	return eeprom;
}

void eeprom93xx_free(void *eepromp)
{
	eeprom93xx_eeprom_t *eeprom = (eeprom93xx_eeprom_t*)eepromp;

	/* Destroy EEPROM. */
//	write_log("eeprom = 0x%p\n", eeprom);
	xfree(eeprom);
}

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
	uae_u8 device_address, device_address_mask;

	eeprom_state estate;
	int eeprom_addr;
	int size;
	int write_offset;
	int addressbitmask;
	uae_u8 *memory;
	struct zfile *zf;

	uae_u8(*read_func)(uae_u8 addr);
	void(*write_func)(uae_u8 addr, uae_u8 v);
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
    write_log(_T("I2C STOP\n"));
#endif
	if (i2c->write_offset >= 0) {
		int len = i2c->size - i2c->write_offset;
		if (len > 16) {
			len = 16;
		}
		if (len > 0) {
			nvram_write(i2c, i2c->write_offset, len);
		}
		if (len < 16) {
			nvram_write(i2c, 0, 16 - len);
		}
	}
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
            write_log(_T("I2C START\n"));
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
			write_log(_T("I2C device address 0x%02x\n"), i2c->current_addr);
#endif
			if ((i2c->current_addr & i2c->device_address_mask) != i2c->device_address) {
				write_log (_T("I2C WARNING: device address != %02x\n"), i2c->device_address);
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
			write_log(_T("I2C device address 0x%02x (Address %04x)\n"), i2c->buffer, i2c->eeprom_addr);
#endif
		} else if (!(i2c->current_addr & 1)) {
#if EEPROM_LOG
            write_log(_T("I2C sent %04x 0x%02x\n"), i2c->eeprom_addr, i2c->buffer);
#endif
			if (i2c->write_offset < 0)
				i2c->write_offset = i2c->eeprom_addr;
			if (i2c->write_func) {
				i2c->write_func(i2c->eeprom_addr, i2c->buffer);
			} else {
				i2c->memory[i2c->eeprom_addr] = i2c->buffer;
				i2c->eeprom_addr = (i2c->eeprom_addr & ~(NVRAM_PAGE_SIZE - 1)) | (i2c->eeprom_addr + 1) & (NVRAM_PAGE_SIZE - 1);
				gui_flicker_led(LED_MD, 0, 2);
			}
        }
        if (i2c->current_addr & 1) {
            i2c->state = RECEIVING_BIT7;
        } else {
            i2c->state = SENDING_BIT7;
        }
        return bitbang_i2c_ret(i2c, 0);

	// Reading from EEPROM
    case RECEIVING_BIT7:
		if (i2c->read_func) {
			i2c->buffer = i2c->read_func(i2c->eeprom_addr);
		} else {
			i2c->buffer = i2c->memory[i2c->eeprom_addr];
		}
		//i2c->buffer = i2c_recv(i2c->bus);
#if EEPROM_LOG
        write_log(_T("I2C RX byte %04X 0x%02x\n"), i2c->eeprom_addr, i2c->buffer);
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
			write_log(_T("I2C NACKED\n"));
#endif
			i2c->state = SENT_NACK;
            //i2c_nack(i2c->bus);
        } else {
			;
#if EEPROM_LOG > 1
            write_log(_T("I2C ACKED\n"));
#endif
		}
        return bitbang_i2c_ret(i2c, 1);
    }
    abort();
}

int i2c_set(void *i2c, int line, int level)
{
	return eeprom_i2c_set(i2c, line, level);
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
	if (s) {
		eeprom_reset(s);

		s->memory = memory;
		s->size = size;
		s->zf = zf;
		s->addressbitmask = (size / 256) - 1;
		s->device_address = 0xa0;
		s->device_address_mask = 0xf0;
	}
    return s;
}

void *i2c_new(uae_u8 device_address, int size, uae_u8 (*read_func)(uae_u8 addr), void (*write_func)(uae_u8 addr, uae_u8 v))
{
	bitbang_i2c_interface *s;

	s = xcalloc(bitbang_i2c_interface, 1);
	if (s) {
		eeprom_reset(s);

		s->memory = NULL;
		s->size = size;
		s->zf = NULL;
		s->addressbitmask = 0;
		s->device_address = 0xa2;
		s->device_address_mask = 0xff;

		s->read_func = read_func;
		s->write_func = write_func;
	}
	return s;
}

void eeprom_free(void *fdv)
{
	struct bitbang_i2c_interface *i2c = (bitbang_i2c_interface*)fdv;
	xfree(i2c);
}

void i2c_free(void *fdv)
{
	eeprom_free(fdv);
}

void i2c_reset(void *fdv)
{
	struct bitbang_i2c_interface *i2c = (bitbang_i2c_interface*)fdv;
	eeprom_reset(i2c);
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
	int pagesize;
	uae_u8 devicecode, mfgcode;
	int flags;
	int writeprot;
	int lastpagewrite;
	int firstwriteoffset;
	int lastwriteoffset;
	uae_u8 page[128];
	bool mpage[128];
	bool pagemodified;
	struct zfile *zf;
};

static void setmodified(struct flashrom_data *fd, int offset)
{
	fd->modified = 1;
	if (offset < fd->firstwriteoffset) {
		fd->firstwriteoffset = offset;
	}
	if (offset > fd->lastwriteoffset) {
		fd->lastwriteoffset = offset;
	}
}

void *flash_new(uae_u8 *rom, int flashsize, int allocsize, uae_u8 mfgcode, uae_u8 devcode, struct zfile *zf, int flags)
{
	struct flashrom_data *fd = xcalloc(struct flashrom_data, 1);
	if (fd) {
		fd->flashsize = flashsize;
		fd->allocsize = allocsize;
		fd->mask = fd->flashsize - 1;
		fd->zf = zf;
		fd->rom = rom;
		fd->flags = flags;
		fd->devicecode = devcode;
		fd->pagesize = mfgcode == 0xbf ? 128 : 64;
		fd->mfgcode = mfgcode;
		fd->sectorsize = devcode == 0x20 ? 16384 : 65536;
		fd->lastwriteoffset = 0;
		fd->firstwriteoffset = allocsize;
		fd->writeprot = (flags & FLASHROM_DATA_PROTECT) ? 1 : 0;
		fd->lastpagewrite = -1;
	}
	return fd;
}

void flash_free(void *fdv)
{
	struct flashrom_data *fd = (struct flashrom_data*)fdv;
	if (!fd)
		return;
	if (fd->zf && fd->modified) {
		if (fd->flags & FLASHROM_EVERY_OTHER_BYTE) {
			zfile_fseek(fd->zf, (fd->flags & FLASHROM_EVERY_OTHER_BYTE_ODD) ? 1 : 0, SEEK_SET);
			int last = fd->lastwriteoffset + 1;
			last += 511;
			last &= ~511;
			if (last > fd->allocsize) {
				last = fd->allocsize;
			}
			for (int i = 0; i < last; i++) {
				zfile_fwrite(&fd->rom[i * 2], 1, 1, fd->zf);
				zfile_fseek(fd->zf, 1, SEEK_CUR);
			}
		} else if (fd->flags & FLASHROM_SKIP_EVERY_OTHER_BYTE) {
			zfile_fseek(fd->zf, 0, SEEK_SET);
			int last = fd->lastwriteoffset + 1;
			last += 511;
			last &= ~511;
			if (last > fd->allocsize) {
				last = fd->allocsize;
			}
			for (int i = 0; i <  last / 2; i++) {
				zfile_fwrite(&fd->rom[i * 2], 1, 1, fd->zf);
			}
		} else {
			uae_s32 msize = zfile_ftell32(fd->zf);
			if (msize > fd->lastwriteoffset) {
				zfile_fseek(fd->zf, fd->firstwriteoffset, SEEK_SET);
				zfile_fwrite(fd->rom + fd->firstwriteoffset, fd->lastwriteoffset - fd->firstwriteoffset + 1, 1, fd->zf);
			} else {
				zfile_fseek(fd->zf, 0, SEEK_SET);
				zfile_fwrite(fd->rom, fd->lastwriteoffset + 1, 1, fd->zf);
			}
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

static void writeflash(struct flashrom_data *fd, int addr, uae_u8 v)
{
	if (!(fd->flags & FLASHROM_PARALLEL_EEPROM)) {
		fd->state = 100;
		if (fd->writeprot <= 0) {
			if (fd->rom[addr] != v) {
				fd->rom[addr] = v;
				fd->modified = 1;
			}
			if (fd->modified) {
				setmodified(fd, addr);
			}
			gui_flicker_led(LED_MD, 0, 2);
		}
	} else {
		int page = addr & ~(fd->pagesize - 1);
		int mpage = addr & (fd->pagesize - 1);
		if (fd->lastpagewrite != page) {
			memset(fd->mpage, 0, sizeof(fd->mpage));
			fd->pagemodified = false;
			fd->lastpagewrite = page;
		}
		if (fd->writeprot <= 0) {
			fd->page[mpage] = v;
			fd->mpage[mpage] = true;
			fd->pagemodified = true;
			gui_flicker_led(LED_MD, 0, 2);
		}
		fd->state = 7 + 1;
	}
}

bool flash_write(void *fdv, uaecptr addr, uae_u8 v)
{
	struct flashrom_data *fd = (struct flashrom_data*)fdv;
	int other_byte_mult = 1;

	if (!fd)
		return false;

	if (fd->flags & (FLASHROM_SKIP_EVERY_OTHER_BYTE | FLASHROM_EVERY_OTHER_BYTE)) {
		addr >>= 1;
		other_byte_mult = 2;
	}

	if (addr * other_byte_mult >= fd->allocsize) {
		return false;
	}

	int oldstate = fd->state;
	bool det = false;

#if FLASH_LOG > 1
	write_log(_T("flash write %08x %02x (%d) PC=%08x\n"), addr, v, fd->state, m68k_getpc());
#endif

	addr &= fd->mask;

	if (fd->state == 7 || fd->state == 8) {
		int a = addr * other_byte_mult;
		writeflash(fd, a, v);
		return true;
	}

	if (v == 0xf0 && fd->state > 0 && fd->state < 7) {
		fd->state = 0;
		return false;
	}

	// unlock
	if (addr == 0x5555 && fd->state <= 2 && v == 0xaa) {
		fd->state = 1;
		det = true;
	}
	if (addr == 0x2aaa && fd->state == 1 && v == 0x55) {
		fd->state = 2;
		det = true;
	}

	// software id exit and reset first byte
	if (addr == 0x5555 && fd->state == 3 && v == 0xaa) {
		fd->state = 1;
		det = true;
	}

	// autoselect
	if (addr == 0x5555 && fd->state == 2 && v == 0x90) {
		fd->state = 3;
		det = true;
	}

	// data protect enable
	if (addr == 0x5555 && fd->state == 2 && v == 0xa0) {
		fd->state = 7;
		det = true;
		fd->writeprot = -1;
		return false;
	}

	// data protect disable
	if (addr == 0x5555 && fd->state == 6 && v == 0x20) {
		fd->state = 0;
		fd->writeprot = 0;
		return false;
	}

	// chip/sector erase/protect disable
	if (addr == 0x5555 && fd->state == 2 && v == 0x80) {
		fd->state = 4;
		det = true;
	}
	if (addr == 0x5555 && fd->state == 4 && v == 0xaa) {
		fd->state = 5;
		det = true;
	}
	if (addr == 0x2aaa && fd->state == 5 && v == 0x55) {
		fd->state = 6;
		det = true;
	}
	if (addr == 0x5555 && fd->state == 6 && v == 0x10) {
		for (int i = 0; i < fd->allocsize; i++)  {
			int a = i * other_byte_mult;
			if (fd->rom[a] != 0xff) {
				fd->rom[a] = 0xff;
				setmodified(fd, a);
			}
		}
		fd->state = 200;
#if FLASH_LOG
		write_log(_T("flash chip erased\n"), addr);
#endif
		gui_flicker_led (LED_MD, 0, 2);
		return true;
	} else if (fd->state == 6 && v == 0x30) {
		int saddr = addr & ~(fd->sectorsize - 1);
		if (saddr < fd->allocsize) {
			for (int i = 0; i < fd->sectorsize; i++) {
				int a = (saddr + i) * other_byte_mult;
				if (fd->rom[a] != 0xff) {
					fd->rom[a] = 0xff;
					setmodified(fd, a);
				}
			}
		}
		fd->state = 200;
#if FLASH_LOG
		write_log(_T("flash sector %d erased (%08x)\n"), saddr / fd->sectorsize, addr);
#endif
		gui_flicker_led (LED_MD, 0, 2);
		return true;
	}

	if (fd->state == oldstate || !det) {
		fd->state = 0;
	}

	if (!det && (fd->flags & FLASHROM_PARALLEL_EEPROM)) {
		int a = addr * other_byte_mult;
		writeflash(fd, a, v);
		return true;
	}
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

	if (fd->flags & (FLASHROM_EVERY_OTHER_BYTE | FLASHROM_SKIP_EVERY_OTHER_BYTE)) {
		addr >>= 1;
		other_byte_mult = 2;
	}

	if (addr * other_byte_mult >= fd->allocsize) {
		return 0xff;
	}

	addr &= fd->mask;

	// write data in pagebuffer when any read is done
	if ((fd->flags & FLASHROM_PARALLEL_EEPROM) && fd->pagemodified) {
		fd->pagemodified = false;
		for (int i = 0; i < fd->pagesize; i++) {
			int offset = fd->lastpagewrite + i;
			if (fd->mpage[i]) {
				if (fd->rom[offset] != fd->page[i]) {
					fd->rom[offset] = fd->page[i];
					setmodified(fd, offset);
				}
				fd->mpage[i] = false;
			}
		}
	}

	if (fd->state == 3) {
		uae_u8 a = addr & 0xff;
		if (a == 0)
			v = fd->mfgcode;
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
		if (fd->state >= 110) {
			fd->state = 0;
		}
	} else {
		fd->state = 0;
		v = fd->rom[addr * other_byte_mult];
	}
#if FLASH_LOG > 1
	write_log(_T("flash read %08x = %02X (%d) PC=%08x\n"), oaddr, v, fd->state, m68k_getpc());
#endif

	return v;
}
