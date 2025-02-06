#ifndef _DEVICE_H_
#define _DEVICE_H_

#define CONFIG_STRING 0
#define CONFIG_INT 1
#define CONFIG_BINARY 2
#define CONFIG_SELECTION 3
#define CONFIG_MIDI 4

typedef struct device_config_selection_t
{
        char description[256];
        int value;
} device_config_selection_t;

typedef struct device_config_t
{
        char name[256];
        char description[256];
        int type;
        char default_string[256];
        int default_int;
        device_config_selection_t selection[16];
} device_config_t;

typedef struct device_t
{
    const char *name;
    const char *internal_name;
    uint32_t    flags; /* system flags */
    uintptr_t   local; /* flags local to device */

    union {
        void *(*init)(const struct device_t *);
        void *(*init_ext)(const struct device_t *, void *);
    };
    void (*close)(void *priv);
    void (*reset)(void *priv);
    union {
        int (*available)(void);
        int (*poll)(void *priv);
    };
    void (*speed_changed)(void *priv);
    void (*force_redraw)(void *priv);

    const device_config_t *config;
} device_t;

void device_init();
void *device_add(const device_t *d);
void device_close_all();
int device_available(device_t *d);
void device_speed_changed();
void device_force_redraw();
void device_add_status_info(char *s, int max_len);

extern char *current_device_name;

int device_get_config_int(const char *name);
char *device_get_config_string(const char *name);

enum {
    DEVICE_PCJR = 2,          /* requires an IBM PCjr */
    DEVICE_XTKBC = 4,          /* requires an XT-compatible keyboard controller */
    DEVICE_AT = 8,          /* requires an AT-compatible system */
    DEVICE_ATKBC = 0x10,       /* requires an AT-compatible keyboard controller */
    DEVICE_PS2 = 0x20,       /* requires a PS/1 or PS/2 system */
    DEVICE_ISA = 0x40,       /* requires the ISA bus */
    DEVICE_CBUS = 0x80,       /* requires the C-BUS bus */
    DEVICE_PCMCIA = 0x100,      /* requires the PCMCIA bus */
    DEVICE_MCA = 0x200,      /* requires the MCA bus */
    DEVICE_HIL = 0x400,      /* requires the HP HIL bus */
    DEVICE_EISA = 0x800,      /* requires the EISA bus */
    DEVICE_AT32 = 0x1000,     /* requires the Mylex AT/32 local bus */
    DEVICE_OLB = 0x2000,     /* requires the OPTi local bus */
    DEVICE_VLB = 0x4000,     /* requires the VLB bus */
    DEVICE_PCI = 0x8000,     /* requires the PCI bus */
    DEVICE_CARDBUS = 0x10000,    /* requires the CardBus bus */
    DEVICE_USB = 0x20000,    /* requires the USB bus */
    DEVICE_AGP = 0x40000,    /* requires the AGP bus */
    DEVICE_AC97 = 0x80000,    /* requires the AC'97 bus */
    DEVICE_COM = 0x100000,   /* requires a serial port */
    DEVICE_LPT = 0x200000,   /* requires a parallel port */
    DEVICE_KBC = 0x400000,   /* is a keyboard controller */
    DEVICE_SOFTRESET = 0x800000,   /* requires to be reset on soft reset */

    DEVICE_ONBOARD = 0x40000000, /* is on-board */
    DEVICE_PIT = 0x80000000, /* device is a PIT */

    DEVICE_ALL = 0xffffffff  /* match all devices */
};

int model_get_config_int(const char *s);
char *model_get_config_string(const char *s);

extern const device_t millennium_device;
extern const device_t millennium_ii_device;
extern const device_t mystique_device;
extern const device_t mystique_220_device;

#endif
