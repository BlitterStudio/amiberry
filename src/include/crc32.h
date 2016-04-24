extern uae_u32 get_crc32 (void *p, int size);
extern uae_u16 get_crc16 (void *p, int size);
extern uae_u32 get_crc32_val (uae_u8 v, uae_u32 crc);
extern void get_sha1 (void *p, int size, void *out);
extern const TCHAR *get_sha1_txt (void *p, int size);
#define SHA1_SIZE 20
