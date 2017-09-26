#ifndef UAE_DEVICES_H
#define UAE_DEVICES_H

void devices_reset(int hardreset);
void devices_vsync_pre(void);
void devices_hsync(void);
void devices_rethink(void);
STATIC_INLINE void devices_update_sound(float clk, float syncadjust)
{
  update_sound (clk);
}
void devices_update_sync(float svpos, float syncadjust);
void reset_all_systems(void);
void do_leave_program(void);
void virtualdevice_init(void);
void devices_restore_start(void);
void device_check_config(void);

#endif /* UAE_DEVICES_H */
