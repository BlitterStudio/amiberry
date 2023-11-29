//
// Created by Dimitris Panokostas on 29/11/2023.
//

#pragma once

#ifdef USE_DBUS

#include "sysdeps.h"
#include "options.h"
#include "target.h"
#include "uae.h"
#include "savestate.h"
#include <dbus/dbus.h>

extern DBusConnection *dbusconn;
extern void DBusHandle();
extern void DBusSetup();

#endif


