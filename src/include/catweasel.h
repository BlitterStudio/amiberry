#ifndef UAE_CATWEASEL_H
#define UAE_CATWEASEL_H

#ifdef CATWEASEL

extern struct catweasel_contr cwc;
extern int catweasel_read_keyboard (uae_u8 *keycode);
extern int catweasel_init (void);
extern void catweasel_free (void);
extern int catweasel_detect (void);
extern uae_u32 catweasel_do_bget (uaecptr addr);
extern void catweasel_do_bput (uaecptr addr, uae_u32 b);
extern int catweasel_read_joystick (uae_u8 *dir, uae_u8 *buttons);
extern void catweasel_hsync (void);
extern int catweasel_isjoystick(void);
extern int catweasel_ismouse(void);
extern int catweasel_read_mouse(int port, int *dx, int *dy, int *buttons);

typedef struct catweasel_drive {
    struct catweasel_contr *contr; /* The controller this drive belongs to */
    int number;                    /* Drive number: 0 or 1 */
    int type;                      /* 0 = not present, 1 = 3.5" */
    int track;                     /* current r/w head position (0..79) */
    int diskindrive;               /* 0 = no disk, 1 = disk in drive */
    int wprot;                     /* 0 = not, 1 = write protected */
    unsigned char sel;
    unsigned char mot;
} catweasel_drive;

typedef struct catweasel_contr {
    int type;                      /* see CATWEASEL_TYPE_* defines below */
    int direct_access;
    int direct_type;
    int iobase;                    /* 0 = not present (factory default is 0x320) */
    void (*msdelay)(int ms);       /* microseconds delay routine, provided by host program */
    catweasel_drive drives[2];     /* at most two drives on each controller */
    int control_register;          /* contents of control register */
    unsigned char crm_sel0;        /* bit masks for the control / status register */
    unsigned char crm_sel1;
    unsigned char crm_mot0;
    unsigned char crm_mot1;
    unsigned char crm_dir;
    unsigned char crm_step;
    unsigned char srm_trk0;
    unsigned char srm_dchg;
    unsigned char srm_writ;
    unsigned char srm_dskready;
    int io_sr;                     /* IO port of control / status register */
    int io_mem;                    /* IO port of memory register */
    int sid[2];
    int can_sid, can_mouse, can_joy, can_kb;
} catweasel_contr;

#define CATWEASEL_TYPE_NONE  -1
#define CATWEASEL_TYPE_MK1    1
#define CATWEASEL_TYPE_MK3    3
#define CATWEASEL_TYPE_MK4    4

/* Initialize a Catweasel controller; c->iobase and c->msdelay must have
   been initialized -- msdelay might be used */
void catweasel_init_controller(catweasel_contr *c);

/* Reset the controller */
void catweasel_free_controller(catweasel_contr *c);

/* Set current drive select mask */
void catweasel_select(catweasel_contr *c, int dr0, int dr1);

/* Start/stop the drive's motor */
void catweasel_set_motor(catweasel_drive *d, int on);

/* Move the r/w head */
int catweasel_step(catweasel_drive *d, int dir);

/* Check for a disk change and update d->diskindrive
   -- msdelay might be used. Returns 1 == disk has been changed */
int catweasel_disk_changed(catweasel_drive *d);

/* Check if disk in selected drive is write protected. */
int catweasel_write_protected(catweasel_drive *d);

/* Read data -- msdelay will be used */
int catweasel_read(catweasel_drive *d, int side, int clock, int time);

/* Write data -- msdelay will be used. If time == -1, the write will
   be started at the index pulse and stopped at the next index pulse,
   or earlier if the Catweasel RAM contains a 128 end byte.  The
   function returns after the write has finished. */
int catweasel_write(catweasel_drive *d, int side, int clock, int time);

int catweasel_fillmfm (catweasel_drive *d, uae_u16 *mfm, int side, int clock, int rawmode);

int catweasel_diskready(catweasel_drive *d);
int catweasel_track0(catweasel_drive *d);


#endif /* CATWEASEL */

#endif /* UAE_CATWEASEL_H */
