#pragma once

#include <stdbool.h>
#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

const struct retro_vfs_interface* libretro_get_vfs_interface(void);
bool libretro_is_vfs_available(void);

#ifdef __cplusplus
}
#endif
