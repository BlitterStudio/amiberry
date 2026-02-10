//
// Created by Dimitris Panokostas on 29/11/2023.
//

#ifdef USE_DBUS

#include "amiberry_dbus.h"
#include <iostream>
#include <cstring>
#include "inputdevice.h"
#include "memory.h"

using namespace Amiberry::DBus;

DBusConnection *dbusconn;

typedef std::function<void(DBusMessage*)> CommandHandler;
static std::map<std::string, CommandHandler> request_handlers;

static void SendReply(DBusMessage* msg, bool status, const std::vector<std::string>& extra_returns = {})
{
	DBusMessage *reply = nullptr;
	DBusMessageIter args;
	dbus_uint32_t serial = 0;

	if ((reply = dbus_message_new_method_return(msg)))
	{
		dbus_message_iter_init_append(reply, &args);
		dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &status);

		if (!extra_returns.empty())
		{
			for (const auto& r : extra_returns)
			{
				const char *cstr = r.c_str();
				dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &cstr);
			}
		}
		dbus_connection_send(dbusconn, reply, &serial);
		dbus_connection_flush(dbusconn);
		dbus_message_unref(reply);
	}
}

static void HandleQuit(DBusMessage* msg)
{
	std::cout << "DBUS: Received QUIT" << std::endl;
	SendReply(msg, true);  // Send reply before quitting
	uae_quit();
}

static void HandlePause(DBusMessage* msg)
{
	std::cout << "DBUS: Received PAUSE" << std::endl;
	setpaused(3);
	activationtoggle(0, true);
	SendReply(msg, true);
}

static void HandleResume(DBusMessage* msg)
{
	std::cout << "DBUS: Received RESUME" << std::endl;
	resumepaused(3);
	activationtoggle(0, false);
	SendReply(msg, true);
}

static void HandleReset(DBusMessage* msg)
{
	std::cout << "DBUS: Received RESET" << std::endl;
	DBusErrorWrapper err;
	char *type = nullptr;
	bool hard = false;
	bool keyboard = true;  // Default to keyboard (soft) reset
	bool status = true;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_STRING, &type, DBUS_TYPE_INVALID);
	// Optional argument - if not provided, defaults to keyboard reset

	if (type)
	{
		if (strcmp(type, "HARD") == 0)
		{
			hard = true;
			keyboard = false;
		}
		else if (strcmp(type, "SOFT") == 0 || strcmp(type, "KEYBOARD") == 0)
		{
			// Soft/keyboard reset (default behavior)
			hard = false;
			keyboard = true;
		}
		else
		{
			// Invalid reset type
			status = false;
		}
	}

	if (status)
	{
		uae_reset(hard, keyboard);
	}

	SendReply(msg, status);
}

static void HandleScreenshot(DBusMessage* msg)
{
	std::cout << "DBUS: Received SCREENSHOT" << std::endl;
	DBusErrorWrapper err;
	char *filename = nullptr;
	bool status = true;
	
	dbus_message_get_args(msg, &err.err, DBUS_TYPE_STRING, &filename, DBUS_TYPE_INVALID);
	
	if (err.is_set())
	{
		std::cout << "DBUS Arguments Error: " << err.err.message << std::endl;
		status = false;
	}
	else if (filename)
	{
		create_screenshot();
		save_thumb(filename);
	}
	else
	{
		status = false;
	}
	
	SendReply(msg, status);
}

static void HandleSavestate(DBusMessage* msg)
{
	std::cout << "DBUS: Received SAVESTATE" << std::endl;
	DBusErrorWrapper err;
	char *statefilename = nullptr;
	char *configfilename = nullptr;
	bool status = true;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_STRING, &statefilename, DBUS_TYPE_STRING, &configfilename, DBUS_TYPE_INVALID);

	if (err.is_set())
	{
		std::cout << "DBUS Arguments Error: " << err.err.message << std::endl;
		status = false;
	}
	else
	{
		std::cout << "SF: " << (statefilename ? statefilename : "null") << "\nCF: " << (configfilename ? configfilename : "null") << std::endl;
		if (!statefilename || !configfilename) {
			status = false;
		} else {
			savestate_initsave(statefilename, 1, true, true);
			save_state(statefilename, "...");
			
			strncpy(changed_prefs.description, "autosave", sizeof(changed_prefs.description));
			cfgfile_save(&changed_prefs, configfilename, 0);
		}
	}
	
	SendReply(msg, status);
}

