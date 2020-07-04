/*
*  uaeexe.c - UAE remote cli
*
*  (c) 1997 by Samuel Devulder
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"
#include "uaeexe.h"

static struct uae_xcmd *first = NULL;
static struct uae_xcmd *last  = NULL;
static TCHAR running = 0;
static uae_u32 REGPARAM3 uaeexe_server (TrapContext *context) REGPARAM;

/*
* Install the server
*/
void uaeexe_install (void)
{
	uaecptr loop;

	if (!uae_boot_rom_type && !currprefs.uaeboard)
		return;
	loop = here ();
	org (UAEEXE_ORG);
	calltrap (deftrapres (uaeexe_server, 0, _T("uaeexe_server")));
	dw (RTS);
	org (loop);
}

/*
* Send command to the remote cli.
*
* To use this, just call uaeexe("command") and the command will be
* executed by the remote cli (provided you've started it in the
* s:user-startup for example). Be sure to add "run" if you want
* to launch the command asynchronously. Please note also that the
* remote cli works better if you've got the fifo-handler installed.
*/
int uaeexe (const TCHAR *cmd)
{
	struct uae_xcmd *nw;

	if (!running)
		goto NORUN;

	nw = xmalloc (struct uae_xcmd, 1);
	if (!nw)
		goto NOMEM;
	nw->cmd = xmalloc (TCHAR, _tcslen (cmd) + 1);
	if (!nw->cmd) {
		xfree (nw);
		goto NOMEM;
	}

	_tcscpy (nw->cmd, cmd);
	nw->prev = last;
	nw->next = NULL;

	if (!first)
		first = nw;
	if (last) {
		last->next = nw;
		last = nw;
	} else
		last = nw;

	return UAEEXE_OK;
NOMEM:
	return UAEEXE_NOMEM;
NORUN:
	write_log (_T("Remote cli is not running.\n"));
	return UAEEXE_NOTRUNNING;
}

/*
* returns next command to be executed
*/
static TCHAR *get_cmd (void)
{
	struct uae_xcmd *cmd;
	TCHAR *s;

	if (!first)
		return NULL;
	s = first->cmd;
	cmd = first;
	first = first->next;
	if (!first)
		last = NULL;
	free (cmd);
	return s;
}

/*
* helper function
*/
#define ARG(x) (trap_get_long(ctx, trap_get_areg(ctx, 7) + 4 * (x + 1)))
static uae_u32 REGPARAM2 uaeexe_server (TrapContext *ctx)
{
	int len;
	TCHAR *cmd;
	char *dst, *s;

	if (ARG (0) && !running) {
		running = 1;
		write_log (_T("Remote CLI started.\n"));
	}

	cmd = get_cmd ();
	if (!cmd)
		return 0;
	if (!ARG (0)) {
		running = 0;
		return 0;
	}

	dst = (char*)get_real_address (ARG (0));
	len = ARG (1);
	s = ua (cmd);
	strncpy (dst, s, len);
	write_log (_T("Sending '%s' to remote cli\n"), cmd);
	xfree (s);
	xfree (cmd);
	return ARG (0);
}
