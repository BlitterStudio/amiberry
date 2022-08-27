#pragma once
#include "uae/types.h"

uaecptr uaeserialdev_startup(TrapContext*, uaecptr resaddr);
void uaeserialdev_install(void);
void uaeserialdev_reset(void);
void uaeserialdev_start_threads(void);

extern int log_uaeserial;

struct uaeserialdata
{
#ifdef _WIN32
    void* handle;
    void* writeevent;
#endif
};