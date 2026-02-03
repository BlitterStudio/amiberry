#pragma once

#ifdef USE_DBUS

#include "sysdeps.h"
#include "options.h"
#include "target.h"
#include "uae.h"
#include "savestate.h"
#include <dbus/dbus.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace Amiberry::DBus {
    constexpr char const* INTERFACE_NAME = "com.blitterstudio.amiberry";
    
    // Command names
    constexpr char const* CMD_QUIT = "QUIT";
    constexpr char const* CMD_PAUSE = "PAUSE";
    constexpr char const* CMD_RESUME = "RESUME";
    constexpr char const* CMD_RESET = "RESET";
    constexpr char const* CMD_SCREENSHOT = "SCREENSHOT";
    constexpr char const* CMD_SAVESTATE = "SAVESTATE";
    constexpr char const* CMD_DISKSWAP = "DISKSWAP";
    constexpr char const* CMD_QUERY_DISKSWAP = "QUERYDISKSWAP";
    constexpr char const* CMD_INSERT_FLOPPY = "INSERTFLOPPY";
    constexpr char const* CMD_INSERT_CD = "INSERTCD";
    
    // New Commands
    constexpr char const* CMD_GET_STATUS = "GET_STATUS";
    constexpr char const* CMD_GET_CONFIG = "GET_CONFIG";
    constexpr char const* CMD_SET_CONFIG = "SET_CONFIG";
    constexpr char const* CMD_LOAD_CONFIG = "LOAD_CONFIG";
    constexpr char const* CMD_SEND_KEY = "SEND_KEY";
    constexpr char const* CMD_READ_MEM = "READ_MEM";
    constexpr char const* CMD_WRITE_MEM = "WRITE_MEM";

    // RAII Wrappers
    struct DBusErrorWrapper {
        DBusError err;
        DBusErrorWrapper() { dbus_error_init(&err); }
        ~DBusErrorWrapper() { dbus_error_free(&err); }
        bool is_set() const { return dbus_error_is_set(&err); }
    };

    struct DBusMessageWrapper {
        DBusMessage* msg = nullptr;
        explicit DBusMessageWrapper(DBusMessage* m) : msg(m) {}
        ~DBusMessageWrapper() { if(msg) dbus_message_unref(msg); }
        operator DBusMessage*() const { return msg; }
    };
}

extern DBusConnection *dbusconn;
extern void DBusHandle();
extern void DBusSetup();

#endif


