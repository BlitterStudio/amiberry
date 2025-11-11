#ifndef UAE_SPECIALMONITORS_H
#define UAE_SPECIALMONITORS_H

#include "memory.h"

extern bool emulate_specialmonitors(struct vidbuffer *src, struct vidbuffer *dst);
extern void specialmonitor_store_fmode(int vpos, int hpos, uae_u16 fmode);
extern void specialmonitor_reset(void);
extern bool specialmonitor_need_genlock(void);
extern bool specialmonitor_uses_control_lines(void);
extern bool specialmonitor_autoconfig_init(struct autoconfig_info*);
extern bool emulate_genlock(struct vidbuffer*, struct vidbuffer*, bool);
extern bool emulate_grayscale(struct vidbuffer*, struct vidbuffer*);
extern bool specialmonitor_linebased(void);
extern void genlock_infotext(uae_u8*, struct vidbuffer*);

extern const TCHAR *specialmonitorfriendlynames[];
extern const TCHAR *specialmonitormanufacturernames[];
extern const TCHAR *specialmonitorconfignames[];

#endif /* UAE_SPECIALMONITORS_H */
