#ifndef UAE_X86_H
#define UAE_X86_H

bool a1060_init(struct autoconfig_info *aci);
bool a2088xt_init(struct autoconfig_info *aci);
bool a2088t_init(struct autoconfig_info *aci);
bool a2286_init(struct autoconfig_info *aci);
bool a2386_init(struct autoconfig_info *aci);
bool isa_expansion_init(struct autoconfig_info *aci);
void x86_bridge_sync_change(void);
void x86_update_sound(float);
bool x86_mouse(int port, int x, int y, int z, int b);

#define X86_STATE_INACTIVE 0
#define X86_STATE_STOP 1
#define X86_STATE_ACTIVE 2

int is_x86_cpu(struct uae_prefs*);

typedef void (*X86_INTERRUPT_CALLBACK)(int,bool);

uae_u8 x86_infloppy(int portnum);
void x86_outfloppy(int portnum, uae_u8 v);
void x86_initfloppy(X86_INTERRUPT_CALLBACK);
void x86_floppy_run(void);
uae_u16 floppy_get_raw_data(int*);

#endif /* UAE_X86_H */
