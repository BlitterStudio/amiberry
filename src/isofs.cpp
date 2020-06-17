/*
 * UAE - The Un*x Amiga Emulator
 *
 * Linux isofs/UAE filesystem wrapper
 *
 * Copyright 2012 Toni Wilen
 *
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "blkdev.h"
#include "isofs_api.h"
#include "uae/string.h"
#include "zfile.h"

#include "isofs.h"

#define MAX_CACHED_BH_COUNT 100
//#define MAX_CACHE_INODE_COUNT 10
#define HASH_SIZE 65536

#define CD_BLOCK_SIZE 2048
#define ISOFS_INVALID_MODE ((isofs_mode_t) -1)

#define ISOFS_I(x) (&x->ei)
#define ISOFS_SB(x) (&x->ei)
#define IS_ERR(x) (x == NULL)

#define XS_IFDIR        0x4000 
#define XS_IFCHR        0x2000
#define XS_IFIFO        0x1000
#define XS_IFREG        0x8000

#define XS_ISDIR(x) (x & XS_IFDIR)

struct buffer_head
{
	struct buffer_head *next;
	uae_u8 *b_data;
	uae_u32 b_blocknr;
	bool linked;
	int usecnt;
	struct super_block *sb;
};

struct inode
{
	struct inode *next;
	uae_u32 i_mode;
	isofs_uid_t i_uid;
	isofs_gid_t i_gid;
	uae_u32 i_ino;
	uae_u32 i_size;
	uae_u32 i_blocks;
	struct super_block *i_sb;
	timeval i_mtime;
	timeval i_atime;
	timeval i_ctime;
	iso_inode_info ei;
	TCHAR *name;
	int i_blkbits;
	bool linked;
	int usecnt;
	int lockcnt;
	bool i_isaflags;
	uae_u8 i_aflags;
	TCHAR *i_comment;
};


struct super_block
{
	int s_high_sierra;
	int s_blocksize;
	int s_blocksize_bits;
	isofs_sb_info ei;
	int unitnum;
	struct inode *inodes, *root;
	int inode_cnt;
	struct buffer_head *buffer_heads;
	int bh_count;
	bool unknown_media;
	struct inode *hash[HASH_SIZE];
	int hash_miss, hash_hit;
};

static int gethashindex(struct inode *inode)
{
	return inode->i_ino & (HASH_SIZE - 1);
}

static void free_inode(struct inode *inode)
{
	if (!inode)
		return;
	inode->i_sb->hash[gethashindex(inode)] = NULL;
	inode->i_sb->inode_cnt--;
	xfree(inode->name);
	xfree(inode->i_comment);
	xfree(inode);
}

static void free_bh(struct buffer_head *bh)
{
	if (!bh)
		return;
	bh->sb->bh_count--;
	xfree(bh->b_data);
	xfree(bh);
}

static void lock_inode(struct inode *inode)
{
	inode->lockcnt++;
}
static void unlock_inode(struct inode *inode)
{
	inode->lockcnt--;
}


static void iput(struct inode *inode)
{
	if (!inode || inode->linked)
		return;

	struct super_block *sb = inode->i_sb;

#if 0
	struct inode *in;
	while (inode->i_sb->inode_cnt > MAX_CACHE_INODE_COUNT) {
		/* not very fast but better than nothing.. */
		struct inode *minin = NULL, *mininprev = NULL;
		struct inode *prev = NULL;
		in = sb->inodes;
		while (in) {
			if (!in->lockcnt && (minin == NULL || in->usecnt < minin->usecnt)) {
				minin = in;
				mininprev = prev;
			}
			prev = in;
			in = in->next;
		}
		if (!minin)
			break;
		if (mininprev)
			mininprev->next = minin->next;
		else
			sb->inodes = minin->next;
		free_inode(minin);
	}
#endif
	inode->next = sb->inodes;
	sb->inodes = inode;
	inode->linked = true;
	sb->inode_cnt++;
	sb->hash[gethashindex(inode)] = inode;
}

static struct inode *find_inode(struct super_block *sb, uae_u64 uniq)
{
	struct inode *inode;

	inode = sb->hash[uniq & (HASH_SIZE - 1)];
	if (inode && inode->i_ino == uniq) {
		sb->hash_hit++;
		return inode;
	}
	sb->hash_miss++;

	inode = sb->inodes;
	while (inode) {
		if (inode->i_ino == uniq) {
			inode->usecnt++;
			return inode;
		}
		inode = inode->next;
	}
	return NULL;
}

static buffer_head *sb_bread(struct super_block *sb, uae_u32 block)
{
	struct buffer_head *bh;
	
	bh = sb->buffer_heads;
	while (bh) {
		if (bh->b_blocknr == block) {
			bh->usecnt++;
			return bh;
		}
		bh = bh->next;
	}
	// simple LRU block cache. Should be in blkdev, not here..
	while (sb->bh_count > MAX_CACHED_BH_COUNT) {
		struct buffer_head *minbh = NULL, *minbhprev = NULL;
		struct buffer_head *prev = NULL;
		bh = sb->buffer_heads;
		while (bh) {
			if (minbh == NULL || bh->usecnt < minbh->usecnt) {
				minbh = bh;
				minbhprev = prev;
			}
			prev = bh;
			bh = bh->next;
		}
		if (minbh) {
			if (minbhprev)
				minbhprev->next = minbh->next;
			else
				sb->buffer_heads = minbh->next;
			free_bh(minbh);
		}
	}
	bh = xcalloc (struct buffer_head, 1);
	bh->sb = sb;
	bh->b_data = xmalloc (uae_u8, CD_BLOCK_SIZE);
	bh->b_blocknr = block;
	if (sys_command_cd_read (sb->unitnum, bh->b_data, block, 1)) {
		bh->next = sb->buffer_heads;
		sb->buffer_heads = bh;
		bh->linked = true;
		sb->bh_count++;
		return bh;
	}
	xfree (bh);
	return NULL;
}

static void brelse(struct buffer_head *sh)
{
}

static TCHAR *getname(char *name, int len)
{
	TCHAR *s;
	char old;

	old = name[len];
	while (len > 0 && name[len - 1] == ' ')
		len--;
	name[len] = 0;
	s = au (name);
	name[len] = old;
	return s;
}

static inline uae_u32 isofs_get_ino(unsigned long block, unsigned long offset, unsigned long bufbits)
{
	return (block << (bufbits - 5)) | (offset >> 5);
}

static inline int isonum_711(char *p)
{
	return *(uae_u8*)p;
}
static inline int isonum_712(char *p)
{
	return *(uae_u8*)p;
}
static inline unsigned int isonum_721(char *pp)
{
	uae_u8 *p = (uae_u8*)pp;
	return p[0] | (p[1] << 8);
}
static inline unsigned int isonum_722(char *pp)
{
	uae_u8 *p = (uae_u8*)pp;
	return p[1] | (p[0] << 8);
}
static inline unsigned int isonum_723(char *pp)
{
	uae_u8 *p = (uae_u8*)pp;
	/* Ignore bigendian datum due to broken mastering programs */
	return p[0] | (p[1] << 8);
}
static inline unsigned int isonum_731(char *pp)
{
	uae_u8 *p = (uae_u8*)pp;
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
}
static inline unsigned int isonum_732(char *pp)
{
	uae_u8 *p = (uae_u8*)pp;
	return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | (p[0] << 0);
}
static inline unsigned int isonum_733(char *pp)
{
	uae_u8 *p = (uae_u8*)pp;
	/* Ignore bigendian datum due to broken mastering programs */
	return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | (p[0] << 0);
}

static void isofs_normalize_block_and_offset(struct iso_directory_record* de, unsigned long *block, unsigned long *offset)
{
#if 0
	/* Only directories are normalized. */
	if (de->flags[0] & 2) {
		*offset = 0;
		*block = (unsigned long)isonum_733(de->extent)
			+ (unsigned long)isonum_711(de->ext_attr_length);
	}
#endif
}

