#pragma once

// Emulator-side hooks of the action queue (amiberry_actions.cpp), for the
// loops that drain it. Not part of the front-end API in
// settings/amiberry_actions.h. Main thread only.

// The number of reset, quit and state-load requests executed actions have
// made. Each marks quit_program, which only the emulation loop carries out;
// a loop that drains actions but cannot (the settings GUI) compares the
// count around a drain to know it must return. A count, not quit_program,
// because a request can leave quit_program unchanged (a second soft reset
// while one is pending).
unsigned long amiberry_actions_emulation_requests();

// The settings GUI released its pause. A PAUSE action executed while the
// GUI held it is applied now, so emulation stays paused as the action
// reported.
void amiberry_actions_gui_closed();
