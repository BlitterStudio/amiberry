
extern void pcem_close(void);
extern void pcemglue_hsync(void);
extern void pcemfreeaddeddevices(void);

uint8_t keyboard_at_read(uint16_t port, void *priv);
uint8_t mem_read_romext(uint32_t addr, void *priv);
uint16_t mem_read_romextw(uint32_t addr, void *priv);
uint32_t mem_read_romextl(uint32_t addr, void *priv);
void pcem_linear_mark(int offset);

extern int SOUNDBUFLEN;
extern int32_t *x86_sndbuffer[2];
extern bool x86_sndbuffer_filled[2];

extern void *pcem_mapping_linear_priv;
extern uae_u32 pcem_mapping_linear_offset;
extern uint8_t(*pcem_linear_read_b)(uint32_t addr, void* priv);
extern uint16_t(*pcem_linear_read_w)(uint32_t addr, void* priv);
extern uint32_t(*pcem_linear_read_l)(uint32_t addr, void* priv);
extern void (*pcem_linear_write_b)(uint32_t addr, uint8_t  val, void* priv);
extern void (*pcem_linear_write_w)(uint32_t addr, uint16_t val, void* priv);
extern void (*pcem_linear_write_l)(uint32_t addr, uint32_t val, void* priv);
