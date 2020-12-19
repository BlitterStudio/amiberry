#ifndef LIBAMIGA_LIBAMIGA_H_
#define LIBAMIGA_LIBAMIGA_H_

// Clipboard functions

char* uae_clipboard_get_text(void);
void uae_clipboard_free_text(char* text);
void uae_clipboard_put_text(const char* text);

typedef const char * (*amiga_plugin_lookup_function)(const char *name);
void amiga_set_plugin_lookup_function(amiga_plugin_lookup_function function);

#ifdef __cplusplus
extern "C" {
#endif
/* Sets uaem metadata write flags based on chars in flags. You only need
 * to call this function to set non-default behavior. */
void uae_set_uaem_write_flags_from_string(const char* flags);
#ifdef __cplusplus
} // extern "C"
#endif
#endif // LIBAMIGA_LIBAMIGA_H_