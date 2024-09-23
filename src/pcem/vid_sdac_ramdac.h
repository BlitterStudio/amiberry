typedef struct sdac_ramdac_t
{
        int magic_count;
        uint8_t command;
        int windex, rindex;
        uint16_t regs[256];
        int reg_ff;
        int rs2;
} sdac_ramdac_t;

void sdac_init(sdac_ramdac_t *ramdac);

void sdac_ramdac_out(uint16_t addr, uint8_t val, sdac_ramdac_t *ramdac, svga_t *svga);
uint8_t sdac_ramdac_in(uint16_t addr, sdac_ramdac_t *ramdac, svga_t *svga);

float sdac_getclock(int clock, void *p);


typedef struct bt482_ramdac_t {
    uint32_t extpallook[4];
    uint8_t rgb[3];
    uint8_t  cursor32_data[256];
    int      hwc_y;
    int      hwc_x;
    uint8_t  cmd_r0;
    uint8_t  cmd_r1;
    uint8_t  cursor_r;
    uint8_t  status;
    uint8_t  type;
} bt482_ramdac_t;

void *bt482_ramdac_init(const device_t *info);
void bt482_ramdac_close(void *priv);
void bt482_hwcursor_draw(svga_t *svga, int displine);
void bt482_recalctimings(void *priv, svga_t *svga);
uint8_t bt482_ramdac_in(uint16_t addr, int rs2, void *priv, svga_t *svga);
void bt482_ramdac_out(uint16_t addr, int rs2, uint8_t val, void *priv, svga_t *svga);
