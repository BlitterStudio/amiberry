
static uae_u8 *inprec_buffer, *inprec_p;
static struct zfile *inprec_zf;
static int inprec_size;
int input_recording = 0;
static uae_u8 *inprec_plast, *inprec_plastptr;
static int inprec_div;

static uae_u32 oldbuttons[4];
static uae_u16 oldjoy[2];

int inprec_open(char *fname, int record)
{
    uae_u32 t = (uae_u32)time(0);
    int i;

    inprec_close();
    inprec_zf = zfile_fopen(fname, record > 0 ? "wb" : "rb");
    if (inprec_zf == NULL)
	return 0;
    inprec_size = 10000;
    inprec_div = 1;
    if (record < 0) {
	uae_u32 id;
	zfile_fseek (inprec_zf, 0, SEEK_END);
	inprec_size = zfile_ftell (inprec_zf);
	zfile_fseek (inprec_zf, 0, SEEK_SET);
	inprec_buffer = inprec_p = (uae_u8*)xmalloc (inprec_size);
	zfile_fread (inprec_buffer, inprec_size, 1, inprec_zf);
	inprec_plastptr = inprec_buffer;
	id = inprec_pu32();
	if (id != 'UAE\0') {
	    inprec_close();
	    return 0;
	}
	inprec_pu32();
	t = inprec_pu32();
	i = inprec_pu32();
	while (i-- > 0)
	    inprec_pu8();
	inprec_p = inprec_plastptr;
	oldbuttons[0] = oldbuttons[1] = oldbuttons[2] = oldbuttons[3] = 0;
	oldjoy[0] = oldjoy[1] = 0;
	if (record < -1)
	    inprec_div = maxvpos;
    } else if (record > 0) {
	inprec_buffer = inprec_p = (uae_u8*)xmalloc (inprec_size);
	inprec_ru32('UAE\0');
	inprec_ru8(1);
	inprec_ru8(UAEMAJOR);
	inprec_ru8(UAEMINOR);
	inprec_ru8(UAESUBREV);
	inprec_ru32(t);
	inprec_ru32(0); // extra header size
    } else {
	return 0;
    }
    input_recording = record;
    srand(t);
    CIA_inprec_prepare();
    write_log ("inprec initialized '%s', mode=%d\n", fname, input_recording);
    return 1;
}

void inprec_close(void)
{
    if (!inprec_zf)
	return;
    if (inprec_buffer && input_recording > 0) {
	hsync_counter++;
	inprec_rstart(INPREC_END);
	inprec_rend();
	hsync_counter--;
	zfile_fwrite (inprec_buffer, inprec_p - inprec_buffer, 1, inprec_zf);
	inprec_p = inprec_buffer;
    }
    zfile_fclose (inprec_zf);
    inprec_zf = NULL;
    xfree (inprec_buffer);
    inprec_buffer = NULL;
    input_recording = 0;
    write_log ("inprec finished\n");
}

void inprec_ru8(uae_u8 v)
{
    *inprec_p++= v;
}
void inprec_ru16(uae_u16 v)
{
    inprec_ru8((uae_u8)(v >> 8));
    inprec_ru8((uae_u8)v);
}
void inprec_ru32(uae_u32 v)
{
    inprec_ru16((uae_u16)(v >> 16));
    inprec_ru16((uae_u16)v);
}
void inprec_rstr(const char *s)
{
    while(*s) {
	inprec_ru8(*s);
	s++;
    }
    inprec_ru8(0);
}
void inprec_rstart(uae_u8 type)
{
    write_log ("INPREC: %08X: %d\n", hsync_counter, type);
    inprec_ru32(hsync_counter);
    inprec_ru8(0);
    inprec_plast = inprec_p;
    inprec_ru8(0xff);
    inprec_ru8(type);
}
void inprec_rend(void)
{
    *inprec_plast = inprec_p - (inprec_plast + 2);
    if (inprec_p >= inprec_buffer + inprec_size - 256) {
	zfile_fwrite (inprec_buffer, inprec_p - inprec_buffer, 1, inprec_zf);
	inprec_p = inprec_buffer;
    }
}

int inprec_pstart(uae_u8 type)
{
    uae_u8 *p = inprec_p;
    uae_u32 hc = hsync_counter;
    static uae_u8 *lastp;
    uae_u32 hc_orig, hc2_orig;

    if (savestate_state)
	return 0;
    if (p[5 + 1] == INPREC_END) {
	inprec_close();
	return 0;
    }
    hc_orig = hc;
    hc /= inprec_div;
    hc *= inprec_div;
    for (;;) {
	uae_u32 hc2 = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	if (p > lastp) {
	    write_log ("INPREC: Next %08x (%08x=%d): %d (%d)\n", hc2, hc, hc2 - hc, p[5 + 1], p[5]);
	    lastp = p;
	}
	hc2_orig = hc2;
	hc2 /= inprec_div;
	hc2 *= inprec_div;
	if (hc > hc2) {
	    write_log ("INPREC: %08x > %08x: %d (%d) missed!\n", hc, hc2, p[5 + 1], p[5]);
	    inprec_close();
	    return 0;
	}
	if (hc2 != hc) {
	    lastp = p;
	    break;
	}
	if (p[5 + 1] == type) {
	    write_log ("INPREC: %08x: %d (%d) (%+d)\n", hc, type, p[5], hc_orig - hc2_orig);
	    inprec_plast = p;
	    inprec_plastptr = p + 5 + 2;
	    return 1;
	}
	p += 5 + 2 + p[5];
    }
    inprec_plast = NULL;
    return 0;
}
void inprec_pend(void)
{
    uae_u8 *p = inprec_p;
    uae_u32 hc = hsync_counter;

    if (!inprec_plast)
	return;
    inprec_plast[5 + 1] = 0;
    inprec_plast = NULL;
    inprec_plastptr = NULL;
    hc /= inprec_div;
    hc *= inprec_div;
    for (;;) {
	uae_u32 hc2 = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	hc2 /= inprec_div;
	hc2 *= inprec_div;
	if (hc2 != hc)
	    break;
	if (p[5 + 1] != 0)
	    return;
	p += 5 + 2 + p[5];
    }
    inprec_p = p;
    if (p[5 + 1] == INPREC_END)
	inprec_close();
}

uae_u8 inprec_pu8(void)
{
    return *inprec_plastptr++;
}
uae_u16 inprec_pu16(void)
{
    uae_u16 v = inprec_pu8() << 8;
    v |= inprec_pu8();
    return v;
}
uae_u32 inprec_pu32(void)
{
    uae_u32 v = inprec_pu16() << 16;
    v |= inprec_pu16();
    return v;
}
int inprec_pstr(char *s)
{
    int len = 0;
    for(;;) {
	uae_u8 v = inprec_pu8();
	*s++ = v;
	if (!v)
	    break;
	len++;
    }
    return len;
}
