
extern int amiga_clipboard_want_data (void);
extern void amiga_clipboard_got_data (uaecptr data, uae_u32 size, uae_u32 actual);
extern void amiga_clipboard_die (void);
extern void amiga_clipboard_init (void);
extern uaecptr amiga_clipboard_proc_start (void);
extern void amiga_clipboard_task_start (uaecptr);
extern void clipboard_disable (bool);
extern void clipboard_vsync (void);