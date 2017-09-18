#ifndef UAE_MMAN_H
#define UAE_MMAN_H

#include "uae/types.h"

#define UAE_IPC_PRIVATE 0x01
#define UAE_IPC_RMID    0x02
#define UAE_IPC_CREAT   0x04
#define UAE_IPC_STAT    0x08

struct uae_mman_data
{
	uaecptr start;
	uae_u32 size;
	bool readonly;
	uae_u32 readonlysize;
	bool directsupport;
	bool hasbarrier;
};
bool uae_mman_info(addrbank *ab, struct uae_mman_data *md);

#endif /* UAE_MMAN_H */
