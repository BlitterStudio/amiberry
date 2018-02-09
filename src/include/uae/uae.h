#ifndef LIBAMIGA_LIBAMIGA_H_
#define LIBAMIGA_LIBAMIGA_H_

typedef const char * (*amiga_plugin_lookup_function)(const char *name);
void amiga_set_plugin_lookup_function(amiga_plugin_lookup_function function);

#endif // LIBAMIGA_LIBAMIGA_H_