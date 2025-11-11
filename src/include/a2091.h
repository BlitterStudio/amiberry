#ifndef UAE_A2091_H
#define UAE_A2091_H

#include "commpipe.h"

#ifdef A2091

#define WD_STATUS_QUEUE 2

struct status_data
{
	volatile uae_u8 status;
	volatile int irq;
};

struct wd_chip_state {

	volatile uae_u8 sasr, scmd, auxstatus;
	volatile int wd_used;
	volatile int wd_phase, wd_next_phase, wd_busy, wd_data_avail;
	volatile bool wd_selected;
	volatile int wd_dataoffset;
	volatile uae_u8 wd_data[32];
	uae_u8 wdregs[32];
	volatile int queue_index;
	volatile uae_u16 intmask;
	struct status_data status[WD_STATUS_QUEUE];
	struct scsi_data *scsi;
	int wd33c93_ver;// 0 or 1
	bool resetnodelay;
	bool resetnodelay_active;
};

#define COMMODORE_8727 0
#define COMMODORE_DMAC 1
#define COMMODORE_SDMAC 2
#define GVP_DMAC_S2 3
#define GVP_DMAC_S1 4
#define COMSPEC_CHIP 6

struct commodore_dmac
{
	uae_u32 dmac_istr, dmac_cntr;
	uae_u32 dmac_dawr;
	uae_u32 dmac_acr;
	uae_u32 dmac_wtc;
	int dmac_dma;
	int old_dmac;

	uae_u8 xt_control;
	uae_u8 xt_status;
	uae_u16 xt_cyls[2], xt_heads[2], xt_sectors[2];

	bool xt_irq;
	int xt_offset;
	int xt_datalen;
	uae_u8 xt_cmd[6];
	uae_u8 xt_statusbyte;

	uae_u8 c8727_pcss;
	uae_u8 c8727_pcsd;
	uae_u8 c8727_ctl;
	uae_u8 c8727_wrcbp;
	uae_u32 c8727_st506_cb;
};
struct gvp_dmac
{
	uae_u16 cntr;
	uae_u32 addr;
	uae_u16 len;
	uae_u8 bank;
	uae_u8 maprom;
	int dma_on;
	uae_u8 version;
	bool use_version;
	uae_u32 addr_mask;
	bool series2;
	int s1_subtype;
	int s1_ramoffset;
	int s1_rammask;
	uae_u8 *buffer;
	int bufoffset;
	uae_u8 *bank_ptr;
};

struct comspec_chip
{
	uae_u8 status;
};

struct wd_state {
	int id;
	bool enabled;
	int configured;
	bool autoconfig;
	bool threaded;
	uae_u8 dmacmemory[128];
	uae_u8 *rom, *rom2;
	int board_mask;
	uaecptr baseaddress, baseaddress2;
	int rombankswitcher, rombank;
	int rom_size, rom_mask;
	int rom2_size, rom2_mask;
	struct romconfig *rc;
	struct wd_state **self_ptr;

	smp_comm_pipe requests;
	volatile int scsi_thread_running;

	// unit 8,9 = ST-506 (A2090)
	// unit 8 = XT (A2091)
	struct scsi_data *scsis[8 + 2];

	bool cdtv;

	int dmac_type;
	struct wd_chip_state wc;
	struct commodore_dmac cdmac;
	struct gvp_dmac gdmac;
	struct comspec_chip comspec;
	addrbank bank;
	addrbank bank2;
	void *userdata;
	uae_u32 dma_mask;
};
extern wd_state *wd_cdtv;

extern void init_wd_scsi (struct wd_state*, bool);
extern void scsi_dmac_a2091_start_dma (struct wd_state*);
extern void scsi_dmac_a2091_stop_dma (struct wd_state*);

extern bool a2090_init (struct autoconfig_info *aci);
extern bool a2090b_init (struct autoconfig_info *aci);
extern bool a2090b_preinit (struct autoconfig_info *aci);

extern bool a2091_init (struct autoconfig_info *aci);

extern bool gvp_init_s1(struct autoconfig_info *aci);
extern bool gvp_init_s2(struct autoconfig_info *aci);
extern bool gvp_init_accelerator(struct autoconfig_info *aci);
extern bool gvp_init_a1208(struct autoconfig_info *aci);

extern bool comspec_init (struct autoconfig_info *aci);
extern bool comspec_preinit (struct autoconfig_info *aci);

extern bool a3000scsi_init(struct autoconfig_info *aci);

extern void wdscsi_put (struct wd_chip_state*, wd_state*, uae_u8);
extern uae_u8 wdscsi_get (struct wd_chip_state*, struct wd_state*);
extern uae_u8 wdscsi_getauxstatus (struct wd_chip_state*);
extern void wdscsi_sasr (struct wd_chip_state*, uae_u8);

#define WDTYPE_A2091 0
#define WDTYPE_A2091_2 1
#define WDTYPE_A3000 2
#define WDTYPE_CDTV 3
#define WDTYPE_GVP 4

#define WD33C93 _T("WD33C93")

extern void a2090_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void a2091_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void gvp_s1_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void gvp_s2_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void gvp_s2_add_accelerator_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void gvp_a1208_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void a3000_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);
extern void comspec_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc);

extern int add_wd_scsi_hd (struct wd_state *wd, int ch, struct hd_hardfiledata *hfd, struct uaedev_config_info *ci, int scsi_level);
extern int add_wd_scsi_cd (struct wd_state *wd, int ch, int unitnum);
extern int add_wd_scsi_tape (struct wd_state *wd, int ch, const TCHAR *tape_directory, bool readonly);

void gvp_accelerator_set_dma_bank(uae_u8);

#endif /* A2091 */

#endif /* UAE_A2091_H */
