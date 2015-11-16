#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "zfile.h"
#include "zarchive.h"

#include "lha.h"

static char *methods[] =
{
	LZHUFF0_METHOD, LZHUFF1_METHOD, LZHUFF2_METHOD, LZHUFF3_METHOD,
	LZHUFF4_METHOD, LZHUFF5_METHOD, LZHUFF6_METHOD, LZHUFF7_METHOD,
	LARC_METHOD, LARC5_METHOD, LARC4_METHOD,
	LZHDIRS_METHOD,
	NULL
};

struct zvolume *archive_directory_lha(struct zfile *zf)
{
    struct zvolume *zv;
    struct zarchive_info zai;
    LzHeader hdr;
    int i;

    tzset();
    zv = zvolume_alloc(zf, ArchiveFormatLHA, NULL);
    while (get_header(zf, &hdr)) {
	struct znode *zn;
	int method;

	for (i = 0; methods[i]; i++) {
	    if (!strcmp(methods[i], hdr.method))
		method = i;
	}
	memset(&zai, 0, sizeof zai);
	zai.name = hdr.name;
	zai.size = hdr.original_size;
	zai.flags = hdr.attribute;
	zai.t = hdr.unix_last_modified_stamp -= timezone;
	if (hdr.name[strlen(hdr.name) + 1] != 0)
	    zai.comment = &hdr.name[strlen(hdr.name) + 1];
	if (method == LZHDIRS_METHOD_NUM) {
	    zvolume_adddir_abs(zv, &zai);
	} else {
	zn = zvolume_addfile_abs(zv, &zai);
	zn->offset = zfile_ftell(zf);
	zn->packedsize = hdr.packed_size;
	    zn->method = method;
	}
	zfile_fseek(zf, hdr.packed_size, SEEK_CUR);
	
    }
    return zv;
}

struct zfile *archive_access_lha(struct znode *zn)
{
    struct zfile *zf = zn->volume->archive;
    struct zfile *out = zfile_fopen_empty (zn->name, zn->size);
    struct interfacing lhinterface;

    zfile_fseek(zf, zn->offset, SEEK_SET);

    lhinterface.method = zn->method;
    lhinterface.dicbit = 13;	/* method + 8; -lh5- */
    lhinterface.infile = zf;
    lhinterface.outfile = out;
    lhinterface.original = zn->size;
    lhinterface.packed = zn->packedsize;

    switch (zn->method) {
	case LZHUFF0_METHOD_NUM:
	case LARC4_METHOD_NUM:
	    zfile_fread(out->data, zn->size, 1, zf);
	break;
	case LARC_METHOD_NUM:		/* -lzs- */
	    lhinterface.dicbit = 11;
	    decode(&lhinterface);
        break;
	case LZHUFF1_METHOD_NUM:		/* -lh1- */
	case LZHUFF4_METHOD_NUM:		/* -lh4- */
	case LARC5_METHOD_NUM:			/* -lz5- */
	    lhinterface.dicbit = 12;
	    decode(&lhinterface);
	break;
	case LZHUFF6_METHOD_NUM:		/* -lh6- */	/* Added N.Watazaki (^_^) */
	case LZHUFF7_METHOD_NUM:                /* -lh7- */
	    lhinterface.dicbit = (zn->method - LZHUFF6_METHOD_NUM) + 15;
	default:
	    decode(&lhinterface);
    }
    return out;
}
