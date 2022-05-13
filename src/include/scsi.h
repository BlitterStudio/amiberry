#ifndef UAE_SCSI_H
#define UAE_SCSI_H

#include "uae/types.h"
#include "memory.h"

#define SCSI_DEFAULT_DATA_BUFFER_SIZE (256 * 512)

struct scsi_data_tape
{
	TCHAR tape_dir[MAX_DPATH];
	int file_number;
	uae_s64 file_offset;
	int blocksize;
	bool realdir;
	struct zdirectory *zd;
	struct my_opendir_s *od;
	struct zfile *zf;
	struct zfile *index;
	int beom;
	bool wp;
	bool nomedia;
	bool unloaded;
	bool init_loaded;
	bool pending_filemark;
	bool last_filemark;
};

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
	struct scsi_data_tape *tape;
	int device_type;
	int nativescsiunit;
	int cd_emu_unit;
	bool atapi;
	uae_u32 unit_attention;
	int uae_unitnum;
};

extern struct scsi_data *scsi_alloc_generic(struct hardfiledata *hfd, int type, int);
extern struct scsi_data *scsi_alloc_hd(int, struct hd_hardfiledata*, int);
extern struct scsi_data *scsi_alloc_cd(int, int, bool, int);
extern struct scsi_data *scsi_alloc_tape(int id, const TCHAR *tape_directory, bool readonly, int);
extern struct scsi_data *scsi_alloc_native(int, int);
extern void scsi_free(struct scsi_data*);
extern void scsi_reset(void);

extern void scsi_start_transfer(struct scsi_data*);
extern int scsi_send_data(struct scsi_data*, uae_u8);
extern int scsi_receive_data(struct scsi_data*, uae_u8*, bool next);
extern void scsi_emulate_cmd(struct scsi_data *sd);
extern void scsi_illegal_lun(struct scsi_data *sd);
extern void scsi_clear_sense(struct scsi_data *sd);
extern bool scsi_cmd_is_safe(uae_u8 cmd);

extern int scsi_hd_emulate(struct hardfiledata *hfd, struct hd_hardfiledata *hdhfd, uae_u8 *cmdbuf, int scsi_cmd_len,
		uae_u8 *scsi_data, int *data_len, uae_u8 *r, int *reply_len, uae_u8 *s, int *sense_len);
extern int scsi_tape_emulate(struct scsi_data_tape *sd, uae_u8 *cmdbuf, int scsi_cmd_len,
		uae_u8 *scsi_data, int *data_len, uae_u8 *r, int *reply_len, uae_u8 *s, int *sense_len);
extern bool scsi_emulate_analyze (struct scsi_data*);

extern bool tape_get_info (int, struct device_info*);
extern struct scsi_data_tape *tape_alloc (int unitnum, const TCHAR *tape_directory, bool readonly);
extern void tape_free (struct scsi_data_tape*);
extern void tape_media_change (int unitnum, struct uaedev_config_info*);

int add_scsi_device(struct scsi_data **sd, int ch, struct uaedev_config_info *ci, struct romconfig *rc);
int add_scsi_hd (struct scsi_data **sd, int ch, struct hd_hardfiledata *hfd, struct uaedev_config_info *ci);
int add_scsi_cd (struct scsi_data **sd, int ch, int unitnum);
int add_scsi_tape (struct scsi_data **sd, int ch, const TCHAR *tape_directory, bool readonly);
void free_scsi (struct scsi_data *sd);
bool tape_can_write(const TCHAR *tape_directory);

void scsi_freenative(struct scsi_data **sd, int max);
void scsi_addnative(struct scsi_data **sd);

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

#define SCSI_MEMORY_FUNCTIONS(x, y, z) \
static void REGPARAM2 x ## _bput(uaecptr addr, uae_u32 b) \
{ \
	y ## _bput(&z, addr, b); \
} \
static void REGPARAM2 x ## _wput(uaecptr addr, uae_u32 b) \
{ \
	y ## _wput(&z, addr, b); \
} \
static void REGPARAM2 x ## _lput(uaecptr addr, uae_u32 b) \
{ \
	y ## _lput(&z, addr, b); \
} \
static uae_u32 REGPARAM2 x ## _bget(uaecptr addr) \
{ \
return y ## _bget(&z, addr); \
} \
static uae_u32 REGPARAM2 x ## _wget(uaecptr addr) \
{ \
return y ## _wget(&z, addr); \
} \
static uae_u32 REGPARAM2 x ## _lget(uaecptr addr) \
{ \
return y ## _lget(&z, addr); \
}

void soft_scsi_put(uaecptr addr, int size, uae_u32 v);
uae_u32 soft_scsi_get(uaecptr addr, int size);

