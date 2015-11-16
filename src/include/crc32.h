extern uae_u32 get_crc32 (uae_u8 *p, int size);
extern uae_u16 get_crc16 (uae_u8 *p, int size);
extern uae_u32 get_crc32_val (uae_u8 v, uae_u32 crc);
extern void get_sha1 (uae_u8 *p, int size, uae_u8 *out);
extern char *get_sha1_txt (uae_u8 *p, int size);
#define SHA1_SIZE 20
