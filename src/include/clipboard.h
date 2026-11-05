#ifndef UAE_CLIPBOARD_H
#define UAE_CLIPBOARD_H

#include "uae/types.h"
#include "clipboard_common.h"

#include <string>

extern void clipboard_reset();

extern int amiga_clipboard_want_data(TrapContext *ctx);
extern void amiga_clipboard_got_data(TrapContext *ctx, uaecptr data, uae_u32 size, uae_u32 actual);
extern void amiga_clipboard_die(TrapContext *ctx);
extern void amiga_clipboard_init(TrapContext *ctx);
extern uaecptr amiga_clipboard_proc_start(TrapContext *ctx);
extern void amiga_clipboard_task_start(TrapContext *ctx, uaecptr);
extern void clipboard_disable(bool);
extern void clipboard_vsync(void);
extern void clipboard_unsafeperiod(void);

// What the host clipboard holds, or what the guest copied.
struct clipboard_content {
	enum class kind { none, text, bitmap };
	kind type = kind::none;
	std::string text;
	clipboard_bitmap bitmap;
};

// The host side calls these from the main thread; the emulation side moves
// the content to and from the guest's clipboard.device.

// The host clipboard now holds this; it is offered to the guest unless it is
// what the guest copied last.
extern void clipboard_set_host_content(clipboard_content &&content);
// Takes what the guest copied since the last call, for the host clipboard.
extern bool clipboard_take_guest_content(clipboard_content *content);
// True once after the guest's clipboard task has started: the host clipboard
// should be read now so that its current content reaches the guest.
extern bool clipboard_host_snapshot_wanted();

#endif /* UAE_CLIPBOARD_H */
