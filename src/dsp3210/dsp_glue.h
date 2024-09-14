
bool dsp_init(struct autoconfig_info *aci);
void dsp_write(uae_u8);
uae_u8 dsp_read(void);
void dsp_pause(int);

extern bool is_dsp_installed;
