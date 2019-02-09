#ifndef UAE_DEVICES_H
#define UAE_DEVICES_H

void devices_reset(int hardreset);
void devices_vsync_pre(void);
void devices_hsync(void);
void devices_rethink(void);
void devices_update_sound(double clk, double syncadjust);
void devices_update_sync(double svpos, double syncadjust);
void reset_all_systems(void);
void do_leave_program(void);
void virtualdevice_init(void);
void devices_restore_start(void);
void device_check_config(void);
void devices_pause(void);
void devices_unpause(void);

#endif /* UAE_DEVICES_H */
