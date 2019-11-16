/*
* UAE - The Un*x Amiga Emulator
*
* Simple 29Fxxx flash ROM chip emulator
* I2C EEPROM (24C08)
* MICROWIRE EEPROM (9346)
*
* (c) 2014 Toni Wilen
*/

#include "sysdeps.h"

#include "zfile.h"
#include "flashrom.h"
#include "gui.h"
#include "uae.h"

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
  		} else if (!(i2c->current_addr & 1)) {
  			if (i2c->write_offset < 0)
  				i2c->write_offset = i2c->eeprom_addr;
		    if (i2c->write_func) {
			    i2c->write_func(i2c->eeprom_addr, i2c->buffer);
		    } else {
		      i2c->memory[i2c->eeprom_addr] = i2c->buffer;
		      i2c->eeprom_addr = (i2c->eeprom_addr & ~(NVRAM_PAGE_SIZE - 1)) | (i2c->eeprom_addr + 1) & (NVRAM_PAGE_SIZE - 1);
		      gui_flicker_led (LED_MD, 0, 2);
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
  			i2c->state = SENT_NACK;
        //i2c_nack(i2c->bus);
  		}
      return bitbang_i2c_ret(i2c, 1);
  }
  target_startup_msg(_T("Internal error"), _T("eeprom_i2c_set: Unhandled case."));
  uae_restart(1, NULL);
  return 0;
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
	s->device_address = 0xa0;
	s->device_address_mask = 0xf0;

  return s;
}

void eeprom_free(void *fdv)
{
	struct bitbang_i2c_interface *i2c = (bitbang_i2c_interface*)fdv;
	xfree(i2c);
}
