//
// Created by Dimitris Panokostas on 29/11/2023.
//

#include "amiberry_dbus.h"
#include <vector>
#include <string>
#include <iostream>

#ifdef USE_DBUS

DBusConnection *dbusconn;

void DBusHandle()
{
	if(dbusconn)
	{
		DBusError err;
		DBusMessage *msg = nullptr;
		DBusMessage *reply = nullptr;
		DBusMessageIter args;
		dbus_uint32_t serial = 0;
		bool status = true;
		bool respond = false;

		dbus_error_init(&err);
		dbus_connection_read_write(dbusconn, 0);

		std::vector<std::string> responses; // array of string to return

		while((msg = dbus_connection_pop_message(dbusconn)))
		{
			if(dbus_message_is_method_call(msg, AMIBERRY_DBUS_INTERFACE, "QUIT"))
			{
				std::cout << "DBUS: Received QUIT" << std::endl;
				respond = false;
				uae_quit();
			}
			if(dbus_message_is_method_call(msg, AMIBERRY_DBUS_INTERFACE, "PAUSE"))
			{
				std::cout << "DBUS: Received PAUSE" << std::endl;
				setpaused(3);
				activationtoggle(0, true);
				respond = true;
			}
			if(dbus_message_is_method_call(msg, AMIBERRY_DBUS_INTERFACE, "RESUME"))
			{
				std::cout << "DBUS: Received RESUME" << std::endl;
				respond = false;
				resumepaused(3);
				activationtoggle(0, false);
				dbus_message_unref(msg);
				return;
			}
			if(dbus_message_is_method_call(msg, AMIBERRY_DBUS_INTERFACE, "RESET"))
			{
				std::cout << "DBUS: Received RESET" << std::endl;
				bool error = false;
				char *type = nullptr;
				bool keyboard = false;
				bool hard = false;

				dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &type, DBUS_TYPE_INVALID);
				respond = false;
				if(dbus_error_is_set(&err))
				{
					// optional arg
					dbus_error_free(&err);
				}
				if(type)
				{
					if(strcmp(type,"HARD") == 0 || strcmp(type,"KEYBOARD") == 0)
					{
						hard = true;
					}
					else
					{
						// invalid string
						status = false;
						error = true;
					}
				}
				else
				{
					keyboard = true;
				}
				if(!error)
				{
					uae_reset(hard,keyboard);
				}
			}
			if(dbus_message_is_method_call(msg, AMIBERRY_DBUS_INTERFACE, "SCREENSHOT"))
			{
				std::cout << "DBUS: Received SCREENSHOT" << std::endl;
				bool error = false;
				char *filename = nullptr;
				dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &filename, DBUS_TYPE_INVALID);
				respond = true;
				if(dbus_error_is_set(&err))
				{
					std::cout << "DBUS Arguments Error: " << err.message << std::endl;
					dbus_error_free(&err);
					status = false;
					error = true;
				}
				if(!error)
				{
					if(filename)
					{
						create_screenshot();
						save_thumb(filename);
					}
					else
					{
						status = false;
					}
				}
			}
			if(dbus_message_is_method_call(msg, AMIBERRY_DBUS_INTERFACE, "SAVESTATE"))
			{
				std::cout << "DBUS: Received SAVESTATE" << std::endl;
				bool error = false;
				char *statefilename = nullptr;
				char *configfilename = nullptr;
				dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &statefilename, DBUS_TYPE_STRING, &configfilename, DBUS_TYPE_INVALID);
				respond = true;
				if(dbus_error_is_set(&err))
				{
					std::cout << "DBUS Arguments Error: " << err.message << std::endl;
					dbus_error_free(&err);
					status = false;
					error = true;
				}
				if(!error)
				{
					std::cout << "SF: " << statefilename << "\nCF: " << configfilename << std::endl;
					if(statefilename)
					{
						savestate_initsave(statefilename, 1, true, true);
						save_state(statefilename, "...");
					}
					if(configfilename)
					{
						strncpy(changed_prefs.description, "autosave", sizeof(changed_prefs.description));
						cfgfile_save(&changed_prefs, configfilename, 0);
					}
					if(!statefilename || !configfilename)
					{
						status = false;
					}
				}
			}
			if(dbus_message_is_method_call(msg, AMIBERRY_DBUS_INTERFACE, "DISKSWAP"))
			{
				std::cout << "DBUS: Received DISKSWAP" << std::endl;
				bool error = false;
				char *disknumstr = nullptr;
				char *drivenumstr = nullptr;
				dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &disknumstr,DBUS_TYPE_STRING, &drivenumstr, DBUS_TYPE_INVALID);
				respond = true;
				if(dbus_error_is_set(&err))
				{
					std::cout << "DBUS Arguments Error: " << err.message << std::endl;
					dbus_error_free(&err);
					status = false;
					error = true;
				}
				if(!disknumstr || !drivenumstr)
				{
					error = true;
					status = false;
				}
				if(!error)
				{
					int disknum = -1;
					int drivenum = -1;
					disknum = atol(disknumstr);
					drivenum = atol(drivenumstr);

					if( (disknum >= 0) && (disknum < MAX_SPARE_DRIVES) && (drivenum >= 0) && (drivenum <= 3) )
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
			}
			if(dbus_message_is_method_call(msg, AMIBERRY_DBUS_INTERFACE, "QUERYDISKSWAP"))
			{
				std::cout << "DBUS: Received QUERYDISKSWAP" << std::endl;
				bool error = false;
				char *drivenumstr = nullptr;
				dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &drivenumstr, DBUS_TYPE_INVALID);
				respond = true;

				if(dbus_error_is_set(&err))
				{
					std::cout << "DBUS Arguments Error: " << err.message << std::endl;
					dbus_error_free(&err);
					status = false;
					error = true;
				}
				if(!drivenumstr)
				{
					error = true;
					status = false;
				}
				if(!error)
				{
					int drivenum = -1;
					int disknum = -1;
					drivenum = atol(drivenumstr);
					if((drivenum >= 0) && (drivenum <= 3))
					{
						for(int i = 0; i < MAX_SPARE_DRIVES; i++)
						{
							if(!_tcscmp(currprefs.floppyslots[drivenum].df, currprefs.dfxlist[i]))
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
			}
			
			if(dbus_message_is_method_call(msg, AMIBERRY_DBUS_INTERFACE, "INSERTFLOPPY"))
			{
				std::cout << "DBUS: Received INSERTFLOPPY" << std::endl;
				bool error = false;
				char *diskpathstr = nullptr;
				char *drivenumstr = nullptr;
				dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &diskpathstr,DBUS_TYPE_STRING, &drivenumstr, DBUS_TYPE_INVALID);
				respond = true;
				if(dbus_error_is_set(&err))
				{
					std::cout << "DBUS Arguments Error: " << err.message << std::endl;
					dbus_error_free(&err);
					status = false;
					error = true;
				}
				if(!diskpathstr || !drivenumstr)
				{
					error = true;
					status = false;
				}
				if(!error)
				{
					int drivenum = -1;
					drivenum = atol(drivenumstr);

					if( (drivenum >= 0) && (drivenum <= 3) )
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
			}
			
			if(dbus_message_is_method_call(msg, AMIBERRY_DBUS_INTERFACE, "INSERTCD"))
			{
				std::cout << "DBUS: Received INSERTCD" << std::endl;
				bool error = false;
				char *diskpathstr = nullptr;
				dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &diskpathstr, DBUS_TYPE_INVALID);
				respond = true;
				if(dbus_error_is_set(&err))
				{
					std::cout << "DBUS Arguments Error: " << err.message << std::endl;
					dbus_error_free(&err);
					status = false;
					error = true;
				}
				if(!diskpathstr)
				{
					error = true;
					status = false;
				}
				if(!error)
				{
					_tcsncpy(changed_prefs.cdslots[0].name, diskpathstr , MAX_DPATH);
					changed_prefs.cdslots[0].name[MAX_DPATH - 1] = 0;
					changed_prefs.cdslots[0].inuse = true;
					
					std::cout << "CD Type: " << changed_prefs.cdslots[0].type << "\n";
					set_config_changed();
				}
			}
			
			if(respond)
			{
				if((reply = dbus_message_new_method_return(msg)))
				{
					dbus_message_iter_init_append(reply, &args);
					dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &status);

					if(!responses.empty())
					{
						for(const auto& r : responses)
						{
							const char *cstr = r.c_str();
							dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &cstr);
						}
						responses.clear();
					}
					dbus_connection_send(dbusconn, reply, &serial);
					dbus_connection_flush(dbusconn);
					dbus_message_unref(reply);
				}
			}
			dbus_message_unref(msg);
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
	DBusError err;
	int result;

	dbus_error_init(&err);

	dbusconn = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if(dbus_error_is_set(&err))
	{
		std::cout << "DBUS Connection Error: " << err.message << std::endl;
		dbus_error_free(&err);
		DBusCleanUp();
		return;
	}

	result = dbus_bus_request_name(dbusconn, AMIBERRY_DBUS_INTERFACE, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

	if(dbus_error_is_set(&err))
	{
		std::cout << "DBUS Name Error: " << err.message << std::endl;
		dbus_error_free(&err);
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
