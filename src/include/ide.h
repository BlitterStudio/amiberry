#ifndef UAE_IDE_H
#define UAE_IDE_H

#include "uae/types.h"

/* IDE drive registers */
#define IDE_DATA	0x00
#define IDE_ERROR	0x01	    /* see err-bits */
#define IDE_NSECTOR	0x02	    /* sector count, nr of sectors to read/write */
#define IDE_SECTOR	0x03	    /* starting sector */
#define IDE_LCYL	0x04	    /* starting cylinder */
#define IDE_HCYL	0x05	    /* high byte of starting cyl */
#define IDE_SELECT	0x06	    /* 101dhhhh , d=drive, hhhh=head */
#define IDE_STATUS	0x07	    /* see status-bits */

#define IDE_SECONDARY 0x0400
#define IDE_DEVCON	0x0406
#define IDE_DRVADDR	0x0407

struct ide_registers
{
	uae_u8 ide_select, ide_nsector, ide_sector, ide_lcyl, ide_hcyl, ide_devcon, ide_error, ide_feat;
	uae_u8 ide_nsector2, ide_sector2, ide_lcyl2, ide_hcyl2, ide_feat2;
	uae_u8 ide_status;
};

struct ide_thread_state;
struct ide_hdf;

#define MAX_IDE_PORTS_BOARD 3
struct ide_board
{
	uae_u8 *rom;
	uae_u8 acmemory[128];
	int rom_size;
	int rom_start;
	int rom_mask;
	uaecptr baseaddress;
	int configured;
	bool keepautoconfig;
	int mask;
	addrbank *bank;
	struct ide_hdf *ide[MAX_IDE_PORTS_BOARD];
	bool irq;
	bool intena;
	bool enabled;
	int state;
	int type;
	int userdata;
	int subtype;
	uae_u16 data_latch;
	struct ide_board **self_ptr;
};

struct ide_hdf
{
	struct hd_hardfiledata hdhfd;
	struct ide_board *board;
	struct ide_registers regs;
	struct ide_registers *regs0;
	struct ide_registers *regs1;
	struct ide_hdf *pair; // master<>slave
	struct ide_thread_state *its;
	bool byteswap;
	int byteswapped_buffer;
	bool adide;

	uae_u8 *secbuf;
	int secbuf_size;
	int buffer_offset;
	int data_offset;
	int data_size;
	int data_multi;
	int direction; // 0 = read, 1 = write
	bool intdrq;
	bool lba48;
	bool lba48cmd;
	uae_u64 start_lba;
	int start_nsec;
	uae_u8 multiple_mode;
	int irq_delay;
	int irq;
	bool irq_new;
	int num;
	int blocksize;
	int maxtransferstate;
	int ata_level;
	int ide_drv;
	int media_type;
	bool mode_8bit;

	bool atapi;
	bool atapi_drdy;
	int cd_unit_num;
	int packet_state;
	int packet_data_size;
	int packet_data_offset;
	int packet_transfer_size;
	struct scsi_data *scsi;
};

struct ide_thread_state
{
	struct ide_hdf **idetable;
	int idetotal;
	volatile int state;
	smp_comm_pipe requests;	
};

uae_u32 ide_read_reg (struct ide_hdf *ide, int ide_reg);
void ide_write_reg (struct ide_hdf *ide, int ide_reg, uae_u32 val);
void ide_put_data(struct ide_hdf *ide, uae_u16 v);
uae_u16 ide_get_data(struct ide_hdf *ide);
void ide_put_data_8bit(struct ide_hdf *ide, uae_u8 v);
uae_u8 ide_get_data_8bit(struct ide_hdf *ide);

bool ide_interrupt_hsync(struct ide_hdf *ide);
bool ide_irq_check(struct ide_hdf *ide, bool edge_triggered);
bool ide_drq_check(struct ide_hdf *ide);
bool ide_isdrive(struct ide_hdf *ide);
void ide_initialize(struct ide_hdf **idetable, int chpair);
struct ide_hdf *add_ide_unit (struct ide_hdf **idetable, int max, int ch, struct uaedev_config_info *ci, struct romconfig *rc);
void remove_ide_unit(struct ide_hdf **idetable, int ch);
void alloc_ide_mem (struct ide_hdf **ide, int max, struct ide_thread_state *its);
void ide_reset_device(struct ide_hdf *ide);

void start_ide_thread(struct ide_thread_state *its);
void stop_ide_thread(struct ide_thread_state *its);

uae_u16 adide_decode_word(uae_u16 w);
uae_u16 adide_encode_word(uae_u16 w);

uae_u8 *ide_save_state(uae_u8 *dst, struct ide_hdf *ide);
uae_u8 *ide_restore_state(uae_u8 *src, struct ide_hdf *ide);

#define IDE_MEMORY_FUNCTIONS(x, y, z) \
static void REGPARAM2 x ## _bput(uaecptr addr, uae_u32 b) \
{ \
	y ## _write_byte(z, addr, b); \
} \
static void REGPARAM2 x ## _wput(uaecptr addr, uae_u32 b) \
{ \
	y ## _write_word(z, addr, b); \
} \
static void REGPARAM2 x ## _lput(uaecptr addr, uae_u32 b) \
{ \
	y ## _write_word(z, addr, b >> 16); \
	y ## _write_word(z, addr + 2, b); \
} \
static uae_u32 REGPARAM2 x ## _bget(uaecptr addr) \
{ \
return y ## _read_byte(z, addr); \
} \
static uae_u32 REGPARAM2 x ## _wget(uaecptr addr) \
{ \
return y ## _read_word(z, addr); \
} \
static uae_u32 REGPARAM2 x ## _lget(uaecptr addr) \
{ \
	uae_u32 v = y ## _read_word(z, addr) << 16; \
	v |= y ## _read_word(z, addr + 2); \
	return v; \
}

#endif /* UAE_IDE_H */