static int make_date(int year, int month, int day, int hour, int minute, int second, int tz)
{
	int crtime, days, i;

	if (year < 0) {
		crtime = 0;
	} else {
		int monlen[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

		days = year * 365;
		if (year > 2)
			days += (year+1) / 4;
		for (i = 1; i < month; i++)
			days += monlen[i-1];
		if (((year+2) % 4) == 0 && month > 2)
			days++;
		days += day - 1;
		crtime = ((((days * 24) + hour) * 60 + minute) * 60)
			+ second;

		/* sign extend */
		if (tz & 0x80)
			tz |= (-1 << 8);
		
		/* 
		 * The timezone offset is unreliable on some disks,
		 * so we make a sanity check.  In no case is it ever
		 * more than 13 hours from GMT, which is 52*15min.
		 * The time is always stored in localtime with the
		 * timezone offset being what get added to GMT to
		 * get to localtime.  Thus we need to subtract the offset
		 * to get to true GMT, which is what we store the time
		 * as internally.  On the local system, the user may set
		 * their timezone any way they wish, of course, so GMT
		 * gets converted back to localtime on the receiving
		 * system.
		 *
		 * NOTE: mkisofs in versions prior to mkisofs-1.10 had
		 * the sign wrong on the timezone offset.  This has now
		 * been corrected there too, but if you are getting screwy
		 * results this may be the explanation.  If enough people
		 * complain, a user configuration option could be added
		 * to add the timezone offset in with the wrong sign
		 * for 'compatibility' with older discs, but I cannot see how
		 * it will matter that much.
		 *
		 * Thanks to kuhlmav@elec.canterbury.ac.nz (Volker Kuhlmann)
		 * for pointing out the sign error.
		 */
		if (-52 <= tz && tz <= 52)
			crtime -= tz * 15 * 60;
	}
	return crtime;
}

/* 
 * We have to convert from a MM/DD/YY format to the Unix ctime format.
 * We have to take into account leap years and all of that good stuff.
 * Unfortunately, the kernel does not have the information on hand to
 * take into account daylight savings time, but it shouldn't matter.
 * The time stored should be localtime (with or without DST in effect),
 * and the timezone offset should hold the offset required to get back
 * to GMT.  Thus  we should always be correct.
 */

static int iso_date(char * p, int flag)
{
	int year, month, day, hour, minute, second, tz;

	year = p[0] - 70;
	month = p[1];
	day = p[2];
	hour = p[3];
	minute = p[4];
	second = p[5];
	if (flag == 0) tz = p[6]; /* High sierra has no time zone */
	else tz = 0;
	
	return make_date(year, month, day, hour, minute, second, tz);
}

static int iso_ltime(char *p)
{
	int year, month, day, hour, minute, second;
	char t;

	t = p[4];
	p[4] = 0;
	year = atol(p);
	p[4] = t;
	t = p[6];
	p[6] = 0;
	month = atol(p + 4);
	p[6] = t;
	t = p[8];
	p[8] = 0;
	day = atol(p + 6);
	p[8] = t;
	t = p[10];
	p[10] = 0;
	hour = atol(p + 8);
	p[10] = t;
	t = p[12];
	p[12] = 0;
	minute = atol(p + 10);
	p[12] = t;
	t = p[14];
	p[14] = 0;
	second = atol(p + 12);
	p[14] = t;

	return make_date(year - 1970, month, day, hour, minute, second, 0);
}

static int isofs_read_level3_size(struct inode *inode)
{
	unsigned long bufsize = ISOFS_BUFFER_SIZE(inode);
	int high_sierra = ISOFS_SB(inode->i_sb)->s_high_sierra;
	struct buffer_head *bh = NULL;
	unsigned long block, offset, block_saved, offset_saved;
	int i = 0;
	int more_entries = 0;
	struct iso_directory_record *tmpde = NULL;
	struct iso_inode_info *ei = ISOFS_I(inode);

	inode->i_size = 0;

	/* The first 16 blocks are reserved as the System Area.  Thus,
	 * no inodes can appear in block 0.  We use this to flag that
	 * this is the last section. */
	ei->i_next_section_block = 0;
	ei->i_next_section_offset = 0;

	block = ei->i_iget5_block;
	offset = ei->i_iget5_offset;

	do {
		struct iso_directory_record *de;
		unsigned int de_len;

		if (!bh) {
			bh = sb_bread(inode->i_sb, block);
			if (!bh)
				goto out_noread;
		}
		de = (struct iso_directory_record *) (bh->b_data + offset);
		de_len = *(unsigned char *) de;

		if (de_len == 0) {
			brelse(bh);
			bh = NULL;
			++block;
			offset = 0;
			continue;
		}

		block_saved = block;
		offset_saved = offset;
		offset += de_len;

		/* Make sure we have a full directory entry */
		if (offset >= bufsize) {
			int slop = bufsize - offset + de_len;
			if (!tmpde) {
				tmpde = (struct iso_directory_record*)xmalloc(uae_u8, 256);
				if (!tmpde)
					goto out_nomem;
			}
			memcpy(tmpde, de, slop);
			offset &= bufsize - 1;
			block++;
			brelse(bh);
			bh = NULL;
			if (offset) {
				bh = sb_bread(inode->i_sb, block);
				if (!bh)
					goto out_noread;
				memcpy((uae_u8*)tmpde+slop, bh->b_data, offset);
			}
			de = tmpde;
		}

		inode->i_size += isonum_733(de->size);
		if (i == 1) {
			ei->i_next_section_block = block_saved;
			ei->i_next_section_offset = offset_saved;
		}

		more_entries = de->flags[-high_sierra] & 0x80;

		i++;
		if (i > 100)
			goto out_toomany;
	} while (more_entries);
out:
	xfree(tmpde);
	if (bh)
		brelse(bh);
	return 0;

out_nomem:
	if (bh)
		brelse(bh);
	return -ENOMEM;

out_noread:
	write_log (_T("ISOFS: unable to read i-node block %lu\n"), block);
	xfree(tmpde);
	return -EIO;

out_toomany:
	write_log (_T("ISOFS: More than 100 file sections ?!?, aborting... isofs_read_level3_size: inode=%u\n"), inode->i_ino);
	goto out;
}

static int parse_rock_ridge_inode(struct iso_directory_record *de, struct inode *inode);

static int isofs_read_inode(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	struct isofs_sb_info *sbi = ISOFS_SB(sb);
	unsigned long bufsize = ISOFS_BUFFER_SIZE(inode);
	unsigned long block;
	int high_sierra = sbi->s_high_sierra;
	struct buffer_head *bh = NULL;
	struct iso_directory_record *de;
	struct iso_directory_record *tmpde = NULL;
	unsigned int de_len;
	unsigned long offset;
	struct iso_inode_info *ei = ISOFS_I(inode);
	int ret = -EIO;

	block = ei->i_iget5_block;
	bh = sb_bread(inode->i_sb, block);
	if (!bh)
		goto out_badread;

	offset = ei->i_iget5_offset;

	de = (struct iso_directory_record *) (bh->b_data + offset);
	de_len = *(unsigned char *) de;

	if (offset + de_len > bufsize) {
		int frag1 = bufsize - offset;

		tmpde = (struct iso_directory_record*)xmalloc (uae_u8, de_len);
		if (tmpde == NULL) {
			ret = -ENOMEM;
			goto fail;
		}
		memcpy(tmpde, bh->b_data + offset, frag1);
		brelse(bh);
		bh = sb_bread(inode->i_sb, ++block);
		if (!bh)
			goto out_badread;
		memcpy((char *)tmpde+frag1, bh->b_data, de_len - frag1);
		de = tmpde;
	}

	inode->i_ino = isofs_get_ino(ei->i_iget5_block, ei->i_iget5_offset, ISOFS_BUFFER_BITS(inode));

	/* Assume it is a normal-format file unless told otherwise */
	ei->i_file_format = isofs_file_normal;

	if (de->flags[-high_sierra] & 2) {
		if (sbi->s_dmode != ISOFS_INVALID_MODE)
			inode->i_mode = XS_IFDIR | sbi->s_dmode;
		else
			inode->i_mode = XS_IFDIR; // | S_IRUGO | S_IXUGO;
	} else {
		if (sbi->s_fmode != ISOFS_INVALID_MODE) {
			inode->i_mode = XS_IFREG | sbi->s_fmode;
		} else {
			/*
			 * Set default permissions: r-x for all.  The disc
			 * could be shared with DOS machines so virtually
			 * anything could be a valid executable.
			 */
			inode->i_mode = XS_IFREG; // | S_IRUGO | S_IXUGO;
		}
	}

	inode->i_uid = sbi->s_uid;
	inode->i_gid = sbi->s_gid;
	inode->i_blocks = 0;

	ei->i_format_parm[0] = 0;
	ei->i_format_parm[1] = 0;
	ei->i_format_parm[2] = 0;

	ei->i_section_size = isonum_733(de->size);
	if (de->flags[-high_sierra] & 0x80) {
		ret = isofs_read_level3_size(inode);
		if (ret < 0)
			goto fail;
		// FIXME: this value is never used (?), because it is overwritten
		// with ret = 0 further down.
		ret = -EIO;
	} else {
		ei->i_next_section_block = 0;
		ei->i_next_section_offset = 0;
		inode->i_size = isonum_733(de->size);
	}

	/*
	 * Some dipshit decided to store some other bit of information
	 * in the high byte of the file length.  Truncate size in case
	 * this CDROM was mounted with the cruft option.
	 */

	if (sbi->s_cruft)
		inode->i_size &= 0x00ffffff;

	if (de->interleave[0]) {
		write_log (_T("ISOFS: Interleaved files not (yet) supported.\n"));
		inode->i_size = 0;
	}

	/* I have no idea what file_unit_size is used for, so
	   we will flag it for now */
	if (de->file_unit_size[0] != 0) {
		write_log (_T("ISOFS: File unit size != 0 for ISO file (%d).\n"), inode->i_ino);
	}

	/* I have no idea what other flag bits are used for, so
	   we will flag it for now */
	if((de->flags[-high_sierra] & ~2)!= 0){
		write_log (_T("ISOFS: Unusual flag settings for ISO file (%d %x).\n"), inode->i_ino, de->flags[-high_sierra]);
	}

	inode->i_mtime.tv_sec =
	inode->i_atime.tv_sec =
	inode->i_ctime.tv_sec = iso_date(de->date, high_sierra);
	inode->i_mtime.tv_usec =
	inode->i_atime.tv_usec =
	inode->i_ctime.tv_usec = 0;

	ei->i_first_extent = (isonum_733(de->extent) +
			isonum_711(de->ext_attr_length));

	/* Set the number of blocks for stat() - should be done before RR */
	inode->i_blocks = (inode->i_size + 511) >> 9;

	/*
	 * Now test for possible Rock Ridge extensions which will override
	 * some of these numbers in the inode structure.
	 */
	if (!high_sierra) {
		parse_rock_ridge_inode(de, inode);
		/* if we want uid/gid set, override the rock ridge setting */
		if (sbi->s_uid_set)
			inode->i_uid = sbi->s_uid;
		if (sbi->s_gid_set)
			inode->i_gid = sbi->s_gid;
	}

#if 0
	/* Now set final access rights if overriding rock ridge setting */
	if (XS_ISDIR(inode->i_mode) && sbi->s_overriderockperm &&
	    sbi->s_dmode != ISOFS_INVALID_MODE)
		inode->i_mode = XS_IFDIR | sbi->s_dmode;
	if (XS_ISREG(inode->i_mode) && sbi->s_overriderockperm &&
	    sbi->s_fmode != ISOFS_INVALID_MODE)
		inode->i_mode = XS_IFREG | sbi->s_fmode;
	/* Install the inode operations vector */
	if (XS_ISREG(inode->i_mode)) {
		inode->i_fop = &generic_ro_fops;
		switch (ei->i_file_format) {
#ifdef CONFIG_ZISOFS
		case isofs_file_compressed:
			inode->i_data.a_ops = &zisofs_aops;
			break;
#endif
		default:
			inode->i_data.a_ops = &isofs_aops;
			break;
		}
	} else if (XS_ISDIR(inode->i_mode)) {
		inode->i_op = &isofs_dir_inode_operations;
		inode->i_fop = &isofs_dir_operations;
	} else if (XS_ISLNK(inode->i_mode)) {
		inode->i_op = &page_symlink_inode_operations;
		inode->i_data.a_ops = &isofs_symlink_aops;
	} else
		/* XXX - parse_rock_ridge_inode() had already set i_rdev. */
		init_special_inode(inode, inode->i_mode, inode->i_rdev);
#endif

	ret = 0;
out:
	xfree(tmpde);
	if (bh)
		brelse(bh);
	return ret;

out_badread:
	write_log(_T("ISOFS: unable to read i-node block\n"));
fail:
	goto out;
}


static struct inode *isofs_iget(struct super_block *sb, unsigned long block, unsigned long offset, const TCHAR *name)
{
	struct inode *inode;
	uae_u32 id;

	if (offset >= 1ul << sb->s_blocksize_bits)
		return NULL;
	if (sb->root) {
		unsigned char bufbits = ISOFS_BUFFER_BITS(sb->root);
		id = isofs_get_ino(block, offset, bufbits);
		inode = find_inode (sb, id);
		if (inode)
			return inode;
	}
	inode = xcalloc(struct inode, 1);
	inode->name = name ? my_strdup(name) : NULL;
	inode->i_sb = sb;
	inode->ei.i_iget5_block = block;
	inode->ei.i_iget5_offset = offset;
	inode->i_blkbits = sb->s_blocksize_bits;
	isofs_read_inode(inode);
	return inode;
}


/**************************************************************

 ROCK RIDGE

 **************************************************************/


#define SIG(A,B) ((A) | ((B) << 8))	/* isonum_721() */

struct rock_state {
	void *buffer;
	unsigned char *chr;
	int len;
	int cont_size;
	int cont_extent;
	int cont_offset;
	struct inode *inode;
};

/*
 * This is a way of ensuring that we have something in the system
 * use fields that is compatible with Rock Ridge.  Return zero on success.
 */

static int check_sp(struct rock_ridge *rr, struct inode *inode)
{
	if (rr->u.SP.magic[0] != 0xbe)
		return -1;
	if (rr->u.SP.magic[1] != 0xef)
		return -1;
	ISOFS_SB(inode->i_sb)->s_rock_offset = rr->u.SP.skip;
	return 0;
}

static void setup_rock_ridge(struct iso_directory_record *de, struct inode *inode, struct rock_state *rs)
{
	rs->len = sizeof(struct iso_directory_record) + de->name_len[0];
	if (rs->len & 1)
		(rs->len)++;
	rs->chr = (unsigned char *)de + rs->len;
	rs->len = *((unsigned char *)de) - rs->len;
	if (rs->len < 0)
		rs->len = 0;

	if (ISOFS_SB(inode->i_sb)->s_rock_offset != -1) {
		rs->len -= ISOFS_SB(inode->i_sb)->s_rock_offset;
		rs->chr += ISOFS_SB(inode->i_sb)->s_rock_offset;
		if (rs->len < 0)
			rs->len = 0;
	}
}

static void init_rock_state(struct rock_state *rs, struct inode *inode)
{
	memset(rs, 0, sizeof(*rs));
	rs->inode = inode;
}

/*
 * Returns 0 if the caller should continue scanning, 1 if the scan must end
 * and -ve on error.
 */
static int rock_continue(struct rock_state *rs)
{
	int ret = 1;
	int blocksize = 1 << rs->inode->i_blkbits;
	const int min_de_size = offsetof(struct rock_ridge, u);

	xfree(rs->buffer);
	rs->buffer = NULL;

	if ((unsigned)rs->cont_offset > blocksize - min_de_size || (unsigned)rs->cont_size > blocksize || (unsigned)(rs->cont_offset + rs->cont_size) > blocksize) {
		write_log (_T("rock: corrupted directory entry. extent=%d, offset=%d, size=%d\n"), rs->cont_extent, rs->cont_offset, rs->cont_size);
		ret = -EIO;
		goto out;
	}

	if (rs->cont_extent) {
		struct buffer_head *bh;

		rs->buffer = xmalloc(uae_u8, rs->cont_size);
		if (!rs->buffer) {
			ret = -ENOMEM;
			goto out;
		}
		ret = -EIO;
		bh = sb_bread(rs->inode->i_sb, rs->cont_extent);
		if (bh) {
			memcpy(rs->buffer, bh->b_data + rs->cont_offset, rs->cont_size);
			brelse(bh);
			rs->chr = (unsigned char*)rs->buffer;
			rs->len = rs->cont_size;
			rs->cont_extent = 0;
			rs->cont_size = 0;
			rs->cont_offset = 0;
			return 0;
		}
		write_log (_T("Unable to read rock-ridge attributes\n"));
	}
out:
	xfree(rs->buffer);
	rs->buffer = NULL;
	return ret;
}

/*
 * We think there's a record of type `sig' at rs->chr.  Parse the signature
 * and make sure that there's really room for a record of that type.
 */
static int rock_check_overflow(struct rock_state *rs, int sig)
{
	int len;

	switch (sig) {
	case SIG('S', 'P'):
		len = sizeof(struct SU_SP_s);
		break;
	case SIG('C', 'E'):
		len = sizeof(struct SU_CE_s);
		break;
	case SIG('E', 'R'):
		len = sizeof(struct SU_ER_s);
		break;
	case SIG('R', 'R'):
		len = sizeof(struct RR_RR_s);
		break;
	case SIG('P', 'X'):
		len = sizeof(struct RR_PX_s);
		break;
	case SIG('P', 'N'):
		len = sizeof(struct RR_PN_s);
		break;
	case SIG('S', 'L'):
		len = sizeof(struct RR_SL_s);
		break;
	case SIG('N', 'M'):
		len = sizeof(struct RR_NM_s);
		break;
	case SIG('C', 'L'):
		len = sizeof(struct RR_CL_s);
		break;
	case SIG('P', 'L'):
		len = sizeof(struct RR_PL_s);
		break;
	case SIG('T', 'F'):
		len = sizeof(struct RR_TF_s);
		break;
	case SIG('Z', 'F'):
		len = sizeof(struct RR_ZF_s);
		break;
	case SIG('A', 'S'):
		len = sizeof(struct RR_AS_s);
		break;
	default:
		len = 0;
		break;
	}
	len += offsetof(struct rock_ridge, u);
	if (len > rs->len) {
		write_log(_T("rock: directory entry would overflow storage\n"));
		write_log(_T("rock: sig=0x%02x, size=%d, remaining=%d\n"), sig, len, rs->len);
		return -EIO;
	}
	return 0;
}

/*
 * return length of name field; 0: not found, -1: to be ignored
 */
static int get_rock_ridge_filename(struct iso_directory_record *de,
			    char *retname, struct inode *inode)
{
	struct rock_state rs;
	struct rock_ridge *rr;
	int sig;
	int retnamlen = 0;
	int truncate = 0;
	int ret = 0;

	if (!ISOFS_SB(inode->i_sb)->s_rock)
		return 0;
	*retname = 0;

	init_rock_state(&rs, inode);
	setup_rock_ridge(de, inode, &rs);
repeat:

	while (rs.len > 2) { /* There may be one byte for padding somewhere */
		rr = (struct rock_ridge *)rs.chr;
		/*
		 * Ignore rock ridge info if rr->len is out of range, but
		 * don't return -EIO because that would make the file
		 * invisible.
		 */
		if (rr->len < 3)
			goto out;	/* Something got screwed up here */
		sig = isonum_721((char*)rs.chr);
		if (rock_check_overflow(&rs, sig))
			goto eio;
		rs.chr += rr->len;
		rs.len -= rr->len;
		/*
		 * As above, just ignore the rock ridge info if rr->len
		 * is bogus.
		 */
		if (rs.len < 0)
			goto out;	/* Something got screwed up here */

		switch (sig) {
		case SIG('R', 'R'):
			if ((rr->u.RR.flags[0] & RR_NM) == 0)
				goto out;
			break;
		case SIG('S', 'P'):
			if (check_sp(rr, inode))
				goto out;
			break;
		case SIG('C', 'E'):
			rs.cont_extent = isonum_733(rr->u.CE.extent);
			rs.cont_offset = isonum_733(rr->u.CE.offset);
			rs.cont_size = isonum_733(rr->u.CE.size);
			break;
		case SIG('N', 'M'):
			if (truncate)
				break;
			if (rr->len < 5)
				break;
			/*
			 * If the flags are 2 or 4, this indicates '.' or '..'.
			 * We don't want to do anything with this, because it
			 * screws up the code that calls us.  We don't really
			 * care anyways, since we can just use the non-RR
			 * name.
			 */
			if (rr->u.NM.flags & 6)
				break;

			if (rr->u.NM.flags & ~1) {
				write_log(_T("Unsupported NM flag settings (%d)\n"), rr->u.NM.flags);
				break;
			}
			if ((strlen(retname) + rr->len - 5) >= 254) {
				truncate = 1;
				break;
			}
			strncat(retname, rr->u.NM.name, rr->len - 5);
			retnamlen += rr->len - 5;
			break;
		case SIG('R', 'E'):
			xfree(rs.buffer);
			return -1;
		default:
			break;
		}
	}
	ret = rock_continue(&rs);
	if (ret == 0)
		goto repeat;
	if (ret == 1)
		return retnamlen; /* If 0, this file did not have a NM field */
out:
	xfree(rs.buffer);
	return ret;
eio:
	ret = -EIO;
	goto out;
}

static int parse_rock_ridge_inode_internal(struct iso_directory_record *de, struct inode *inode, int regard_xa)
{
	int symlink_len = 0;
	int cnt, sig;
	struct inode *reloc;
	struct rock_ridge *rr;
	int rootflag;
	struct rock_state rs;
	int ret = 0;

	if (!ISOFS_SB(inode->i_sb)->s_rock)
		return 0;

	init_rock_state(&rs, inode);
	setup_rock_ridge(de, inode, &rs);
	if (regard_xa) {
		rs.chr += 14;
		rs.len -= 14;
		if (rs.len < 0)
			rs.len = 0;
	}

repeat:
	while (rs.len > 2) { /* There may be one byte for padding somewhere */
		rr = (struct rock_ridge *)rs.chr;
		/*
		 * Ignore rock ridge info if rr->len is out of range, but
		 * don't return -EIO because that would make the file
		 * invisible.
		 */
		if (rr->len < 3)
			goto out;	/* Something got screwed up here */
		sig = isonum_721((char*)rs.chr);
		if (rock_check_overflow(&rs, sig))
			goto eio;
		rs.chr += rr->len;
		rs.len -= rr->len;
		/*
		 * As above, just ignore the rock ridge info if rr->len
		 * is bogus.
		 */
		if (rs.len < 0)
			goto out;	/* Something got screwed up here */

		switch (sig) {
		case SIG('A', 'S'):
		{
			char *p = &rr->u.AS.data[0];
			if (rr->u.AS.flags & 1) { // PROTECTION
				inode->i_isaflags = true;
				inode->i_aflags = p[3];
				p += 4;
			}
			if (rr->u.AS.flags & 2) { // COMMENT
				const int maxcomment = 80;
				if (!inode->i_comment)
					inode->i_comment = xcalloc (TCHAR, maxcomment + 1);
				int l = p[0];
				char t = p[l];
				p[l] = 0;
				au_copy (inode->i_comment + _tcslen (inode->i_comment), maxcomment + 1 - _tcslen (inode->i_comment), p + 1);
				p[l] = t;
			}
			break;
		}
		case SIG('S', 'P'):
			if (check_sp(rr, inode))
				goto out;
			break;
		case SIG('C', 'E'):
			rs.cont_extent = isonum_733(rr->u.CE.extent);
			rs.cont_offset = isonum_733(rr->u.CE.offset);
			rs.cont_size = isonum_733(rr->u.CE.size);
			break;
		case SIG('E', 'R'):
			ISOFS_SB(inode->i_sb)->s_rock = 1;
			write_log(_T("ISO 9660 Extensions: "));
			{
				int p;
				for (p = 0; p < rr->u.ER.len_id; p++)
					write_log(_T("%c"), rr->u.ER.data[p]);
			}
			write_log(_T("\n"));
			break;
		case SIG('P', 'X'):
			inode->i_mode = isonum_733(rr->u.PX.mode);
			//set_nlink(inode, isonum_733(rr->u.PX.n_links));
			inode->i_uid = isonum_733(rr->u.PX.uid);
			inode->i_gid = isonum_733(rr->u.PX.gid);
			break;
		case SIG('P', 'N'):
			{
				int high, low;
				high = isonum_733(rr->u.PN.dev_high);
				low = isonum_733(rr->u.PN.dev_low);
				/*
				 * The Rock Ridge standard specifies that if
				 * sizeof(dev_t) <= 4, then the high field is
				 * unused, and the device number is completely
				 * stored in the low field.  Some writers may
				 * ignore this subtlety,
				 * and as a result we test to see if the entire
				 * device number is
				 * stored in the low field, and use that.
				 */
#if 0
				if ((low & ~0xff) && high == 0) {
					inode->i_rdev =
					    MKDEV(low >> 8, low & 0xff);
				} else {
					inode->i_rdev =
					    MKDEV(high, low);
				}
#endif
			}
			break;
		case SIG('T', 'F'):
			/*
			 * Some RRIP writers incorrectly place ctime in the
			 * TF_CREATE field. Try to handle this correctly for
			 * either case.
			 */
			/* Rock ridge never appears on a High Sierra disk */
			cnt = 0;
			if (rr->u.TF.flags & TF_CREATE) {
				inode->i_ctime.tv_sec =
				    iso_date(rr->u.TF.times[cnt++].time,
					     0);
				inode->i_ctime.tv_usec = 0;
			}
			if (rr->u.TF.flags & TF_MODIFY) {
				inode->i_mtime.tv_sec =
				    iso_date(rr->u.TF.times[cnt++].time,
					     0);
				inode->i_mtime.tv_usec = 0;
			}
			if (rr->u.TF.flags & TF_ACCESS) {
				inode->i_atime.tv_sec =
				    iso_date(rr->u.TF.times[cnt++].time,
					     0);
				inode->i_atime.tv_usec = 0;
			}
			if (rr->u.TF.flags & TF_ATTRIBUTES) {
				inode->i_ctime.tv_sec =
				    iso_date(rr->u.TF.times[cnt++].time,
					     0);
				inode->i_ctime.tv_usec = 0;
			}
			break;
		case SIG('S', 'L'):
			{
				int slen;
				struct SL_component *slp;
				struct SL_component *oldslp;
				slen = rr->len - 5;
				slp = &rr->u.SL.link;
				inode->i_size = symlink_len;
				while (slen > 1) {
					rootflag = 0;
					switch (slp->flags & ~1) {
					case 0:
						inode->i_size +=
						    slp->len;
						break;
					case 2:
						inode->i_size += 1;
						break;
					case 4:
						inode->i_size += 2;
						break;
					case 8:
						rootflag = 1;
						inode->i_size += 1;
						break;
					default:
						write_log(_T("Symlink component flag not implemented\n"));
					}
					slen -= slp->len + 2;
					oldslp = slp;
					slp = (struct SL_component *)
						(((char *)slp) + slp->len + 2);

					if (slen < 2) {
						if (((rr->u.SL.
						      flags & 1) != 0)
						    &&
						    ((oldslp->
						      flags & 1) == 0))
							inode->i_size +=
							    1;
						break;
					}

					/*
					 * If this component record isn't
					 * continued, then append a '/'.
					 */
					if (!rootflag
					    && (oldslp->flags & 1) == 0)
						inode->i_size += 1;
				}
			}
			symlink_len = inode->i_size;
			break;
		case SIG('R', 'E'):
			write_log(_T("Attempt to read inode for relocated directory\n"));
			goto out;
		case SIG('C', 'L'):
			ISOFS_I(inode)->i_first_extent = isonum_733(rr->u.CL.location);
			reloc = isofs_iget(inode->i_sb, ISOFS_I(inode)->i_first_extent, 0, NULL);
			if (IS_ERR(reloc)) {
				ret = -1; //PTR_ERR(reloc);
				goto out;
			}
			inode->i_mode = reloc->i_mode;
			//set_nlink(inode, reloc->i_nlink);
			inode->i_uid = reloc->i_uid;
			inode->i_gid = reloc->i_gid;
			//inode->i_rdev = reloc->i_rdev;
			inode->i_size = reloc->i_size;
			inode->i_blocks = reloc->i_blocks;
			inode->i_atime = reloc->i_atime;
			inode->i_ctime = reloc->i_ctime;
			inode->i_mtime = reloc->i_mtime;
			iput(reloc);
			break;
#ifdef CONFIG_ZISOFS
		case SIG('Z', 'F'): {
			int algo;

			if (ISOFS_SB(inode->i_sb)->s_nocompress)
				break;
			algo = isonum_721(rr->u.ZF.algorithm);
			if (algo == SIG('p', 'z')) {
				int block_shift =
					isonum_711(&rr->u.ZF.parms[1]);
				if (block_shift > 17) {
					printk(KERN_WARNING "isofs: "
						"Can't handle ZF block "
						"size of 2^%d\n",
						block_shift);
				} else {
					/*
					 * Note: we don't change
					 * i_blocks here
					 */
					ISOFS_I(inode)->i_file_format =
						isofs_file_compressed;
					/*
					 * Parameters to compression
					 * algorithm (header size,
					 * block size)
					 */
					ISOFS_I(inode)->i_format_parm[0] =
						isonum_711(&rr->u.ZF.parms[0]);
					ISOFS_I(inode)->i_format_parm[1] =
						isonum_711(&rr->u.ZF.parms[1]);
					inode->i_size =
					    isonum_733(rr->u.ZF.
						       real_size);
				}
			} else {
				printk(KERN_WARNING
				       "isofs: Unknown ZF compression "
						"algorithm: %c%c\n",
				       rr->u.ZF.algorithm[0],
				       rr->u.ZF.algorithm[1]);
			}
			break;
		}
#endif
		default:
			break;
		}
	}
	ret = rock_continue(&rs);
	if (ret == 0)
		goto repeat;
	if (ret == 1)
		ret = 0;
out:
	xfree(rs.buffer);
	return ret;
eio:
	ret = -EIO;
	goto out;
}

static char *get_symlink_chunk(char *rpnt, struct rock_ridge *rr, char *plimit)
{
	int slen;
	int rootflag;
	struct SL_component *oldslp;
	struct SL_component *slp;
	slen = rr->len - 5;
	slp = &rr->u.SL.link;
	while (slen > 1) {
		rootflag = 0;
		switch (slp->flags & ~1) {
		case 0:
			if (slp->len > plimit - rpnt)
				return NULL;
			memcpy(rpnt, slp->text, slp->len);
			rpnt += slp->len;
			break;
		case 2:
			if (rpnt >= plimit)
				return NULL;
			*rpnt++ = '.';
			break;
		case 4:
			if (2 > plimit - rpnt)
				return NULL;
			*rpnt++ = '.';
			*rpnt++ = '.';
			break;
		case 8:
			if (rpnt >= plimit)
				return NULL;
			rootflag = 1;
			*rpnt++ = '/';
			break;
		default:
			write_log(_T("Symlink component flag not implemented (%d)\n"), slp->flags);
		}
		slen -= slp->len + 2;
		oldslp = slp;
		slp = (struct SL_component *)((char *)slp + slp->len + 2);

		if (slen < 2) {
			/*
			 * If there is another SL record, and this component
			 * record isn't continued, then add a slash.
			 */
			if ((!rootflag) && (rr->u.SL.flags & 1) &&
			    !(oldslp->flags & 1)) {
				if (rpnt >= plimit)
					return NULL;
				*rpnt++ = '/';
			}
			break;
		}

		/*
		 * If this component record isn't continued, then append a '/'.
		 */
		if (!rootflag && !(oldslp->flags & 1)) {
			if (rpnt >= plimit)
				return NULL;
			*rpnt++ = '/';
		}
	}
	return rpnt;
}

static int parse_rock_ridge_inode(struct iso_directory_record *de, struct inode *inode)
{
	int result = parse_rock_ridge_inode_internal(de, inode, 0);

	/*
	 * if rockridge flag was reset and we didn't look for attributes
	 * behind eventual XA attributes, have a look there
	 */
	if ((ISOFS_SB(inode->i_sb)->s_rock_offset == -1)
	    && (ISOFS_SB(inode->i_sb)->s_rock == 2)) {
		result = parse_rock_ridge_inode_internal(de, inode, 14);
	}
	return result;
}

#if 0
/*
 * readpage() for symlinks: reads symlink contents into the page and either
 * makes it uptodate and returns 0 or returns error (-EIO)
 */
static int rock_ridge_symlink_readpage(struct file *file, struct page *page)
{
	struct inode *inode = page->mapping->host;
	struct iso_inode_info *ei = ISOFS_I(inode);
	struct isofs_sb_info *sbi = ISOFS_SB(inode->i_sb);
	char *link = kmap(page);
	unsigned long bufsize = ISOFS_BUFFER_SIZE(inode);
	struct buffer_head *bh;
	char *rpnt = link;
	unsigned char *pnt;
	struct iso_directory_record *raw_de;
	unsigned long block, offset;
	int sig;
	struct rock_ridge *rr;
	struct rock_state rs;
	int ret;

	if (!sbi->s_rock)
		goto error;

	init_rock_state(&rs, inode);
	block = ei->i_iget5_block;
	bh = sb_bread(inode->i_sb, block);
	if (!bh)
		goto out_noread;

	offset = ei->i_iget5_offset;
	pnt = (unsigned char *)bh->b_data + offset;

	raw_de = (struct iso_directory_record *)pnt;

	/*
	 * If we go past the end of the buffer, there is some sort of error.
	 */
	if (offset + *pnt > bufsize)
		goto out_bad_span;

	/*
	 * Now test for possible Rock Ridge extensions which will override
	 * some of these numbers in the inode structure.
	 */

	setup_rock_ridge(raw_de, inode, &rs);

repeat:
	while (rs.len > 2) { /* There may be one byte for padding somewhere */
		rr = (struct rock_ridge *)rs.chr;
		if (rr->len < 3)
			goto out;	/* Something got screwed up here */
		sig = isonum_721(rs.chr);
		if (rock_check_overflow(&rs, sig))
			goto out;
		rs.chr += rr->len;
		rs.len -= rr->len;
		if (rs.len < 0)
			goto out;	/* corrupted isofs */

		switch (sig) {
		case SIG('R', 'R'):
			if ((rr->u.RR.flags[0] & RR_SL) == 0)
				goto out;
			break;
		case SIG('S', 'P'):
			if (check_sp(rr, inode))
				goto out;
			break;
		case SIG('S', 'L'):
			rpnt = get_symlink_chunk(rpnt, rr,
						 link + (PAGE_SIZE - 1));
			if (rpnt == NULL)
				goto out;
			break;
		case SIG('C', 'E'):
			/* This tells is if there is a continuation record */
			rs.cont_extent = isonum_733(rr->u.CE.extent);
			rs.cont_offset = isonum_733(rr->u.CE.offset);
			rs.cont_size = isonum_733(rr->u.CE.size);
		default:
			break;
		}
	}
	ret = rock_continue(&rs);
	if (ret == 0)
		goto repeat;
	if (ret < 0)
		goto fail;

	if (rpnt == link)
		goto fail;
	brelse(bh);
	*rpnt = '\0';
	SetPageUptodate(page);
	kunmap(page);
	unlock_page(page);
	return 0;

	/* error exit from macro */
out:
	kfree(rs.buffer);
	goto fail;
out_noread:
	printk("unable to read i-node block");
	goto fail;
out_bad_span:
	printk("symlink spans iso9660 blocks\n");
fail:
	brelse(bh);
error:
	SetPageError(page);
	kunmap(page);
	unlock_page(page);
	return -EIO;
}

#endif


static TCHAR *get_joliet_name(char *name, unsigned char len, bool utf8)
{
	TCHAR *out;

	if (utf8) {
		/* probably never used */
		uae_char *o = xmalloc(uae_char, len + 1);
		for (int i = 0; i < len; i++)
			o[i] = name[i];
		o[len] = 0;
		out = utf8u(o);
		xfree(o);
	} else {
		len /= 2;
		out = xmalloc(TCHAR, len + 1);
		for (int i = 0; i < len; i++)
			out[i] = isonum_722(name + i * 2);
		out[len] = 0;
	}

	if ((len > 2) && (out[len - 2] == ';') && (out[len - 1] == '1')) {
		len -= 2;
		out[len] = 0;
	}

	/*
	* Windows doesn't like periods at the end of a name,
	* so neither do we
	*/
	while (len >= 2 && (out[len - 1] == '.')) {
		len--;
		out[len] = 0;
	}
	return out;
}

static TCHAR *get_joliet_filename(struct iso_directory_record * de, struct inode * inode)
{
	unsigned char utf8;
	//struct nls_table *nls;
	TCHAR *out;

	utf8 = ISOFS_SB(inode->i_sb)->s_utf8;
	//nls = ISOFS_SB(inode->i_sb)->s_nls_iocharset;

	out = get_joliet_name(de->name, de->name_len[0], utf8 != 0);
	return out;
}


/************************************************************ */



/*
 * Get a set of blocks; filling in buffer_heads if already allocated
 * or getblk() if they are not.  Returns the number of blocks inserted
 * (-ve == error.)
 */
static int isofs_get_blocks(struct inode *inode, uae_u32 iblock, struct buffer_head *bh, unsigned long nblocks)
{
	unsigned int b_off = iblock;
	unsigned offset, sect_size;
	unsigned int firstext;
	unsigned int nextblk, nextoff;
	int section, rv, error;
	struct iso_inode_info *ei = ISOFS_I(inode);

	error = -1;
	rv = 0;
#if 0
	if (iblock != b_off) {
		write(KERN_DEBUG "%s: block number too large\n", __func__);
		goto abort;
	}
#endif

	offset = 0;
	firstext = ei->i_first_extent;
	sect_size = ei->i_section_size >> ISOFS_BUFFER_BITS(inode);
	nextblk = ei->i_next_section_block;
	nextoff = ei->i_next_section_offset;
	section = 0;

	while (nblocks) {
		/* If we are *way* beyond the end of the file, print a message.
		 * Access beyond the end of the file up to the next page boundary
		 * is normal, however because of the way the page cache works.
		 * In this case, we just return 0 so that we can properly fill
		 * the page with useless information without generating any
		 * I/O errors.
		 */
		if (b_off > ((inode->i_size) >> ISOFS_BUFFER_BITS(inode))) {
			write_log (_T("ISOFS: block >= EOF (%u, %llu)\n"), b_off, (unsigned long long)inode->i_size);
			goto abort;
		}

		/* On the last section, nextblk == 0, section size is likely to
		 * exceed sect_size by a partial block, and access beyond the
		 * end of the file will reach beyond the section size, too.
		 */
		while (nextblk && (b_off >= (offset + sect_size))) {
			struct inode *ninode;

			offset += sect_size;
			ninode = isofs_iget(inode->i_sb, nextblk, nextoff, NULL);
			if (IS_ERR(ninode)) {
				//error = PTR_ERR(ninode);
				goto abort;
			}
			firstext  = ISOFS_I(ninode)->i_first_extent;
			sect_size = ISOFS_I(ninode)->i_section_size >> ISOFS_BUFFER_BITS(ninode);
			nextblk   = ISOFS_I(ninode)->i_next_section_block;
			nextoff   = ISOFS_I(ninode)->i_next_section_offset;
			iput(ninode);

			if (++section > 100) {
				write_log (_T("ISOFS: More than 100 file sections ?!? aborting...\n"));
				goto abort;
			}
		}

		if (bh) {
			bh->b_blocknr = firstext + b_off - offset;
		}
		bh++;	/* Next buffer head */
		b_off++;	/* Next buffer offset */
		nblocks--;
		rv++;
	}

	error = 0;
abort:
	return rv != 0 ? rv : -1;
}

static struct buffer_head *isofs_bread(struct inode *inode, uae_u32 block)
{
	struct buffer_head dummy[1];
	int error;
	
	error = isofs_get_blocks(inode, block, dummy, 1);
	if (error < 0)
		return NULL;
	return sb_bread(inode->i_sb, dummy[0].b_blocknr);
}


/*
 * Check if root directory is empty (has less than 3 files).
 *
 * Used to detect broken CDs where ISO root directory is empty but Joliet root
 * directory is OK. If such CD has Rock Ridge extensions, they will be disabled
 * (and Joliet used instead) or else no files would be visible.
 */
static bool rootdir_empty(struct super_block *sb, unsigned long block)
{
	int offset = 0, files = 0, de_len;
	struct iso_directory_record *de;
	struct buffer_head *bh;

	bh = sb_bread(sb, block);
	if (!bh)
		return true;
	while (files < 3) {
		de = (struct iso_directory_record *) (bh->b_data + offset);
		de_len = *(unsigned char *) de;
		if (de_len == 0)
			break;
		files++;
		offset += de_len;
	}
	brelse(bh);
	return files < 3;
}

/*
 * Initialize the superblock and read the root inode.
 *
 * Note: a check_disk_change() has been done immediately prior
 * to this call, so we don't need to check again.
 */
static int isofs_fill_super(struct super_block *s, void *data, int silent, uae_u64 *uniq)
{
	struct buffer_head *bh = NULL, *pri_bh = NULL;
	struct hs_primary_descriptor *h_pri = NULL;
	struct iso_primary_descriptor *pri = NULL;
	struct iso_supplementary_descriptor *sec = NULL;
	struct iso_directory_record *rootp;
	struct inode *inode;
	struct iso9660_options opt;
	struct isofs_sb_info *sbi;
	unsigned long first_data_zone;
	int joliet_level = 0;
	int iso_blknum, block;
	int orig_zonesize;
	int table, error = -EINVAL;
	unsigned int vol_desc_start;
	TCHAR *volume_name = NULL, *ch;
	uae_u32 volume_date;

	//save_mount_options(s, data);

	sbi = &s->ei;

	memset (&opt, 0, sizeof opt);
	//if (!parse_options((char *)data, &opt))
	//	goto out_freesbi;

	opt.blocksize = 2048;
	opt.map = 'n';
	opt.rock = 1;
	opt.joliet = 1;

	sbi->s_high_sierra = 0; /* default is iso9660 */

	vol_desc_start = 0;
#if 0
	struct device_info di;
	if (sys_command_info (s->unitnum, &di, true)) {
		vol_desc_start = di.toc.firstaddress;
	}
#endif

	for (iso_blknum = vol_desc_start+16; iso_blknum < vol_desc_start+100; iso_blknum++) {
		struct hs_volume_descriptor *hdp;
		struct iso_volume_descriptor  *vdp;

		block = iso_blknum << ISOFS_BLOCK_BITS;
		if (!(bh = sb_bread(s, block)))
			goto out_no_read;

		vdp = (struct iso_volume_descriptor *)bh->b_data;
		hdp = (struct hs_volume_descriptor *)bh->b_data;

		/*
		 * Due to the overlapping physical location of the descriptors,
		 * ISO CDs can match hdp->id==HS_STANDARD_ID as well. To ensure
		 * proper identification in this case, we first check for ISO.
		 */
		if (strncmp (vdp->id, ISO_STANDARD_ID, sizeof vdp->id) == 0) {
			if (isonum_711(vdp->type) == ISO_VD_END)
				break;
			if (isonum_711(vdp->type) == ISO_VD_PRIMARY) {
				if (pri == NULL) {
					pri = (struct iso_primary_descriptor *)vdp;
					/* Save the buffer in case we need it ... */
					pri_bh = bh;
					bh = NULL;
				}
			}
			else if (isonum_711(vdp->type) == ISO_VD_SUPPLEMENTARY) {
				sec = (struct iso_supplementary_descriptor *)vdp;
				if (sec->escape[0] == 0x25 && sec->escape[1] == 0x2f) {
					if (opt.joliet) {
						if (sec->escape[2] == 0x40)
							joliet_level = 1;
						else if (sec->escape[2] == 0x43)
							joliet_level = 2;
						else if (sec->escape[2] == 0x45)
							joliet_level = 3;

						write_log (_T("ISO 9660 Extensions: Microsoft Joliet Level %d\n"), joliet_level);
					}
					goto root_found;
				} else {
					/* Unknown supplementary volume descriptor */
					sec = NULL;
				}
			}
		} else {
			if (strncmp (hdp->id, HS_STANDARD_ID, sizeof hdp->id) == 0) {
				if (isonum_711(hdp->type) != ISO_VD_PRIMARY)
					goto out_freebh;

				sbi->s_high_sierra = 1;
				opt.rock = 0;
				h_pri = (struct hs_primary_descriptor *)vdp;
				goto root_found;
			}
		}

		/* Just skip any volume descriptors we don't recognize */

		brelse(bh);
		bh = NULL;
	}
	/*
	 * If we fall through, either no volume descriptor was found,
	 * or else we passed a primary descriptor looking for others.
	 */
	if (!pri)
		goto out_unknown_format;
	brelse(bh);
	bh = pri_bh;
	pri_bh = NULL;

root_found:

	if (joliet_level && (pri == NULL || !opt.rock)) {
		/* This is the case of Joliet with the norock mount flag.
		 * A disc with both Joliet and Rock Ridge is handled later
		 */
		pri = (struct iso_primary_descriptor *) sec;
	}

	if(sbi->s_high_sierra){
		rootp = (struct iso_directory_record *) h_pri->root_directory_record;
		sbi->s_nzones = isonum_733(h_pri->volume_space_size);
		sbi->s_log_zone_size = isonum_723(h_pri->logical_block_size);
		sbi->s_max_size = isonum_733(h_pri->volume_space_size);
	} else {
		if (!pri)
			goto out_freebh;
		rootp = (struct iso_directory_record *) pri->root_directory_record;
		sbi->s_nzones = isonum_733(pri->volume_space_size);
		sbi->s_log_zone_size = isonum_723(pri->logical_block_size);
		sbi->s_max_size = isonum_733(pri->volume_space_size);
	}

	sbi->s_ninodes = 0; /* No way to figure this out easily */

	orig_zonesize = sbi->s_log_zone_size;
	/*
	 * If the zone size is smaller than the hardware sector size,
	 * this is a fatal error.  This would occur if the disc drive
	 * had sectors that were 2048 bytes, but the filesystem had
	 * blocks that were 512 bytes (which should only very rarely
	 * happen.)
	 */
	if (orig_zonesize < opt.blocksize)
		goto out_bad_size;

	/* RDE: convert log zone size to bit shift */
	switch (sbi->s_log_zone_size) {
	case  512: sbi->s_log_zone_size =  9; break;
	case 1024: sbi->s_log_zone_size = 10; break;
	case 2048: sbi->s_log_zone_size = 11; break;

	default:
		goto out_bad_zone_size;
	}

	//s->s_magic = ISOFS_SUPER_MAGIC;

	/*
	 * With multi-extent files, file size is only limited by the maximum
	 * size of a file system, which is 8 TB.
	 */
	//s->s_maxbytes = 0x80000000000LL;

	/*
	 * The CDROM is read-only, has no nodes (devices) on it, and since
	 * all of the files appear to be owned by root, we really do not want
	 * to allow suid.  (suid or devices will not show up unless we have
	 * Rock Ridge extensions)
	 */

	//s->s_flags |= MS_RDONLY /* | MS_NODEV | MS_NOSUID */;

	/* Set this for reference. Its not currently used except on write
	   which we don't have .. */

	first_data_zone = isonum_733(rootp->extent) + isonum_711(rootp->ext_attr_length);
	sbi->s_firstdatazone = first_data_zone;

	write_log (_T("ISOFS: Max size:%ld   Log zone size:%ld\n"), sbi->s_max_size, 1UL << sbi->s_log_zone_size);
	write_log (_T("ISOFS: First datazone:%ld\n"), sbi->s_firstdatazone);
	if(sbi->s_high_sierra)
		write_log(_T("ISOFS: Disc in High Sierra format.\n"));
	ch = getname(pri->system_id, 4);
	write_log (_T("ISOFS: System ID: %s"), ch);
	xfree(ch);
	volume_name = getname(pri->volume_id, 32);
	volume_date = iso_ltime(pri->creation_date);
	write_log (_T(" Volume ID: '%s'\n"), volume_name);
	if (!strncmp(pri->system_id, ISO_SYSTEM_ID_CDTV, strlen(ISO_SYSTEM_ID_CDTV)))
		sbi->s_cdtv = 1;

	/*
	 * If the Joliet level is set, we _may_ decide to use the
	 * secondary descriptor, but can't be sure until after we
	 * read the root inode. But before reading the root inode
	 * we may need to change the device blocksize, and would
	 * rather release the old buffer first. So, we cache the
	 * first_data_zone value from the secondary descriptor.
	 */
	if (joliet_level) {
		pri = (struct iso_primary_descriptor *) sec;
		rootp = (struct iso_directory_record *)pri->root_directory_record;
		first_data_zone = isonum_733(rootp->extent) + isonum_711(rootp->ext_attr_length);
	}


	/*
	 * We're all done using the volume descriptor, and may need
	 * to change the device blocksize, so release the buffer now.
	 */
	brelse(pri_bh);
	brelse(bh);

#if 0
	if (joliet_level && opt.utf8 == 0) {
		char *p = opt.iocharset ? opt.iocharset : CONFIG_NLS_DEFAULT;
		sbi->s_nls_iocharset = load_nls(p);
		if (! sbi->s_nls_iocharset) {
			/* Fail only if explicit charset specified */
			if (opt.iocharset)
				goto out_freesbi;
			sbi->s_nls_iocharset = load_nls_default();
		}
	}
#endif
	//s->s_op = &isofs_sops;
	//s->s_export_op = &isofs_export_ops;

	sbi->s_mapping = opt.map;
	sbi->s_rock = (opt.rock ? 2 : 0);
	sbi->s_rock_offset = -1; /* initial offset, will guess until SP is found*/
	sbi->s_cruft = opt.cruft;
	sbi->s_hide = opt.hide;
	sbi->s_showassoc = opt.showassoc;
	sbi->s_uid = opt.uid;
	sbi->s_gid = opt.gid;
	sbi->s_uid_set = opt.uid_set;
	sbi->s_gid_set = opt.gid_set;
	sbi->s_utf8 = opt.utf8;
	sbi->s_nocompress = opt.nocompress;
	sbi->s_overriderockperm = opt.overriderockperm;

	/*
	 * Read the root inode, which _may_ result in changing
	 * the s_rock flag. Once we have the final s_rock value,
	 * we then decide whether to use the Joliet descriptor.
	 */
	inode = isofs_iget(s, sbi->s_firstdatazone, 0, NULL);
	if (IS_ERR(inode))
		goto out_no_root;


	/*
	 * Fix for broken CDs with Rock Ridge and empty ISO root directory but
	 * correct Joliet root directory.
	 */
	if (sbi->s_rock == 1 && joliet_level && rootdir_empty(s, sbi->s_firstdatazone)) {
		write_log(_T("ISOFS: primary root directory is empty. Disabling Rock Ridge and switching to Joliet.\n"));
		sbi->s_rock = 0;
	}

	/*
	 * If this disk has both Rock Ridge and Joliet on it, then we
	 * want to use Rock Ridge by default.  This can be overridden
	 * by using the norock mount option.  There is still one other
	 * possibility that is not taken into account: a Rock Ridge
	 * CD with Unicode names.  Until someone sees such a beast, it
	 * will not be supported.
	 */
	if (sbi->s_rock == 1) {
		joliet_level = 0;
		sbi->s_cdtv = 1; /* only convert if plain iso9660 */
	} else if (joliet_level) {
		sbi->s_rock = 0;
		sbi->s_cdtv = 1; /* only convert if plain iso9660 */
		if (sbi->s_firstdatazone != first_data_zone) {
			sbi->s_firstdatazone = first_data_zone;
			write_log (_T("ISOFS: changing to secondary root\n"));
			iput(inode);
			inode = isofs_iget(s, sbi->s_firstdatazone, 0, NULL);
			if (IS_ERR(inode))
				goto out_no_root;
			TCHAR *volname = get_joliet_name(pri->volume_id, 28, sbi->s_utf8);
			if (volname && _tcslen(volname) > 0) {
				xfree(volume_name);
				volume_name = volname;
				write_log(_T("ISOFS: Joliet Volume ID: '%s'\n"), volume_name);
			} else {
				xfree(volname);
			}
		}
	}

	if (opt.check == 'u') {
		/* Only Joliet is case insensitive by default */
		if (joliet_level)
			opt.check = 'r';
		else
			opt.check = 's';
	}
	sbi->s_joliet_level = joliet_level;

	/* Make sure the root inode is a directory */
	if (!XS_ISDIR(inode->i_mode)) {
		write_log (_T("isofs_fill_super: root inode is not a directory. Corrupted media?\n"));
		goto out_iput;
	}

	table = 0;
	if (joliet_level)
		table += 2;
	if (opt.check == 'r')
		table++;

	//s->s_d_op = &isofs_dentry_ops[table];

	/* get the root dentry */
	//s->s_root = d_alloc_root(inode);
	//if (!(s->s_root))
	//	goto out_no_root;

	//kfree(opt.iocharset);

	iput(inode);
	s->root = inode;
	inode->name = volume_name;
	inode->i_ctime.tv_sec = volume_date;
	*uniq = inode->i_ino;
	return 0;

	/*
	 * Display error messages and free resources.
	 */
out_iput:
	iput(inode);
	goto out_no_inode;
out_no_root:
	write_log (_T("ISOFS: get root inode failed\n"));
out_no_inode:
#ifdef CONFIG_JOLIET
	unload_nls(sbi->s_nls_iocharset);
#endif
	goto out_freesbi;
out_no_read:
	write_log (_T("ISOFS: bread failed, dev=%d, iso_blknum=%d, block=%d\n"), s->unitnum, iso_blknum, block);
	goto out_freebh;
out_bad_zone_size:
	write_log(_T("ISOFS: Bad logical zone size %ld\n"), sbi->s_log_zone_size);
	goto out_freebh;
out_bad_size:
	write_log (_T("ISOFS: Logical zone size(%d) < hardware blocksize(%u)\n"), orig_zonesize, opt.blocksize);
	goto out_freebh;
out_unknown_format:
	if (!silent)
		write_log (_T("ISOFS: Unable to identify CD-ROM format.\n"));
out_freebh:
	brelse(bh);
	brelse(pri_bh);
out_freesbi:
	xfree(volume_name);
	return error;
}

static int isofs_name_translate(struct iso_directory_record *de, char *newn, struct inode *inode)
{
	char * old = de->name;
	int len = de->name_len[0];
	int i;

	for (i = 0; i < len; i++) {
		unsigned char c = old[i];
		if (!c)
			break;

		if (!inode->i_sb->ei.s_cdtv) { /* keep case if Amiga/CDTV/CD32 */
			/* convert from second character (same as CacheCDFS default) */
			if (i > 0 && c >= 'A' && c <= 'Z')
				c |= 0x20;	/* lower case */
		}

		/* Drop trailing '.;1' (ISO 9660:1988 7.5.1 requires period) */
		if (c == '.' && i == len - 3 && old[i + 1] == ';' && old[i + 2] == '1')
			break;

		/* Drop trailing ';1' */
		if (c == ';' && i == len - 2 && old[i + 1] == '1')
			break;

		/* Convert remaining ';' to '.' */
		/* Also '/' to '.' (broken Acorn-generated ISO9660 images) */
		if (c == ';' || c == '/')
			c = '.';

		newn[i] = c;
	}
	return i;
}

static int isofs_cmp(const char *name, char *compare, int dlen)
{
	if (!compare)
		return 1;
	/* we don't care about special "." and ".." files */
	if (dlen == 1) {
		/* "." */
		if (compare[0] == 0) {
			return 1;
		} else if (compare[0] == 1) {
			return 1;
		}
	}
	char tmp = compare[dlen];
	compare[dlen] = 0;
	int c = stricmp(name, compare);
	compare[dlen] = tmp;
	return c;
}

static struct inode *isofs_find_entry(struct inode *dir, char *tmpname, TCHAR *tmpname2, struct iso_directory_record *tmpde, const char *name, const TCHAR *nameu)
{
	unsigned long bufsize = ISOFS_BUFFER_SIZE(dir);
	unsigned char bufbits = ISOFS_BUFFER_BITS(dir);
	unsigned long block, f_pos, offset, block_saved, offset_saved;
	struct buffer_head *bh = NULL;
	struct isofs_sb_info *sbi = ISOFS_SB(dir->i_sb);
	int i;
	TCHAR *jname;

	if (!ISOFS_I(dir)->i_first_extent)
		return 0;

	f_pos = 0;
	offset = 0;
	block = 0;

	while (f_pos < dir->i_size) {
		struct iso_directory_record *de;
		int de_len, match, dlen;
		char *dpnt;

		if (!bh) {
			bh = isofs_bread(dir, block);
			if (!bh)
				return 0;
		}

		de = (struct iso_directory_record *) (bh->b_data + offset);

		de_len = *(unsigned char *) de;
		if (!de_len) {
			brelse(bh);
			bh = NULL;
			f_pos = (f_pos + ISOFS_BLOCK_SIZE) & ~(ISOFS_BLOCK_SIZE - 1);
			block = f_pos >> bufbits;
			offset = 0;
			continue;
		}

		block_saved = bh->b_blocknr;
		offset_saved = offset;
		offset += de_len;
		f_pos += de_len;

		/* Make sure we have a full directory entry */
		if (offset >= bufsize) {
			int slop = bufsize - offset + de_len;
			memcpy((uae_u8*)tmpde, de, slop);
			offset &= bufsize - 1;
			block++;
			brelse(bh);
			bh = NULL;
			if (offset) {
				bh = isofs_bread(dir, block);
				if (!bh)
					return 0;
				memcpy((uae_u8*)tmpde + slop, bh->b_data, offset);
			}
			de = tmpde;
		}

		dlen = de->name_len[0];
		dpnt = de->name;
		/* Basic sanity check, whether name doesn't exceed dir entry */
		if (de_len < dlen + sizeof(struct iso_directory_record)) {
			write_log (_T("iso9660: Corrupted directory entry in block %lu of inode %u\n"), block, dir->i_ino);
			return 0;
		}

		jname = NULL;
		if (sbi->s_rock && ((i = get_rock_ridge_filename(de, tmpname, dir)))) {
			dlen = i;	/* possibly -1 */
			dpnt = tmpname;
		} else if (sbi->s_joliet_level) {
			jname = get_joliet_filename(de, dir);
		} else if (sbi->s_mapping == 'n') {
			dlen = isofs_name_translate(de, tmpname, dir);
			dpnt = tmpname;
		}

		/*
		 * Skip hidden or associated files unless hide or showassoc,
		 * respectively, is set
		 */
		match = 0;
		if (dlen > 0 && (!sbi->s_hide || (!(de->flags[-sbi->s_high_sierra] & 1))) && (sbi->s_showassoc || (!(de->flags[-sbi->s_high_sierra] & 4)))) {
			if (jname)
				match = _tcsicmp(jname, nameu) == 0;
			else
				match = isofs_cmp(name, dpnt, dlen) == 0;
		}
		xfree (jname);
		if (match) {
			isofs_normalize_block_and_offset(de, &block_saved, &offset_saved);
			struct inode *dinode = isofs_iget(dir->i_sb, block_saved, offset_saved, nameu);
			iput(dinode);
			brelse(bh);
			return dinode;
		}

	}
	brelse(bh);
	return 0;
}

/* Acorn extensions written by Matthew Wilcox <willy@bofh.ai> 1998 */
static int get_acorn_filename(struct iso_directory_record *de, char *retname, struct inode *inode)
{
	int std;
	unsigned char *chr;
	int retnamlen = isofs_name_translate(de, retname, inode);

	if (retnamlen == 0)
		return 0;
	std = sizeof(struct iso_directory_record) + de->name_len[0];
	if (std & 1)
		std++;
	if ((*((unsigned char *) de) - std) != 32)
		return retnamlen;
	chr = ((unsigned char *) de) + std;
	if (strncmp((char*)chr, "ARCHIMEDES", 10))
		return retnamlen;
	if ((*retname == '_') && ((chr[19] & 1) == 1))
		*retname = '!';
	if (((de->flags[0] & 2) == 0) && (chr[13] == 0xff) && ((chr[12] & 0xf0) == 0xf0)) {
		retname[retnamlen] = ',';
		sprintf(retname+retnamlen+1, "%3.3x",
			((chr[12] & 0xf) << 8) | chr[11]);
		retnamlen += 4;
	}
	return retnamlen;
}

struct file 
{
	uae_u32 f_pos;
};

static int do_isofs_readdir(struct inode *inode, struct file *filp, char *tmpname, struct iso_directory_record *tmpde, TCHAR *outname, uae_u64 *uniq)
{
	unsigned long bufsize = ISOFS_BUFFER_SIZE(inode);
	unsigned char bufbits = ISOFS_BUFFER_BITS(inode);
	unsigned long block, offset, block_saved, offset_saved;
	unsigned long inode_number = 0;	/* Quiet GCC */
	struct buffer_head *bh = NULL;
	int len;
	int map;
	int first_de = 1;
	char *p = NULL;		/* Quiet GCC */
	struct iso_directory_record *de;
	struct isofs_sb_info *sbi = ISOFS_SB(inode->i_sb);
	struct inode *dinode = NULL;
	int bh_block = 0;

	offset = filp->f_pos & (bufsize - 1);
	block = filp->f_pos >> bufbits;

	while (filp->f_pos < inode->i_size) {
		int de_len;

		if (!bh) {
			bh = isofs_bread(inode, block);
			if (!bh)
				return 0;
			bh_block = bh->b_blocknr;
		}

		de = (struct iso_directory_record *) (bh->b_data + offset);

		de_len = *(unsigned char *) de;

		/*
		 * If the length byte is zero, we should move on to the next
		 * CDROM sector.  If we are at the end of the directory, we
		 * kick out of the while loop.
		 */

		if (de_len == 0) {
			brelse(bh);
			bh = NULL;
			filp->f_pos = (filp->f_pos + ISOFS_BLOCK_SIZE) & ~(ISOFS_BLOCK_SIZE - 1);
			block = filp->f_pos >> bufbits;
			offset = 0;
			continue;
		}

		block_saved = block;
		offset_saved = offset;
		offset += de_len;

		/* Make sure we have a full directory entry */
		if (offset >= bufsize) {
			int slop = bufsize - offset + de_len;
			memcpy(tmpde, de, slop);
			offset &= bufsize - 1;
			block++;
			brelse(bh);
			bh = NULL;
			if (offset) {
				bh = isofs_bread(inode, block);
				if (!bh)
					return 0;
				memcpy((uae_u8*)tmpde + slop, bh->b_data, offset);
			}
			de = tmpde;
		}
		/* Basic sanity check, whether name doesn't exceed dir entry */
		if (de_len < de->name_len[0] + sizeof(struct iso_directory_record)) {
			write_log (_T("iso9660: Corrupted directory entry in block %lu of inode %u\n"), block, inode->i_ino);
			return 0;
		}

		if (first_de) {
			isofs_normalize_block_and_offset(de, &block_saved, &offset_saved);
			inode_number = isofs_get_ino(block_saved, offset_saved, bufbits);
		}

		if (de->flags[-sbi->s_high_sierra] & 0x80) {
			first_de = 0;
			filp->f_pos += de_len;
			continue;
		}
		first_de = 1;

		/* Handle the case of the '.' directory */
		if (de->name_len[0] == 1 && de->name[0] == 0) {
			filp->f_pos += de_len;
			continue;
		}
		len = 0;
		/* Handle the case of the '..' directory */
		if (de->name_len[0] == 1 && de->name[0] == 1) {
			filp->f_pos += de_len;
			continue;
		}
		/* Handle everything else.  Do name translation if there
		   is no Rock Ridge NM field. */

		/*
		 * Do not report hidden files if so instructed, or associated
		 * files unless instructed to do so
		 */
		if ((sbi->s_hide && (de->flags[-sbi->s_high_sierra] & 1)) || (!sbi->s_showassoc && (de->flags[-sbi->s_high_sierra] & 4))) {
			filp->f_pos += de_len;
			continue;
		}

		map = 1;
#if 1
		if (sbi->s_rock) {
			len = get_rock_ridge_filename(de, tmpname, inode);
			if (len != 0) {		/* may be -1 */
				p = tmpname;
				map = 0;
			}
		}
#endif
		TCHAR *jname = NULL;
		if (map) {
			if (sbi->s_joliet_level) {
				jname = get_joliet_filename(de, inode);
				len = 1;
			} else
			if (sbi->s_mapping == 'a') {
				len = get_acorn_filename(de, tmpname, inode);
				p = tmpname;
			} else
			if (sbi->s_mapping == 'n') {
				len = isofs_name_translate(de, tmpname, inode);
				p = tmpname;
			} else {
				p = de->name;
				len = de->name_len[0];
			}
		}

		filp->f_pos += de_len;
		if (len > 0) {
			if (jname == NULL) {
				if (p == NULL) {
					write_log(_T("ISOFS: no name copied (p == NULL)\n"));
					outname[0] = _T('\0');
				}
				else {
					char t = p[len];
					p[len] = 0;
					au_copy (outname, MAX_DPATH, p);
					p[len] = t;
				}
			} else {
				uae_tcslcpy (outname, jname, MAX_DPATH);
				xfree (jname);
			}
			dinode = isofs_iget(inode->i_sb, bh_block, offset_saved, outname);
			iput(dinode);
			*uniq = dinode->i_ino;
			brelse(bh);
			return 1;
		}

		continue;
	}
	brelse(bh);
	return 0;
}


void *isofs_mount(int unitnum, uae_u64 *uniq)
{
	struct super_block *sb;
	
	sb = xcalloc(struct super_block, 1);
	sb->s_blocksize = 2048;
	sb->s_blocksize_bits = 11;
	sb->unitnum = unitnum;
	if (sys_command_ismedia (unitnum, true)) {
		if (isofs_fill_super(sb, NULL, 0, uniq)) {
			sb->unknown_media = true;
		}
	}
	return sb;
}
void isofs_unmount(void *sbp)
{
	struct super_block *sb = (struct super_block*)sbp;
	struct inode *inode;
	struct buffer_head *bh;

	if (!sb)
		return;
	write_log (_T("miss: %d hit: %d\n"), sb->hash_miss, sb->hash_hit);
	inode = sb->inodes;
	while (inode) {
		struct inode *next = inode->next;
		free_inode(inode);
		inode = next;
	}
	bh = sb->buffer_heads;
	while (bh) {
		struct buffer_head *next = bh->next;
		free_bh(bh);
		bh = next;
	}
	xfree (sb);
}

bool isofs_mediainfo(void *sbp, struct isofs_info *ii)
{
	struct super_block *sb = (struct super_block*)sbp;
	
	memset (ii, 0, sizeof (struct isofs_info));

	if (!sb)
		return true;
	struct isofs_sb_info *sbi = ISOFS_SB(sb);
	ii->blocksize = 2048;
	if (sys_command_ismedia (sb->unitnum, true)) {
		struct device_info di;
		uae_u32 totalblocks = 0;
		ii->media = true;
		di.cylinders = 0;
		_stprintf (ii->devname, _T("CD%d"), sb->unitnum);
		if (sys_command_info (sb->unitnum, &di, true)) {
			totalblocks = di.cylinders * di.sectorspertrack * di.trackspercylinder;
			uae_tcslcpy (ii->devname, di.label, sizeof (ii->devname));
		}
		ii->unknown_media = sb->unknown_media;
		if (sb->root) {
			if (_tcslen(sb->root->name) == 0) {
				uae_tcslcpy(ii->volumename, _T("NO_LABEL"), sizeof(ii->volumename));
			} else {
				uae_tcslcpy (ii->volumename, sb->root->name, sizeof(ii->volumename));
			}
			ii->blocks = sbi->s_max_size;
			ii->totalblocks = totalblocks ? totalblocks : ii->blocks;
			ii->creation = sb->root->i_ctime.tv_sec;
		}
		if (!ii->volumename[0] || !ii->blocks)
			ii->unknown_media = true;
	}
	return true;
}

struct cd_opendir_s
{
	struct super_block *sb;
	struct inode *inode;
	struct file f;
	char tmp1[1024];
	char tmp2[1024];
};

struct cd_opendir_s *isofs_opendir(void *sb, uae_u64 uniq)
{
	struct cd_opendir_s *od = xcalloc(struct cd_opendir_s, 1);
	od->sb = (struct super_block*)sb;
	od->inode = find_inode(od->sb, uniq);
	if (od->inode) {
		lock_inode(od->inode);
		od->f.f_pos = 0;
		return od;
	}
	xfree(od);
	return NULL;
}
void isofs_closedir(struct cd_opendir_s *od)
{
	unlock_inode(od->inode);
	xfree (od);
}
bool isofs_readdir(struct cd_opendir_s *od, TCHAR *name, uae_u64 *uniq)
{
	return do_isofs_readdir(od->inode, &od->f, od->tmp1, (struct iso_directory_record*)od->tmp2, name, uniq) != 0;
}

void isofss_fill_file_attrs(void *sbp, uae_u64 parent, int *dir, int *flags, TCHAR **comment, uae_u64 uniq)
{
	struct super_block *sb = (struct super_block*)sbp;
	struct inode *inode = find_inode(sb, uniq);
	if (!inode)
		return;

	*comment = NULL;
	*dir = XS_ISDIR(inode->i_mode) ? 1 : 0;
	if (inode->i_isaflags)
		*flags = inode->i_aflags;
	else
		*flags = 0;
	if (inode->i_comment)
		*comment = my_strdup(inode->i_comment);
}

bool isofs_stat(void *sbp, uae_u64 uniq, struct mystat *statbuf)
{
	struct super_block *sb = (struct super_block*)sbp;
	struct inode *inode = find_inode(sb, uniq);

	if (!inode)
		return false;
	
	statbuf->mtime.tv_sec = inode->i_mtime.tv_sec;
	statbuf->mtime.tv_usec = 0;
	if (!XS_ISDIR(inode->i_mode)) {
		statbuf->size = inode->i_size;
	}
	return true;
}

bool isofs_exists(void *sbp, uae_u64 parent, const TCHAR *name, uae_u64 *uniq)
{
	char tmp1[1024];
	TCHAR tmp1x[1024];
	char tmp2[1024];
	char tmp3[1024];
	struct super_block *sb = (struct super_block*)sbp;
	struct inode *inode = find_inode(sb, parent);

	if (!inode)
		return false;
	ua_copy(tmp3, sizeof tmp3, name);
	inode = isofs_find_entry(inode, tmp1, tmp1x, (struct iso_directory_record*)tmp2, tmp3, name);
	if (inode) {
		*uniq = inode->i_ino;
		return true;
	}
	return false;
}

void isofs_dispose_inode(void *sbp, uae_u64 uniq)
{
	struct super_block *sb = (struct super_block*)sbp;
	struct inode *inode;
	struct inode *old = NULL, *prev = NULL;

	if (!sb)
		return;
	inode = sb->inodes;
	while (inode) {
		if (inode->i_ino == uniq) {
			old = inode;
			break;
		}
		prev = inode;
		inode = inode->next;
	}
	if (!old)
		return;

	if (prev)
		prev->next = old->next;
	else
		sb->inodes = old->next;
	free_inode(old);
}

struct cd_openfile_s
{
	struct super_block *sb;
	struct inode *inode;
	uae_s64 seek;
};

struct cd_openfile_s *isofs_openfile(void *sbp, uae_u64 uniq, int flags)
{
	struct super_block *sb = (struct super_block*)sbp;
	struct inode *inode = find_inode(sb, uniq);
	if (!inode)
		return NULL;
	struct cd_openfile_s *of = xcalloc(struct cd_openfile_s, 1);
	of->sb = sb;
	of->inode = inode;
	return of;
}

void isofs_closefile(struct cd_openfile_s *of)
{
	xfree(of);
}

uae_s64 isofs_lseek(struct cd_openfile_s *of, uae_s64 offset, int mode)
{
	struct inode *inode = of->inode;
	int ret = -1;
	switch (mode)
	{
	case SEEK_SET:
		of->seek = offset;
		break;
	case SEEK_CUR:
		of->seek += offset;
		break;
	case SEEK_END:
		of->seek = inode->i_size + offset;
		break;
	}
	if (of->seek < 0) {
		of->seek = 0;
		ret = -1;
	} else if (of->seek > inode->i_size) {
		of->seek = inode->i_size;
		ret = -1;
	} else {
		ret = of->seek;
	}
	return ret;
}
uae_s64 isofs_fsize(struct cd_openfile_s *of)
{
	struct inode *inode = of->inode;
	return inode->i_size;
}

uae_s64 isofs_read(struct cd_openfile_s *of, void *bp, unsigned int size)
{
	struct inode *inode = of->inode;
	uae_u32 bufsize = ISOFS_BUFFER_SIZE(inode);
	uae_u32 bufmask = bufsize - 1;
	uae_s64 offset = of->seek;
	struct buffer_head *bh;
	uae_u64 totalread = 0;
	uae_u32 read;
	uae_u8 *b = (uae_u8*)bp;

	if (size + of->seek > inode->i_size)
		size = inode->i_size - of->seek;

	// first partial sector
	if (offset & bufmask) {
		bh = isofs_bread(inode, offset / bufsize);
		if (!bh)
			return 0;
		read = size < (bufsize - (offset & bufmask)) ? size : (bufsize - (offset & bufmask));
		memcpy (b, bh->b_data + (offset & bufmask), read);
		offset += read;
		size -= read;
		totalread += read;
		b += read;
		of->seek += read;
		brelse(bh);
	}
	// complete sector(s)
	while (size >= bufsize) {
		bh = isofs_bread(inode, offset / bufsize);
		if (!bh)
			return totalread;
		read = size < bufsize ? size : bufsize;
		memcpy (b, bh->b_data, read);
		offset += read;
		size -= read;
		totalread += read;
		b += read;
		of->seek += read;
		brelse(bh);
	}
	// and finally last partial sector
	if (size > 0) {
		bh = isofs_bread(inode, offset / bufsize);
		if (!bh)
			return totalread;
		read = size;
		memcpy (b, bh->b_data, size);
		totalread += read;
		of->seek += read;
		brelse(bh);
	}

	return totalread;
}
