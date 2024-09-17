#ifndef UAE_PCI_HW_H
#define UAE_PCI_HW_H

#include "uae/types.h"

#define MAX_PCI_BOARDS 6
#define MAX_PCI_BARS 7

typedef uae_u32(REGPARAM3 *pci_get_func)(struct pci_board_state*,uaecptr) REGPARAM;
typedef void (REGPARAM3 *pci_put_func)(struct pci_board_state*,uaecptr,uae_u32) REGPARAM;
typedef void (*pci_dev_irq)(struct pci_board_state*,bool);
typedef bool(*pci_dev_init)(struct pci_board_state*,struct autoconfig_info*);
typedef void(*pci_dev_reset)(struct pci_board_state*);
typedef void(*pci_dev_hsync)(struct pci_board_state*);
typedef void(*pci_dev_chkcfg)(struct pci_board_state*, uae_u8*);
typedef void(*pci_dev_free)(struct pci_board_state*);
typedef uae_u8(*pci_get_config_func)(uaecptr);
typedef void(*pci_put_config_func)(uaecptr, uae_u8);
typedef void(*pci_change_config_func)(struct pci_board_state*);

typedef struct
{
	pci_get_func lget, wget, bget;
	pci_put_func lput, wput, bput;
} pci_addrbank;

typedef int(*pci_slot_index)(uaecptr, bool, uae_u32*);

struct pci_config
{
	uae_u16 vendor;
	uae_u16 device;
	uae_u16 command;
	uae_u16 status;
	uae_u8 revision;
	uae_u32 deviceclass;
	uae_u8 header;
	uae_u16 subsystenvendor;
	uae_u16 subsystem;
	uae_u8 interruptpin;
	uae_u8 min_grant;
	uae_u8 max_latency;
	uae_u32 bars[MAX_PCI_BARS];
};

struct pci_board
{
	const TCHAR *label;
	const struct pci_config *config;
	pci_dev_init init;
	pci_dev_free free;
	pci_dev_reset reset;
	pci_dev_hsync hsync;
	pci_dev_chkcfg chkcfg;
	pci_addrbank bars[MAX_PCI_BARS + 1];
	bool dont_mask_io;
	pci_get_config_func pci_get_config;
	pci_put_config_func pci_put_config;
	pci_change_config_func pci_change_config;

};

struct pci_board_state
{
	uae_u8 config_data[256 + 3];
	uaecptr bar[MAX_PCI_BARS];
	uaecptr bar_old[MAX_PCI_BARS];
	bool bar_enabled[MAX_PCI_BARS];
	uaecptr bar_start[MAX_PCI_BARS];
	uaecptr bar_end[MAX_PCI_BARS];
	uae_u32 bar_size[MAX_PCI_BARS];
	int selected_bar;
	const struct pci_board *board;
	int slot;
	int func;
	bool memory_map_active;
	bool io_map_active;
	struct pci_bridge *bridge;
	pci_dev_irq irq_callback;
	struct pci_config dynamic_config;
	void *userdata;
};

struct pci_bridge
{
	const TCHAR *label;
	int type;
	int endian_swap_config;
	uae_u32 io_offset;
	uae_u32 memory_start_offset[2];
	int endian_swap_io;
	uae_u32 memory_window_offset;
	int endian_swap_memory;
	bool pcipcidma;
	bool amigapicdma;
	uae_u8 intena;
	uae_u8 irq;
	uae_u8 reset;
	uae_u16 intreq_mask;
	pci_slot_index get_index;
	struct pci_board_state boards[MAX_PCI_BOARDS];
	uae_u8 config[16];
	uae_u8 *data;
	int configured;
	int configured_2;
	int bank_zorro;
	int bank_2_zorro;
	addrbank *bank;
	addrbank *bank_2;
	int board_size;
	int board_size_2;
	uaecptr baseaddress;
	uaecptr baseaddress_end;
	uaecptr baseaddress_offset;
	uaecptr baseaddress_2;
	uaecptr baseaddress_end_2;
	uaecptr baseaddress_offset_2;
	uae_u8 acmemory[128];
	uae_u8 acmemory_2[128];
	struct romconfig *rc;
	uae_u16 window[2];
	int log_slot_cnt;
	int phys_slot_cnt;
	bool multiwindow;
	int windowindex;
};

extern void pci_irq_callback(struct pci_board_state *pcibs, bool irq);
extern void pci_write_dma(struct pci_board_state *pcibs, uaecptr addr, uae_u8*, int size);
extern void pci_read_dma(struct pci_board_state *pcibs, uaecptr addr, uae_u8*, int size);

extern const struct pci_board ne2000_pci_board;
extern const struct pci_board ne2000_pci_board_x86;
extern const struct pci_board ne2000_pci_board_pcmcia;
extern const struct pci_board ne2000_pci_board;

extern const struct pci_board es1370_pci_board;
extern const struct pci_board fm801_pci_board;
extern const struct pci_board fm801_pci_board_func1;
extern const struct pci_board solo1_pci_board;

#endif /* UAE_PCI_HW_H */
