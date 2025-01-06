/*
* UAE - The Un*x Amiga Emulator
*
* Amiberry uaenet emulation
*
* Copyright 2007 Toni Wilen
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "ethernet.h"
static int ethernet_paused;

void ethernet_pause(int pause)
{
	ethernet_paused = pause;
}

void ethernet_reset()
{
	ethernet_paused = 0;
}