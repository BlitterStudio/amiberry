#ifndef UAE_SCSI_H
#define UAE_SCSI_H

#include "uae/types.h"
#include "memory.h"

#define SCSI_DEFAULT_DATA_BUFFER_SIZE (256 * 512)

struct scsi_data
{
	int id;
	void *privdata;
	int cmd_len;
	uae_u8 *data;
	int data_len;
	int status;
	uae_u8 sense[256];
	int sense_len;
	uae_u8 reply[256];
	uae_u8 cmd[16];
	uae_u8 msgout[4];
	int reply_len;
	int direction;
	uae_u8 message[1];
	int blocksize;

	int offset;
	uae_u8 *buffer;
	int buffer_size;
	struct hd_hardfiledata *hdhfd;
	struct hardfiledata *hfd;
	int device_type;
	int cd_emu_unit;
	bool atapi;
	uae_u32 unit_attention;
};

extern struct scsi_data *scsi_alloc_generic(struct hardfiledata *hfd, int type);
extern struct scsi_data *scsi_alloc_cd(int, int, bool);
extern void scsi_free(struct scsi_data*);

extern void scsi_start_transfer(struct scsi_data*);
extern void scsi_emulate_cmd(struct scsi_data *sd);
extern void scsi_clear_sense(struct scsi_data *sd);

extern int scsi_hd_emulate(struct hardfiledata *hfd, struct hd_hardfiledata *hdhfd, uae_u8 *cmdbuf, int scsi_cmd_len,
		uae_u8 *scsi_data, int *data_len, uae_u8 *r, int *reply_len, uae_u8 *s, int *sense_len);
extern bool scsi_emulate_analyze (struct scsi_data*);

#define SCSI_NO_SENSE_DATA		0x00
#define SCSI_NOT_READY			0x04
#define SCSI_NOT_LOADED			0x09
#define SCSI_INSUF_CAPACITY		0x0a
#define SCSI_HARD_DATA_ERROR	0x11
#define SCSI_WRITE_PROTECT		0x17
#define SCSI_CORRECTABLE_ERROR	0x18
#define SCSI_FILE_MARK			0x1c
#define SCSI_INVALID_COMMAND	0x20
#define SCSI_INVALID_FIELD		0x24
#define SCSI_INVALID_LUN		0x25
#define SCSI_UNIT_ATTENTION		0x30
#define SCSI_END_OF_MEDIA		0x34
#define SCSI_MEDIUM_NOT_PRESENT	0x3a

#define SCSI_SK_NO_SENSE        0x0
#define SCSI_SK_REC_ERR         0x1     /* recovered error */
#define SCSI_SK_NOT_READY       0x2
#define SCSI_SK_MED_ERR         0x3     /* medium error */
#define SCSI_SK_HW_ERR          0x4     /* hardware error */
#define SCSI_SK_ILLEGAL_REQ     0x5
#define SCSI_SK_UNIT_ATT        0x6     /* unit attention */
#define SCSI_SK_DATA_PROTECT    0x7
#define SCSI_SK_BLANK_CHECK     0x8
#define SCSI_SK_VENDOR_SPEC     0x9
#define SCSI_SK_COPY_ABORTED    0xA
#define SCSI_SK_ABORTED_CMND    0xB
#define SCSI_SK_VOL_OVERFLOW    0xD
#define SCSI_SK_MISCOMPARE      0xE

#define SCSI_STATUS_GOOD                   0x00
#define SCSI_STATUS_CHECK_CONDITION        0x02
#define SCSI_STATUS_CONDITION_MET          0x04
#define SCSI_STATUS_BUSY                   0x08
#define SCSI_STATUS_INTERMEDIATE           0x10
#define SCSI_STATUS_ICM                    0x14 /* intermediate condition met */
#define SCSI_STATUS_RESERVATION_CONFLICT   0x18
#define SCSI_STATUS_COMMAND_TERMINATED     0x22
#define SCSI_STATUS_QUEUE_FULL             0x28
#define SCSI_STATUS_ACA_ACTIVE             0x30

#endif /* UAE_SCSI_H */
