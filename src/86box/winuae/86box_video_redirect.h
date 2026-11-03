/*
* UAE - The Un*x Amiga Emulator
*
* Force-included into the 86Box video card sources.
*
* The RTG boards are not part of the PC the bridgeboards emulate: their
* memory and I/O handlers live on the Amiga side (gfxboard.cpp), their
* timers run from the Amiga's horizontal sync and their helper devices
* (RAMDACs, clock generators) must not join the bridgeboard's device
* list. Rename the 86Box calls to the versions in 86box_video.cpp.
*/

#ifndef WINUAE_86BOX_VIDEO_REDIRECT_H
#define WINUAE_86BOX_VIDEO_REDIRECT_H

#define mem_mapping_add             rtg_mem_mapping_add
#define mem_mapping_set_handler     rtg_mem_mapping_set_handler
#define mem_mapping_set_p           rtg_mem_mapping_set_p
#define mem_mapping_set_addr        rtg_mem_mapping_set_addr
#define mem_mapping_set_base_ignore rtg_mem_mapping_set_base_ignore
#define mem_mapping_disable         rtg_mem_mapping_disable
#define mem_mapping_enable          rtg_mem_mapping_enable
#define mem_readl_phys              rtg_mem_readl_phys
#define dma_bm_read                 rtg_dma_bm_read
#define dma_bm_write                rtg_dma_bm_write

#define io_sethandler               rtg_io_sethandler
#define io_removehandler            rtg_io_removehandler

#define timer_add                   rtg_timer_add
#define timer_enable                rtg_timer_enable
#define timer_disable               rtg_timer_disable
#define timer_on_auto               rtg_timer_on_auto
#define timer_stop                  rtg_timer_stop

#define device_add                  rtg_device_add
#define device_add_inst             rtg_device_add_inst
#define device_add_params           rtg_device_add_params
#define device_add_inst_params      rtg_device_add_inst_params
#define device_get_instance         rtg_device_get_instance
#define device_get_config_int       rtg_device_get_config_int
#define device_get_config_bios      rtg_device_get_config_bios
#define device_get_bios_file        rtg_device_get_bios_file
#define device_get_bios_flags       rtg_device_get_bios_flags
#define device_get_bios_local       rtg_device_get_bios_local

#define pci_add_card                rtg_pci_add_card
#define pci_irq                     rtg_pci_irq

#define machine_has_bus             rtg_machine_has_bus

#endif /* WINUAE_86BOX_VIDEO_REDIRECT_H */
