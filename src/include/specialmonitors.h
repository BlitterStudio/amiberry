#ifndef UAE_SPECIALMONITORS_H
#define UAE_SPECIALMONITORS_H

#include "memory.h"

bool emulate_specialmonitors(struct vidbuffer *src, struct vidbuffer *dst);
bool emulate_specialmonitors_line(struct vidbuffer *src, struct vidbuffer *dst, int line);
void specialmonitor_store_fmode(int vpos, int hpos, uae_u16 fmode);
void specialmonitor_reset(void);
bool specialmonitor_need_genlock(void);
bool specialmonitor_uses_control_lines(void);
bool specialmonitor_autoconfig_init(struct autoconfig_info*);
bool emulate_genlock(struct vidbuffer*, struct vidbuffer*, bool);
bool emulate_grayscale(struct vidbuffer*, struct vidbuffer*);
bool specialmonitor_linebased(void);

const TCHAR *specialmonitorfriendlynames[];
const TCHAR *specialmonitormanufacturernames[];
const TCHAR *specialmonitorconfignames[];

#endif /* UAE_SPECIALMONITORS_H */