static void HandleDiskSwap(DBusMessage* msg)
{
	std::cout << "DBUS: Received DISKSWAP" << std::endl;
	DBusErrorWrapper err;
	char *disknumstr = nullptr;
	char *drivenumstr = nullptr;
	bool status = true;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_STRING, &disknumstr, DBUS_TYPE_STRING, &drivenumstr, DBUS_TYPE_INVALID);

	if (err.is_set())
	{
		std::cout << "DBUS Arguments Error: " << err.err.message << std::endl;
		status = false;
	}
	else if (!disknumstr || !drivenumstr)
	{
		status = false;
	}
	else
	{
		int disknum = atol(disknumstr);
		int drivenum = atol(drivenumstr);

		if ((disknum >= 0) && (disknum < MAX_SPARE_DRIVES) && (drivenum >= 0) && (drivenum <= 3))
		{
			for (int i = 0; i < 4; i++) {
				if (!_tcscmp(currprefs.floppyslots[i].df, currprefs.dfxlist[disknum]))
					changed_prefs.floppyslots[i].df[0] = 0;
			}
			_tcscpy(changed_prefs.floppyslots[drivenum].df, currprefs.dfxlist[disknum]);
			set_config_changed();
		}
		else
		{
			status = false;
		}
	}

	SendReply(msg, status);
}

static void HandleQueryDiskSwap(DBusMessage* msg)
{
	std::cout << "DBUS: Received QUERYDISKSWAP" << std::endl;
	DBusErrorWrapper err;
	char *drivenumstr = nullptr;
	bool status = true;
	std::vector<std::string> responses;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_STRING, &drivenumstr, DBUS_TYPE_INVALID);

	if (err.is_set())
	{
		std::cout << "DBUS Arguments Error: " << err.err.message << std::endl;
		status = false;
	}
	else if (!drivenumstr)
	{
		status = false;
	}
	else
	{
		int drivenum = atol(drivenumstr);
		if ((drivenum >= 0) && (drivenum <= 3))
		{
			int disknum = -1;
			for (int i = 0; i < MAX_SPARE_DRIVES; i++)
			{
				if (!_tcscmp(currprefs.floppyslots[drivenum].df, currprefs.dfxlist[i]))
				{
					disknum = i;
					break;
				}
			}
			responses.push_back(std::to_string(disknum));
		}
		else
		{
			status = false;
		}
	}
	
	SendReply(msg, status, responses);
}

static void HandleInsertFloppy(DBusMessage* msg)
{
	std::cout << "DBUS: Received INSERTFLOPPY" << std::endl;
	DBusErrorWrapper err;
	char *diskpathstr = nullptr;
	char *drivenumstr = nullptr;
	bool status = true;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_STRING, &diskpathstr, DBUS_TYPE_STRING, &drivenumstr, DBUS_TYPE_INVALID);

	if (err.is_set())
	{
		std::cout << "DBUS Arguments Error: " << err.err.message << std::endl;
		status = false;
	}
	else if (!diskpathstr || !drivenumstr)
	{
		status = false;
	}
	else
	{
		int drivenum = atol(drivenumstr);

		if ((drivenum >= 0) && (drivenum <= 3))
		{
			_tcsncpy(changed_prefs.floppyslots[drivenum].df, diskpathstr, MAX_DPATH);
			changed_prefs.floppyslots[drivenum].df[MAX_DPATH - 1] = 0;
			set_config_changed();
		}
		else
		{
			status = false;
		}
	}
	
	SendReply(msg, status);
}

static void HandleInsertCD(DBusMessage* msg)
{
	std::cout << "DBUS: Received INSERTCD" << std::endl;
	DBusErrorWrapper err;
	char *diskpathstr = nullptr;
	bool status = true;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_STRING, &diskpathstr, DBUS_TYPE_INVALID);

	if (err.is_set())
	{
		std::cout << "DBUS Arguments Error: " << err.err.message << std::endl;
		status = false;
	}
	else if (!diskpathstr)
	{
		status = false;
	}
	else
	{
		_tcsncpy(changed_prefs.cdslots[0].name, diskpathstr, MAX_DPATH);
		changed_prefs.cdslots[0].name[MAX_DPATH - 1] = 0;
		changed_prefs.cdslots[0].inuse = true;

		std::cout << "CD Type: " << changed_prefs.cdslots[0].type << "\n";
		set_config_changed();
	}

	SendReply(msg, status);
}

