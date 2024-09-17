/*
* UAE - The Un*x Amiga Emulator
*
* LUA Scripting Layer
*
* Copyright 2013 Frode SOlheim
*/

#include "sysconfig.h"
#include "sysdeps.h"
#ifdef WITH_LUA
#include <lualib.h>
#endif

#include "options.h"
#include "savestate.h"
#include "memory.h"
#include "debug.h"
#include "identify.h"
#include "luascript.h"
#include "uae.h"
#include "zfile.h"
#include "threaddep/thread.h"

#ifdef WITH_LUA

static int g_num_states;
static lua_State *g_states[MAX_LUA_STATES];

static uae_sem_t lua_sem;

static int l_uae_read_u8(lua_State *L)
{
    int addr = luaL_checkint(L, 1);
    int value = debug_read_memory_8(addr);
    lua_pushinteger(L, value);
    return value >= 0 ? 1 : 0;
}

static int l_uae_write_u8(lua_State *L)
{
    int addr = luaL_checkint(L, 1);
    uint8_t value = luaL_checkint(L, 2);
    debug_write_memory_8(addr, value);
    return 0;
}
static int l_uae_write_u16(lua_State *L)
{
    int addr = luaL_checkint(L, 1);
    uint16_t value = luaL_checkint(L, 2);
    debug_write_memory_16(addr, value);
    return 0;
}

static int l_uae_read_u16(lua_State *L)
{
    int addr = luaL_checkint(L, 1);
    int value = debug_read_memory_16(addr);
    lua_pushinteger(L, value);
	return value >= 0 ? 1 : 0;
}

/* peek = read without any side-effects */
static int l_uae_peek_u16(lua_State *L)
{
    int result = 0;
	uint16_t value;
    int addr = luaL_checkint(L, 1);

	value = debug_peek_memory_16 (addr);
	if (value >= 0) {
        lua_pushinteger(L, value);
        result = 1;
    }
    return result;
}

static int l_uae_read_config(lua_State *L)
{
	int result = 0;
    const char *s = luaL_checkstring(L, 1);
	TCHAR *ts = au(s);
	TCHAR out[MAX_DPATH];
	if (cfgfile_searchconfig(ts, -1, out, sizeof out / sizeof(TCHAR)) == -1) {
		char *c = ua(out);
		lua_pushstring(L, c);
		xfree(c);
		result = 1;
	}
	xfree(ts);
	return result;
}

static int l_uae_write_config(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
	TCHAR *ts = au(s);
	TCHAR out[MAX_DPATH];
	cfgfile_modify(-1, ts, uaetcslen(ts), out, sizeof out / sizeof(TCHAR));
	char *c = ua(out);
	lua_pushstring(L, c);
	xfree(c);
	return 1;
}

static int l_uae_log(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    write_log("%s", s);
    //printf("%s", s);
    return 0;
}

void uae_lua_log_error(lua_State *L, const char *msg)
{
    write_log("%s: %s\n", msg, lua_tostring(L, -1));
    //printf("%s: %s\n", msg, lua_tostring(L, -1));
}

void uae_lua_aquire_lock()
{
	uae_sem_wait (&lua_sem);
}

void uae_lua_release_lock()
{
	uae_sem_post (&lua_sem);
}

void uae_lua_run_handler(const char *name)
{
    lua_State **L = g_states;
    while(*L) {
        uae_lua_aquire_lock();
        lua_getglobal(*L, name);
        if (lua_isnil(*L, -1)) {
            //lua_pop(*L, 1);
        }
        else if (lua_pcall(*L, 0, 0, 0) != 0) {
            uae_lua_log_error(*L, name);
            //lua_pop(*L, 1);
        }
        lua_settop(*L, 0);
        uae_lua_release_lock();
        L++;
    }
}

void uae_lua_load(const TCHAR *filename)
{
	char *fn;
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	fn = ua (filename);
	int err = luaL_loadfilex(L, fn, NULL);
	if (!err) {
		err = lua_pcall(L, 0, LUA_MULTRET, 0);
		if (!err) {
			write_log (_T("'%s' loaded\n"), filename);
			uae_lua_init_state (L);
		}
	}
	if (err)
		write_log (_T("'%s' initialization failed: %d\n"), err);
	xfree (fn);
}

void uae_lua_loadall(void)
{
	TCHAR tmp[MAX_DPATH];
	fetch_luapath (tmp, sizeof tmp / sizeof (TCHAR));
	_tcscat (tmp, _T("default.lua"));
	if (zfile_exists (tmp))
		uae_lua_load(tmp);
	for (int i = 0; i < MAX_LUA_STATES; i++) {
		if (currprefs.luafiles[i][0]) {
			uae_lua_load(currprefs.luafiles[i]);
		}
	}
}

void uae_lua_init_state(lua_State *L)
{
    write_log(_T("uae_lua_init_state %p\n"), L);
    if (g_num_states == MAX_LUA_STATES) {
        write_log(_T("WARNING: too many lua states (ignored this one)\n"));
        return;
    }
    g_states[g_num_states] = L;
    g_num_states++;

    lua_register(L, "uae_log", l_uae_log);

    lua_register(L, "uae_read_u8", l_uae_read_u8);
    lua_register(L, "uae_read_u16", l_uae_read_u16);
    lua_register(L, "uae_peek_u16", l_uae_peek_u16);
    lua_register(L, "uae_write_u8", l_uae_write_u8);
    lua_register(L, "uae_write_u16", l_uae_write_u16);

	lua_register(L, "uae_read_config", l_uae_read_config);
	lua_register(L, "uae_write_config", l_uae_write_config);

	for (int i = 0; custd[i].name; i++) {
		char *s = ua(custd[i].name);
		lua_pushinteger(L, custd[i].adr);
		lua_setglobal(L, s);
		xfree(s);
	}
}

void uae_lua_init(void)
{
	uae_sem_init (&lua_sem, 0, 1);
}

void uae_lua_free(void)
{
	for (int i = 0; i < g_num_states; i++) {
		lua_close(g_states[i]);
	}
	g_num_states = 0;
	uae_sem_destroy(&lua_sem);
}

#endif