void apollo_scsi_bput(uaecptr addr, uae_u8 v, uae_u32 config);
uae_u8 apollo_scsi_bget(uaecptr addr, uae_u32 config);
void apollo_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

void ivsvector_scsi_bput(uaecptr addr, uae_u8 v);
uae_u8 ivsvector_scsi_bget(uaecptr addr);
void ivsvector_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
bool ivsvector_init(struct autoconfig_info *aci);

void twelvegauge_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
bool twelvegauge_init(struct autoconfig_info *aci);

uae_u8 parallel_port_scsi_read(int reg, uae_u8 data, uae_u8 dir);
void parallel_port_scsi_write(int reg, uae_u8 v, uae_u8 dir);
extern bool parallel_port_scsi;

bool supra_init(struct autoconfig_info*);
void supra_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool golem_init(struct autoconfig_info*);
void golem_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool stardrive_init(struct autoconfig_info*);
void stardrive_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool kommos_init(struct autoconfig_info*);
void kommos_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool vector_init(struct autoconfig_info*);
void vector_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool protar_init(struct autoconfig_info *aci);
void protar_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool add500_init(struct autoconfig_info *aci);
void add500_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool kronos_init(struct autoconfig_info *aci);
void kronos_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool adscsi_init(struct autoconfig_info *aci);
void adscsi_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool trifecta_init(struct autoconfig_info *aci);
void trifecta_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

void rochard_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
bool rochard_scsi_init(struct romconfig *rc, uaecptr baseaddress);

bool cltda1000scsi_init(struct autoconfig_info *aci);
void cltda1000scsi_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool ptnexus_init(struct autoconfig_info *aci);
void ptnexus_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool dataflyer_init(struct autoconfig_info *aci);
void dataflyer_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool dataflyerplus_scsi_init(struct romconfig *rc, uaecptr baseaddress);
void dataflyerplus_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool tecmar_init(struct autoconfig_info *aci);
void tecmar_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool xebec_init(struct autoconfig_info *aci);
void xebec_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool microforge_init(struct autoconfig_info *aci);
void microforge_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool paradox_init(struct autoconfig_info *aci);
void paradox_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool hda506_init(struct autoconfig_info *aci);
void hda506_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool alf1_init(struct autoconfig_info *aci);
void alf1_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool alf2_init(struct autoconfig_info *aci);
void alf2_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool promigos_init(struct autoconfig_info *aci);
void promigos_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool system2000_preinit(struct autoconfig_info *aci);
bool system2000_init(struct autoconfig_info *aci);
void system2000_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool omtiadapter_init(struct autoconfig_info *aci);
void omtiadapter_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool hd20_init(struct autoconfig_info *aci);
void hd20_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool phoenixboard_init(struct autoconfig_info *aci);
void phoenixboard_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool trumpcard_init(struct autoconfig_info*);
void trumpcard_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool trumpcardpro_init(struct autoconfig_info*);
void trumpcardpro_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool scram5380_init(struct autoconfig_info*);
void scram5380_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool ossi_init(struct autoconfig_info*);
void ossi_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool hardframe_init(struct autoconfig_info*);
void hardframe_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool malibu_init(struct autoconfig_info*);
void malibu_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool addhard_init(struct autoconfig_info*);
void addhard_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool inmate_init(struct autoconfig_info*);
void inmate_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool emplant_init(struct autoconfig_info*);
void emplant_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool hd3000_init(struct autoconfig_info *aci);
void hd3000_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool wedge_preinit(struct autoconfig_info *aci);
bool wedge_init(struct autoconfig_info *aci);
void wedge_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool eveshamref_init(struct autoconfig_info *aci);
void eveshamref_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool profex_init(struct autoconfig_info *aci);
void profex_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool fasttrak_init(struct autoconfig_info *aci);
void fasttrak_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool overdrive_init(struct autoconfig_info *aci);
void overdrive_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool synthesis_init(struct autoconfig_info* aci);
void synthesis_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

bool fireball_init(struct autoconfig_info* aci);
void fireball_add_scsi_unit(int ch, struct uaedev_config_info* ci, struct romconfig* rc);

uae_u8 idescsi_scsi_get(uaecptr addr);
void idescsi_scsi_put(uaecptr addr, uae_u8 v);

void x86_rt1000_bput(int, uae_u8);
uae_u8 x86_rt1000_bget(int);
bool x86_rt1000_init(struct autoconfig_info *aci);
void x86_rt1000_add_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

#endif /* UAE_SCSI_H */
