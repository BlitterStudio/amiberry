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
	bool atapi;
	uae_u32 unit_attention;
};

extern struct scsi_data *scsi_alloc_generic(struct hardfiledata *hfd, int type);
extern void scsi_free(struct scsi_data*);

extern void scsi_start_transfer(struct scsi_data*);
extern void scsi_emulate_cmd(struct scsi_data *sd);
extern void scsi_clear_sense(struct scsi_data *sd);

extern int scsi_hd_emulate(struct hardfiledata *hfd, struct hd_hardfiledata *hdhfd, uae_u8 *cmdbuf, int scsi_cmd_len,
		uae_u8 *scsi_data, int *data_len, uae_u8 *r, int *reply_len, uae_u8 *s, int *sense_len);
extern bool scsi_emulate_analyze (struct scsi_data*);

#define SCSI_STATUS_GOOD                   0x00
#define SCSI_STATUS_CHECK_CONDITION        0x02

#endif /* UAE_SCSI_H */
