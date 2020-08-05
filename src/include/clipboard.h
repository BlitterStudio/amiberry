#ifndef UAE_CLIPBOARD_H
#define UAE_CLIPBOARD_H

#include "uae/types.h"

extern void clipboard_init();
extern void clipboard_changed(int);
extern void clipboard_reset();
extern void clipboard_active(int, int);

extern int amiga_clipboard_want_data(TrapContext *ctx);
extern void amiga_clipboard_got_data(TrapContext *ctx, uaecptr data, uae_u32 size, uae_u32 actual);
extern void amiga_clipboard_die(TrapContext *ctx);
extern void amiga_clipboard_init(TrapContext *ctx);
extern uaecptr amiga_clipboard_proc_start(TrapContext *ctx);
extern void amiga_clipboard_task_start(TrapContext *ctx, uaecptr);
extern void clipboard_disable(bool);
extern void clipboard_vsync(void);
extern void clipboard_unsafeperiod(void);

#endif /* UAE_CLIPBOARD_H */
