#ifndef LIBAMIGA_LIBAMIGA_H_
#define LIBAMIGA_LIBAMIGA_H_

// Clipboard functions

char* uae_clipboard_get_text(void);
void uae_clipboard_free_text(const char* text);
void uae_clipboard_put_text(const char* text);

#endif // LIBAMIGA_LIBAMIGA_H_