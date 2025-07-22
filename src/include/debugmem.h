
uaecptr debugmem_reloc(uaecptr exeaddress, uae_u32 len, uaecptr dbgaddress, uae_u32 dbglen, uaecptr task, uae_u32 *stack);
void debugmem_init(bool);
uaecptr debugmem_allocmem(int mode, uae_u32 size, uae_u32 flags, uae_u32 caller);
uae_u32 debugmem_freemem(int mode, uaecptr addr, uae_u32 size, uae_u32 caller);
void debugmem_trap(uaecptr addr);
void debugmem_addsegs(TrapContext *ctx, uaecptr seg, uaecptr name, uae_u32 lock, bool residentonly);
void debugmem_remsegs(uaecptr seg);
uae_u32 debugmem_exit(void);
bool debugmem_break(int);
bool debugmem_inhibit_break(int mode);
void debugmem_disable(void);
void debugmem_enable(void);
int debugmem_get_segment(uaecptr addr, bool *exact, bool *ext, TCHAR *out, TCHAR *name);
int debugmem_get_symbol(uaecptr addr, TCHAR *out, int maxsize);
bool debugmem_get_symbol_value(const TCHAR *name, uae_u32 *valp);
bool debugmem_list_segment(int mode, uaecptr addr);
int debugmem_get_sourceline(uaecptr addr, TCHAR *out, int maxsize);
bool debugmem_get_range(uaecptr*, uaecptr*);
bool debugmem_isactive(void);
bool debugger_load_libraries(void);
void debugger_scan_libraries(void);
bool debugger_get_library_symbol(uaecptr base, uaecptr addr, TCHAR *out);
bool debugmem_list_stackframe(bool super);
bool debugmem_break_stack_pop(void);
bool debugmem_break_stack_push(void);
bool debugmem_enable_stackframe(bool enable);
bool debugmem_illg(uae_u16);
void debugmem_flushcache(uaecptr, int);

extern uae_u32 debugmem_chiplimit;
extern uae_u32 debugmem_chiphit(uaecptr addr, uae_u32 v, int size);
extern bool debugmem_extinvalidmem(uaecptr addr, uae_u32 v, int size);

struct zfile *read_executable_rom(struct zfile*, int size, int blocks);