static void HandleGetStatus(DBusMessage* msg)
{
	std::cout << "DBUS: Received GET_STATUS" << std::endl;
	bool status = true;
	std::vector<std::string> responses;

	// Paused
	responses.push_back(std::string("Paused=") + (pause_emulation ? "true" : "false"));
	
	// Config
	responses.push_back(std::string("Config=") + changed_prefs.description);
	
	// Floppies
	for(int i=0; i<4; ++i) {
		if (changed_prefs.floppyslots[i].df[0])
			responses.push_back(std::string("Floppy") + std::to_string(i) + "=" + changed_prefs.floppyslots[i].df);
	}
	
	SendReply(msg, status, responses);
}

static void HandleGetConfig(DBusMessage* msg)
{
	std::cout << "DBUS: Received GET_CONFIG" << std::endl;
	DBusErrorWrapper err;
	char *optname = nullptr;
	bool status = true;
	std::vector<std::string> responses;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_STRING, &optname, DBUS_TYPE_INVALID);

	if (err.is_set() || !optname) {
		status = false;
	} else {
		// Memory options
		if (strcmp(optname, "chipmem_size") == 0) {
			responses.push_back(std::to_string(changed_prefs.chipmem_size));
		} else if (strcmp(optname, "fastmem_size") == 0) {
			responses.push_back(std::to_string(changed_prefs.fastmem_size));
		} else if (strcmp(optname, "bogomem_size") == 0) {
			responses.push_back(std::to_string(changed_prefs.bogomem_size));
		} else if (strcmp(optname, "z3fastmem_size") == 0) {
			responses.push_back(std::to_string(changed_prefs.z3fastmem_size));
		}
		// CPU options
		else if (strcmp(optname, "cpu_model") == 0) {
			responses.push_back(std::to_string(changed_prefs.cpu_model));
		} else if (strcmp(optname, "cpu_speed") == 0) {
			responses.push_back(std::to_string(changed_prefs.m68k_speed));
		} else if (strcmp(optname, "cpu_compatible") == 0) {
			responses.push_back(changed_prefs.cpu_compatible ? "true" : "false");
		} else if (strcmp(optname, "cpu_24bit_addressing") == 0) {
			responses.push_back(changed_prefs.address_space_24 ? "true" : "false");
		}
		// Chipset options
		else if (strcmp(optname, "chipset") == 0) {
			responses.push_back(std::to_string(changed_prefs.chipset_mask));
		} else if (strcmp(optname, "ntsc") == 0) {
			responses.push_back(changed_prefs.ntscmode ? "true" : "false");
		}
		// Floppy options
		else if (strcmp(optname, "floppy_speed") == 0) {
			responses.push_back(std::to_string(changed_prefs.floppy_speed));
		} else if (strcmp(optname, "nr_floppies") == 0) {
			responses.push_back(std::to_string(changed_prefs.nr_floppies));
		}
		// Display options
		else if (strcmp(optname, "gfx_width") == 0) {
			responses.push_back(std::to_string(changed_prefs.gfx_monitor[0].gfx_size_win.width));
		} else if (strcmp(optname, "gfx_height") == 0) {
			responses.push_back(std::to_string(changed_prefs.gfx_monitor[0].gfx_size_win.height));
		} else if (strcmp(optname, "gfx_fullscreen") == 0) {
			responses.push_back(changed_prefs.gfx_apmode[0].gfx_fullscreen ? "true" : "false");
		}
		// Sound options
		else if (strcmp(optname, "sound_output") == 0) {
			responses.push_back(std::to_string(changed_prefs.produce_sound));
		} else if (strcmp(optname, "sound_stereo") == 0) {
			responses.push_back(std::to_string(changed_prefs.sound_stereo));
		}
		// Joystick options
		else if (strcmp(optname, "joyport0") == 0) {
			responses.push_back(std::to_string(changed_prefs.jports[0].id));
		} else if (strcmp(optname, "joyport1") == 0) {
			responses.push_back(std::to_string(changed_prefs.jports[1].id));
		}
		// Description
		else if (strcmp(optname, "description") == 0) {
			responses.push_back(changed_prefs.description);
		}
		else {
			// Unknown config option
			status = false;
		}
	}
	SendReply(msg, status, responses);
}

