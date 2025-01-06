#ifndef UAE_MMAN_H
#define UAE_MMAN_H

#include "uae/types.h"

#define MAX_SHMID 256

typedef int uae_key_t;

/* One shmid data structure for each shared memory segment in the system. */
struct uae_shmid_ds {
	uae_key_t key;
	uae_u32 size;
	uae_u32 rosize;
	void *addr;
	TCHAR name[MAX_DPATH];
	void *attached;
	int mode;
	void *natmembase;
	bool fake;
	int maprom;
};

void *uae_shmat(addrbank *ab, int shmid, void *shmaddr, int shmflg, struct uae_mman_data *md);
int uae_shmdt(const void *shmaddr);
int uae_shmget(uae_key_t key, const addrbank *ab, int shmflg);
int uae_shmctl(int shmid, int cmd, struct uae_shmid_ds *buf);

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
	bool maprom;
	bool directsupport;
	bool hasbarrier;
};
bool uae_mman_info(addrbank *ab, struct uae_mman_data *md);

#endif /* UAE_MMAN_H */
