
#define SIMULATE_SHIFT 0x200
#define SIMULATE_RELEASED_SHIFT 0x400

void inputmode_init(void);
void inputmode_redraw(void);

void set_joyConf(struct uae_prefs *p);

extern int show_inputmode;