static void HandleSetConfig(DBusMessage* msg)
{
	std::cout << "DBUS: Received SET_CONFIG" << std::endl;
	DBusErrorWrapper err;
	char *optname = nullptr;
	char *optval = nullptr;
	bool status = true;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_STRING, &optname, DBUS_TYPE_STRING, &optval, DBUS_TYPE_INVALID);

	if (err.is_set() || !optname || !optval) {
		status = false;
	} else {
		// Floppy options
		if (strcmp(optname, "floppy_speed") == 0) {
			changed_prefs.floppy_speed = atol(optval);
			set_config_changed();
		}
		// CPU options
		else if (strcmp(optname, "cpu_speed") == 0) {
			changed_prefs.m68k_speed = atol(optval);
			set_config_changed();
		} else if (strcmp(optname, "turbo_emulation") == 0) {
			changed_prefs.turbo_emulation = (strcmp(optval, "true") == 0 || strcmp(optval, "1") == 0);
			set_config_changed();
		}
		// Display options
		else if (strcmp(optname, "gfx_fullscreen") == 0) {
			bool fullscreen = (strcmp(optval, "true") == 0 || strcmp(optval, "1") == 0);
			changed_prefs.gfx_apmode[0].gfx_fullscreen = fullscreen ? GFX_FULLSCREEN : GFX_WINDOW;
			set_config_changed();
		}
		// Sound options
		else if (strcmp(optname, "sound_output") == 0) {
			changed_prefs.produce_sound = atol(optval);
			set_config_changed();
		} else if (strcmp(optname, "sound_stereo") == 0) {
			changed_prefs.sound_stereo = atol(optval);
			set_config_changed();
		} else if (strcmp(optname, "sound_volume") == 0) {
			changed_prefs.sound_volume_master = atol(optval);
			set_config_changed();
		}
		// Chipset options
		else if (strcmp(optname, "ntsc") == 0) {
			changed_prefs.ntscmode = (strcmp(optval, "true") == 0 || strcmp(optval, "1") == 0);
			set_config_changed();
		}
		else {
			// Unknown config option
			status = false;
		}
	}
	SendReply(msg, status);
}

static void HandleSendKey(DBusMessage* msg)
{
	std::cout << "DBUS: Received SEND_KEY" << std::endl;
	DBusErrorWrapper err;
	int keycode = 0;
	int state = 0; // 1=press, 0=release
	bool status = true;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_INT32, &keycode, DBUS_TYPE_INT32, &state, DBUS_TYPE_INVALID);

	if (err.is_set()) {
		status = false;
	} else {
		inputdevice_add_inputcode(keycode, state, nullptr);
	}
	SendReply(msg, status);
}

static void HandleLoadConfig(DBusMessage* msg)
{
	std::cout << "DBUS: Received LOAD_CONFIG" << std::endl;
	DBusErrorWrapper err;
	char *configpath = nullptr;
	bool status = true;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_STRING, &configpath, DBUS_TYPE_INVALID);

	if (err.is_set())
	{
		std::cout << "DBUS Arguments Error: " << err.err.message << std::endl;
		status = false;
	}
	else if (!configpath)
	{
		status = false;
	}
	else
	{
		// Load the config file into changed_prefs
		int result = target_cfgfile_load(&changed_prefs, configpath, CONFIG_TYPE_DEFAULT, 0);
		if (result)
		{
			set_config_changed();
		}
		else
		{
			std::cout << "DBUS: Failed to load config: " << configpath << std::endl;
			status = false;
		}
	}

	SendReply(msg, status);
}

static void HandleLoadState(DBusMessage* msg)
{
	std::cout << "DBUS: Received LOADSTATE" << std::endl;
	DBusErrorWrapper err;
	char *statepath = nullptr;
	bool status = true;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_STRING, &statepath, DBUS_TYPE_INVALID);

	if (err.is_set())
	{
		std::cout << "DBUS Arguments Error: " << err.err.message << std::endl;
		status = false;
	}
	else if (!statepath)
	{
		status = false;
	}
	else
	{
		// Set up the state restore
		savestate_state = STATE_DORESTORE;
		_tcscpy(savestate_fname, statepath);
		std::cout << "DBUS: Will restore state from: " << statepath << std::endl;
	}

	SendReply(msg, status);
}

