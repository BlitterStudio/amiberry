#ifndef UAE_DEVICES_H
#define UAE_DEVICES_H

void devices_reset(int hardreset);
void devices_reset_ext(int hardreset);
void devices_vsync_pre(void);
void devices_vsync_post(void);
void devices_hsync(void);
void devices_rethink(void);
void devices_rethink_all(void func(void));
void devices_syncchange(void);
void devices_update_sound(float clk, float syncadjust);
void devices_update_sync(float svpos, float syncadjust);
void do_leave_program(void);
void virtualdevice_init(void);
void virtualdevice_free(void);
void devices_restore_start(void);
void device_check_config(void);
void devices_pause(void);
void devices_unpause(void);
void devices_unsafeperiod(void);

typedef void (*DEVICE_INT)(int hardreset);
typedef void (*DEVICE_VOID)(void);

void device_add_vsync_pre(DEVICE_VOID p);
void device_add_vsync_post(DEVICE_VOID p);
void device_add_hsync(DEVICE_VOID p);
void device_add_rethink(DEVICE_VOID p);
void device_add_check_config(DEVICE_VOID p);
void device_add_reset(DEVICE_INT p);
void device_add_reset_imm(DEVICE_INT p);
void device_add_exit(DEVICE_VOID p, DEVICE_VOID p2);

#define IRQ_SOURCE_PCI 0
#define IRQ_SOURCE_SOUND 1
#define IRQ_SOURCE_NE2000 2
#define IRQ_SOURCE_A2065 3
#define IRQ_SOURCE_NCR 4
#define IRQ_SOURCE_NCR9X 5
#define IRQ_SOURCE_CPUBOARD 6
#define IRQ_SOURCE_UAE 7
#define IRQ_SOURCE_SCSI 8
#define IRQ_SOURCE_WD 9
#define IRQ_SOURCE_X86 10
#define IRQ_SOURCE_GAYLE 11
#define IRQ_SOURCE_CIA 12
#define IRQ_SOURCE_CD32CDTV 13
#define IRQ_SOURCE_IDE 14
#define IRQ_SOURCE_GFX 15
#define IRQ_SOURCE_MAX 16


#endif /* UAE_DEVICES_H */
