#ifndef LIBAMIGA_LIBAMIGA_H_
#define LIBAMIGA_LIBAMIGA_H_

// Clipboard functions

char* uae_clipboard_get_text(void);
void uae_clipboard_free_text(char* text);
void uae_clipboard_put_text(const char* text);

typedef const char * (*amiga_plugin_lookup_function)(const char *name);
void amiga_set_plugin_lookup_function(amiga_plugin_lookup_function function);

typedef int (*audio_callback)(int type, int16_t* buffer, int size);
int amiga_set_cd_audio_callback(audio_callback func);

#endif // LIBAMIGA_LIBAMIGA_H_