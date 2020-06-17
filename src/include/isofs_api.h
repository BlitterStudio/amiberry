#ifndef UAE_ISOFS_API_H
#define UAE_ISOFS_API_H

#include "uae/types.h"

struct cd_opendir_s;
struct cd_openfile_s;

struct isofs_info
{
	bool media;
	bool unknown_media;
	TCHAR volumename[256];
	TCHAR devname[256];
	uae_u32 blocks;
	uae_u32 totalblocks;
	uae_u32 blocksize;
	time_t creation;
};

void *isofs_mount(int unitnum, uae_u64 *uniq);
void isofs_unmount(void *sb);
bool isofs_mediainfo(void *sb, struct isofs_info*);
struct cd_opendir_s *isofs_opendir(void *sb, uae_u64 uniq);
void isofs_closedir(struct cd_opendir_s*);
bool isofs_readdir(struct cd_opendir_s*, TCHAR*, uae_u64 *uniq);
bool isofs_stat(void *sb, uae_u64, struct mystat*);
void isofss_fill_file_attrs(void *sb, uae_u64, int*, int*, TCHAR**, uae_u64);
bool isofs_exists(void *sb, uae_u64, const TCHAR*, uae_u64*);
void isofs_dispose_inode(void *sb, uae_u64);

struct cd_openfile_s *isofs_openfile(void*,uae_u64, int);
void isofs_closefile(struct cd_openfile_s*);
uae_s64 isofs_lseek(struct cd_openfile_s*, uae_s64, int);
uae_s64 isofs_fsize(struct cd_openfile_s*);
uae_s64 isofs_read(struct cd_openfile_s*, void*, unsigned int);

#endif /* UAE_ISOFS_API_H */