static void HandleReadMem(DBusMessage* msg)
{
	std::cout << "DBUS: Received READ_MEM" << std::endl;
	DBusErrorWrapper err;
	uint32_t addr = 0;
	int size = 1; // bytes to read, maybe support 1, 2, 4? Or byte array?
	// For simplicity, let's just return a byte or word. Or maybe size is 1,2,4.
	// DBus args: UINT32 addr, INT32 width (1,2,4)
	int width = 0;
	bool status = true;
	std::vector<std::string> responses;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_UINT32, &addr, DBUS_TYPE_INT32, &width, DBUS_TYPE_INVALID);

	if (err.is_set()) {
		status = false;
	} else {
		if (width == 1) {
			responses.push_back(std::to_string(get_byte(addr)));
		} else if (width == 2) {
			responses.push_back(std::to_string(get_word(addr)));
		} else if (width == 4) {
			responses.push_back(std::to_string(get_long(addr)));
		} else {
			status = false;
		}
	}
	SendReply(msg, status, responses);
}

static void HandleWriteMem(DBusMessage* msg)
{
	std::cout << "DBUS: Received WRITE_MEM" << std::endl;
	DBusErrorWrapper err;
	uint32_t addr = 0;
	uint32_t val = 0;
	int width = 0;
	bool status = true;

	dbus_message_get_args(msg, &err.err, DBUS_TYPE_UINT32, &addr, DBUS_TYPE_INT32, &width, DBUS_TYPE_UINT32, &val, DBUS_TYPE_INVALID);

	if (err.is_set()) {
		status = false;
	} else {
		if (width == 1) {
			put_byte(addr, val);
		} else if (width == 2) {
			put_word(addr, val);
		} else if (width == 4) {
			put_long(addr, val);
		} else {
			status = false;
		}
	}
	SendReply(msg, status);
}


void DBusHandle()
{
	if (!dbusconn) return;
	
	if (request_handlers.empty()) {
		request_handlers[CMD_QUIT] = HandleQuit;
		request_handlers[CMD_PAUSE] = HandlePause;
		request_handlers[CMD_RESUME] = HandleResume;
		request_handlers[CMD_RESET] = HandleReset;
		request_handlers[CMD_SCREENSHOT] = HandleScreenshot;
		request_handlers[CMD_SAVESTATE] = HandleSavestate;
		request_handlers[CMD_DISKSWAP] = HandleDiskSwap;
		request_handlers[CMD_QUERY_DISKSWAP] = HandleQueryDiskSwap;
		request_handlers[CMD_INSERT_FLOPPY] = HandleInsertFloppy;
		request_handlers[CMD_INSERT_CD] = HandleInsertCD;
		
		request_handlers[CMD_GET_STATUS] = HandleGetStatus;
		request_handlers[CMD_GET_CONFIG] = HandleGetConfig;
		request_handlers[CMD_SET_CONFIG] = HandleSetConfig;
		request_handlers[CMD_LOAD_CONFIG] = HandleLoadConfig;
		request_handlers[CMD_LOAD_STATE] = HandleLoadState;
		request_handlers[CMD_SEND_KEY] = HandleSendKey;
		request_handlers[CMD_READ_MEM] = HandleReadMem;
		request_handlers[CMD_WRITE_MEM] = HandleWriteMem;
	}

	dbus_connection_read_write(dbusconn, 0);

	DBusMessageWrapper msg(nullptr);
	while ((msg.msg = dbus_connection_pop_message(dbusconn)))
	{
		if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_METHOD_CALL &&
			dbus_message_has_interface(msg, INTERFACE_NAME))
		{
			const char* member = dbus_message_get_member(msg);
			if (member && request_handlers.count(member))
			{
				request_handlers[member](msg);
			}
		}
	}
}

void DBusCleanUp()
{
	if(dbusconn)
	{
		dbus_connection_unref(dbusconn);
		dbusconn = nullptr;
	}
}

void DBusSetup()
{
	DBusErrorWrapper err;
	int result;

	dbusconn = dbus_bus_get(DBUS_BUS_SESSION, &err.err);
	if(err.is_set())
	{
		std::cout << "DBUS Connection Error: " << err.err.message << std::endl;
		DBusCleanUp();
		return;
	}

	result = dbus_bus_request_name(dbusconn, INTERFACE_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING, &err.err);

	if(err.is_set())
	{
		std::cout << "DBUS Name Error: " << err.err.message << std::endl;
		DBusCleanUp();
		return;
	}

	if(result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER )
	{
		std::cout << "Not primary name owner\n";
		DBusCleanUp();
	}
	atexit(DBusCleanUp);
}

#endif
