#ifndef UAE_UAERESOURCE_H
#define UAE_UAERESOURCE_H

#include "uae/types.h"

uaecptr uaeres_startup(TrapContext *ctx, uaecptr resaddr);
void uaeres_install (void);

#endif /* UAE_UAERESOURCE_H */
