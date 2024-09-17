
#include <stdio.h>

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef CATWEASEL

#include "options.h"
#include "memory.h"
#include "ioport.h"
#include "catweasel.h"
#include "uae.h"
#include "zfile.h"

#define DRIVER
#include <catweasl_usr.h>

struct catweasel_contr cwc;

static int cwhsync, cwmk3buttonsync;
static int cwmk3port, cwmk3port1, cwmk3port2;
static int handshake;
static int mouse_x[2], mouse_y[2], mouse_px[2], mouse_py[2];

static HANDLE handle = INVALID_HANDLE_VALUE;

int catweasel_isjoystick (void)
{
	uae_u8 b = cwc.can_joy;
	if (!cwc.direct_access)
		return 0;
	if (b) {
		if (cwc.type == CATWEASEL_TYPE_MK3 && cwc.sid[0])
			b |= 0x80;
		if (cwc.type >= CATWEASEL_TYPE_MK4)
			b |= 0x80;
	}
	return b;
}
int catweasel_ismouse (void)
{
	if (!cwc.direct_access)
		return 0;
	return cwc.can_mouse;
}

static int hsync_requested;
static void hsync_request (void)
{
	hsync_requested = 10;
};

static void sid_write (uae_u8 reg, uae_u8 val, int sidnum)
{
	if (sidnum >= cwc.can_sid)
		return;
	catweasel_do_bput(0xd8, val);
	catweasel_do_bput(0xdc, reg | (sidnum << 7));
	catweasel_do_bget(0xd8); // dummy read
	catweasel_do_bget(0xd8); // dummy read
}

static uae_u8 sid_read (uae_u8 reg, int sidnum)
{
	if (sidnum >= cwc.can_sid)
		return 0;
	catweasel_do_bput(0xdc, 0x20 | reg | (sidnum << 7));
	catweasel_do_bget(0xd8); // dummy read
	catweasel_do_bget(0xd8); // dummy read
	return catweasel_do_bget(0xd8);
}

static uae_u8 get_buttons (void)
{
	uae_u8 b, b2;

	b = 0;
	if (cwc.type < CATWEASEL_TYPE_MK3 || !cwc.direct_access)
		return b;
	hsync_request();
	b2 = catweasel_do_bget(0xc8) & (0x80 | 0x40);
	if (!(b2 & 0x80))
		b |= 0x80;
	if (!(b2 & 0x40))
		b |= 0x08;
	if (cwc.type >= CATWEASEL_TYPE_MK4) {
		b &= ~0x80;
		catweasel_do_bput(3, 0x81);
		if (!(catweasel_do_bget(0x07) & 0x10))
			b |= 0x80;
		b2 = catweasel_do_bget(0xd0) ^ 15;
		catweasel_do_bput(3, 0x41);
		if (cwc.sid[0]) {
			b2 &= ~(1 | 2);
			if (sid_read(0x19, 0) > 0x7f)
				b2 |= 2;
			if (sid_read(0x1a, 0) > 0x7f)
				b2 |= 1;
		}
		if (cwc.sid[1]) {
			b2 &= ~(4 | 8);
			if (sid_read(0x19, 1) > 0x7f)
				b2 |= 8;
			if (sid_read(0x1a, 1) > 0x7f)
				b2 |= 4;
		}
	} else {
		b2 = cwmk3port1 | (cwmk3port2 << 2);
	}
	b |= (b2 & (8 | 4)) << 3;
	b |= (b2 & (1 | 2)) << 1;
	return b;
}

int catweasel_read_mouse (int port, int *dx, int *dy, int *buttons)
{
	if (!cwc.can_mouse || !cwc.direct_access)
		return 0;
	hsync_request();
	*dx = mouse_x[port];
	mouse_x[port] = 0;
	*dy = mouse_y[port];
	mouse_y[port] = 0;
	*buttons = (get_buttons() >> (port * 4)) & 15;
	return 1;
}

static void sid_reset (void)
{
	int i;
	for (i = 0; i < 0x19; i++) {
		sid_write(i, 0, 0);
		sid_write(i, 0, 1);
	}
}

static void catweasel_detect_sid (void)
{
	int i, j;
	uae_u8 b1, b2;

	cwc.sid[0] = cwc.sid[1] = 0;
	if (!cwc.can_sid || !cwc.direct_access)
		return;
	sid_reset();
	if (cwc.type >= CATWEASEL_TYPE_MK4) {
		catweasel_do_bput(3, 0x81);
		b1 = catweasel_do_bget(0xd0);
		for (i = 0; i < 100; i++) {
			sid_read(0x19, 0); // delay
			b2 = catweasel_do_bget(0xd0);
			if ((b1 & 3) != (b2 & 3))
				cwc.sid[0] = 6581;
			if ((b1 & 12) != (b2 & 12))
				cwc.sid[1] = 6581;
		}
	}
	catweasel_do_bput(3, 0x41);
	for (i = 0; i < 2 ;i++) {
		sid_reset();
		sid_write(0x0f, 0xff, i);
		sid_write(0x12, 0x10, i);
		for(j = 0; j != 1000; j++) {
			sid_write(0, 0, i);
			if((sid_read(0x1b, i) & 0x80) != 0) {
				cwc.sid[i] = 6581;
				break;
			}
		}
		sid_reset();
		sid_write(0x0f, 0xff, i);
		sid_write(0x12, 0x30, i);
		for(j = 0; j != 1000; j++) {
			sid_write(0, 0, i);
			if((sid_read(0x1b, i) & 0x80) != 0) {
				cwc.sid[i] = 8580;
				break;
			}
		}
	}
	sid_reset();
}

void catweasel_hsync (void)
{
	int i;

	if (cwc.type < CATWEASEL_TYPE_MK3 || !cwc.direct_access)
		return;
	cwhsync--;
	if (cwhsync > 0)
		return;
	cwhsync = 10;
	if (handshake) {
		/* keyboard handshake */
		catweasel_do_bput (0xd0, 0);
		handshake = 0;
	}
	if (hsync_requested < 0)
		return;
	hsync_requested--;
	if (cwc.type == CATWEASEL_TYPE_MK3 && cwc.sid[0]) {
		uae_u8 b;
		cwmk3buttonsync--;
		if (cwmk3buttonsync <= 0) {
			cwmk3buttonsync = 30;
			b = 0;
			if (sid_read (0x19, 0) > 0x7f)
				b |= 2;
			if (sid_read (0x1a, 0) > 0x7f)
				b |= 1;
			if (cwmk3port == 0) {
				cwmk3port1 = b;
				catweasel_do_bput (0xd4, 0); // select port2
				cwmk3port = 1;
			} else {
				cwmk3port2 = b;
				catweasel_do_bget (0xd4); // select port1
				cwmk3port = 0;
			}
		}
	}
	if (!cwc.can_mouse)
		return;
	/* read MK4 mouse counters */
	catweasel_do_bput (3, 0x81);
	for (i = 0; i < 2; i++) {
		int x, y, dx, dy;
		x = (uae_s8)catweasel_do_bget (0xc4 + i * 8);
		y = (uae_s8)catweasel_do_bget (0xc0 + i * 8);
		dx = mouse_px[i] - x;
		if (dx > 127)
			dx = 255 - dx;
		if (dx < -128)
			dx = 255 + dx;
		dy = mouse_py[i] - y;
		if (dy > 127)
			dy = 255 - dy;
		if (dy < -128)
			dy = 255 + dy;
		mouse_x[i] -= dx;
		mouse_y[i] -= dy;
		mouse_px[i] = x;
		mouse_py[i] = y;
	}
	catweasel_do_bput (3, 0x41);
}

int catweasel_read_joystick (uae_u8 *dir, uae_u8 *buttons)
{
	if (!cwc.can_joy || !cwc.direct_access)
		return 0;
	hsync_request ();
	*dir = catweasel_do_bget (0xc0);
	*buttons = get_buttons ();
	return 1;
}

int catweasel_read_keyboard (uae_u8 *keycode)
{
	uae_u8 v;

	if (!cwc.can_kb || !cwc.direct_access)
		return 0;
	if (!currprefs.catweasel)
		return 0;
	v = catweasel_do_bget (0xd4);
	if (!(v & 0x80))
		return 0;
	if (handshake)
		return 0;
	*keycode = catweasel_do_bget (0xd0);
	catweasel_do_bput (0xd0, 0);
	handshake = 1;
	return 1;
}

uae_u32	catweasel_do_bget (uaecptr addr)
{
	DWORD did_read = 0;
	uae_u8 buf1[1], buf2[1];

	if (addr >= 0x100)
		return 0;
	buf1[0] = (uae_u8)addr;
	if (handle != INVALID_HANDLE_VALUE) {
		if (!DeviceIoControl (handle, CW_PEEKREG_FULL, buf1, 1, buf2, 1, &did_read, 0))
			write_log (_T("catweasel_do_bget %02x fail err=%d\n"), buf1[0], GetLastError ());
	} else {
		buf2[0] = ioport_read (cwc.iobase + addr);
	}
	//write_log (_T("G %02X %02X %d\n"), buf1[0], buf2[0], did_read);
	return buf2[0];
}

void catweasel_do_bput (uaecptr	addr, uae_u32 b)
{
	uae_u8 buf[2];
	DWORD did_read = 0;

	if (addr >= 0x100)
		return;
	buf[0] = (uae_u8)addr;
	buf[1] = b;
	if (handle != INVALID_HANDLE_VALUE) {
		if (!DeviceIoControl (handle, CW_POKEREG_FULL, buf, 2, 0, 0, &did_read, 0))
			write_log (_T("catweasel_do_bput %02x=%02x fail err=%d\n"), buf[0], buf[1], GetLastError ());
	} else {
		ioport_write (cwc.iobase + addr, b);
	}
	//write_log (_T("P %02X %02X %d\n"), (uae_u8)addr, (uae_u8)b, did_read);
}

#include "core.cw4.cpp"

static int cw_config_done (void)
{
	return ioport_read (cwc.iobase + 7) & 4;
}
static int cw_fpga_ready (void)
{
	return ioport_read (cwc.iobase + 7) & 8;
}
static void cw_resetFPGA (void)
{
	ioport_write (cwc.iobase + 2, 227);
	ioport_write (cwc.iobase + 3, 0);
	sleep_millis (10);
	ioport_write (cwc.iobase + 3, 65);
}

static int catweasel3_configure (void)
{
	ioport_write (cwc.iobase, 241);
	ioport_write (cwc.iobase + 1, 0);
	ioport_write (cwc.iobase + 2, 0);
	ioport_write (cwc.iobase + 4, 0);
	ioport_write (cwc.iobase + 5, 0);
	ioport_write (cwc.iobase + 0x29, 0);
	ioport_write (cwc.iobase + 0x2b, 0);
	return 1;
}

static int catweasel4_configure (void)
{
	struct zfile *f;
	time_t t;

	ioport_write (cwc.iobase, 241);
	ioport_write (cwc.iobase + 1, 0);
	ioport_write (cwc.iobase + 2, 227);
	ioport_write (cwc.iobase + 3, 65);
	ioport_write (cwc.iobase + 4, 0);
	ioport_write (cwc.iobase + 5, 0);
	ioport_write (cwc.iobase + 0x29, 0);
	ioport_write (cwc.iobase + 0x2b, 0);
	sleep_millis(10);

	if (cw_config_done()) {
		write_log (_T("CW: FPGA already configured, skipping core upload\n"));
		return 1;
	}
	cw_resetFPGA();
	sleep_millis(10);
	if (cw_config_done()) {
		write_log (_T("CW: FPGA failed to reset!\n"));
		return 0;
	}
	f = zfile_fopen(_T("core.cw4"), _T("rb"), ZFD_NORMAL);
	if (!f) {
		f = zfile_fopen_data (_T("core.cw4.gz"), core_len, core);
		f = zfile_gunzip (f);
	}
	write_log (_T("CW: starting core upload, this will take few seconds\n"));
	t = time(NULL) + 10; // give up if upload takes more than 10s
	for (;;) {
		uae_u8 b;
		if (zfile_fread (&b, 1, 1, f) != 1)
			break;
		ioport_write (cwc.iobase + 3, (b & 1) ? 67 : 65);
		while (!cw_fpga_ready()) {
			if (time(NULL) >= t) {
				write_log (_T("CW: FPGA core upload got stuck!?\n"));
				cw_resetFPGA();
				return 0;
			}
		}
		ioport_write (cwc.iobase + 192, b);
	}
	if (!cw_config_done()) {
		write_log (_T("CW: FPGA didn't accept the core!\n"));
		cw_resetFPGA();
		return 0;
	}
	sleep_millis(10);
	write_log (_T("CW: core uploaded successfully\n"));
	return 1;
}

#include <setupapi.h>
#include <cfgmgr32.h>

#define PCI_CW_MK3 _T("PCI\\VEN_E159&DEV_0001&SUBSYS_00021212")
#define PCI_CW_MK4 _T("PCI\\VEN_E159&DEV_0001&SUBSYS_00035213")
#define PCI_CW_MK4_BUG _T("PCI\\VEN_E159&DEV_0001&SUBSYS_00025213")

int force_direct_catweasel;
static int direct_detect(void)
{
	HDEVINFO devs;
	SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
	SP_DEVINFO_DATA devInfo;
	int devIndex;
	int cw = 0;

	devs = SetupDiGetClassDevsEx(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT, NULL, NULL, NULL);
	if (devs == INVALID_HANDLE_VALUE)
		return 0;
	devInfoListDetail.cbSize = sizeof(devInfoListDetail);
	if(SetupDiGetDeviceInfoListDetail(devs,&devInfoListDetail)) {
		devInfo.cbSize = sizeof(devInfo);
		for(devIndex=0;SetupDiEnumDeviceInfo(devs,devIndex,&devInfo);devIndex++) {
			TCHAR devID[MAX_DEVICE_ID_LEN];
			if(CM_Get_Device_ID_Ex(devInfo.DevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS)
				devID[0] = TEXT('\0');
			if (!_tcsncmp (devID, PCI_CW_MK3, _tcslen (PCI_CW_MK3))) {
				if (cw > 3)
					break;
				cw = 3;
			}
			if (!_tcsncmp (devID, PCI_CW_MK4, _tcslen (PCI_CW_MK4)) ||
				!_tcsncmp (devID, PCI_CW_MK4_BUG, _tcslen (PCI_CW_MK4_BUG)))
				cw = 4;
			if (cw) {
				SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
				ULONG status = 0;
				ULONG problem = 0;
				LOG_CONF config = 0;
				BOOL haveConfig = FALSE;
				ULONG dataSize;
				PBYTE resDesData;
				RES_DES prevResDes, resDes;
				RESOURCEID resId = ResType_IO;

				devInfoListDetail.cbSize = sizeof(devInfoListDetail);
				if((!SetupDiGetDeviceInfoListDetail(devs,&devInfoListDetail)) ||
					(CM_Get_DevNode_Status_Ex(&status,&problem,devInfo.DevInst,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS))
					break;
				if(!(status & DN_HAS_PROBLEM)) {
					if (CM_Get_First_Log_Conf_Ex(&config,
						devInfo.DevInst,
						ALLOC_LOG_CONF,
						devInfoListDetail.RemoteMachineHandle) == CR_SUCCESS) {
							haveConfig = TRUE;
					}
				}
				if(!haveConfig) {
					if (CM_Get_First_Log_Conf_Ex(&config,
						devInfo.DevInst,
						FORCED_LOG_CONF,
						devInfoListDetail.RemoteMachineHandle) == CR_SUCCESS) {
							haveConfig = TRUE;
					}
				}
				if(!haveConfig) {
					if(!(status & DN_HAS_PROBLEM) || (problem != CM_PROB_HARDWARE_DISABLED)) {
						if (CM_Get_First_Log_Conf_Ex(&config,
							devInfo.DevInst,
							BOOT_LOG_CONF,
							devInfoListDetail.RemoteMachineHandle) == CR_SUCCESS) {
								haveConfig = TRUE;
						}
					}
				}
				if(!haveConfig)
					break;
				prevResDes = (RES_DES)config;
				resDes = 0;
				while(CM_Get_Next_Res_Des_Ex(&resDes,prevResDes,ResType_IO,&resId,0,NULL)==CR_SUCCESS) {
					if(prevResDes != config)
						CM_Free_Res_Des_Handle(prevResDes);
					prevResDes = resDes;
					if(CM_Get_Res_Des_Data_Size_Ex(&dataSize,resDes,0,NULL)!=CR_SUCCESS)
						continue;
					resDesData = (PBYTE)malloc (dataSize);
					if(!resDesData)
						continue;
					if(CM_Get_Res_Des_Data_Ex(resDes,resDesData,dataSize,0,NULL)!=CR_SUCCESS) {
						free (resDesData);
						continue;
					}
					if (resId == ResType_IO) {
						PIO_RESOURCE pIoData = (PIO_RESOURCE)resDesData;
						if(pIoData->IO_Header.IOD_Alloc_End-pIoData->IO_Header.IOD_Alloc_Base+1) {
							write_log (_T("CW: PCI SCAN: CWMK%d @%I64X - %I64X\n"), cw,
								pIoData->IO_Header.IOD_Alloc_Base,pIoData->IO_Header.IOD_Alloc_End);
							cwc.iobase = (int)pIoData->IO_Header.IOD_Alloc_Base;
							cwc.direct_type = cw;
						}
					}
					free (resDesData);
				}
				if(prevResDes != config)
					CM_Free_Res_Des_Handle(prevResDes);
				CM_Free_Log_Conf_Handle(config);
			}
		}
	}
	SetupDiDestroyDeviceInfoList(devs);
	if (cw) {
		if (!ioport_init ())
			cw = 0;
	}
	return cw;
}

static int detected;

int catweasel_init(void)
{
	TCHAR name[32], tmp[1000], *s;
	int i;
	DWORD len;
	uae_u8 buffer[10000];
	uae_u32 model, base;
	int detect = 0;

	if (cwc.type)
		return 1;

	if (force_direct_catweasel >= 100) {

		cwc.iobase = force_direct_catweasel & 0xffff;
		if (force_direct_catweasel > 0xffff) {
			cwc.direct_type = force_direct_catweasel >> 16;
		} else {
			cwc.direct_type = force_direct_catweasel >= 0x400 ? 3 : 1;
		}
		cwc.direct_access = 1;

	} else {

		for (i = 0; i < 4; i++) {
			int j = i;
			if (currprefs.catweasel > 0)
				j = currprefs.catweasel + i;
			if (currprefs.catweasel < 0)
				j = -currprefs.catweasel + 1 + i;
			_stprintf (name, _T("\\\\.\\CAT%d_F0"), j);
			handle = CreateFile (name, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, 0,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (handle != INVALID_HANDLE_VALUE || currprefs.catweasel)
				break;
		}
		if (handle == INVALID_HANDLE_VALUE)
			catweasel_detect();
		cwc.direct_access = 0;
		if (currprefs.catweasel < 0)
			cwc.direct_access = 1;
	}

	if (handle == INVALID_HANDLE_VALUE) {
		_tcscpy (name, _T("[DIRECT]"));
		if (cwc.direct_type && ioport_init()) {
			cwc.direct_access = 1;
			if (cwc.direct_type == 4 && catweasel4_configure()) {
				cwc.type = 4;
				cwc.can_joy = 2;
				cwc.can_sid = 2;
				cwc.can_kb = 1;
				cwc.can_mouse = 2;
			} else if (cwc.direct_type == 3 && catweasel3_configure()) {
				cwc.type = 3;
				cwc.can_joy = 1;
				cwc.can_sid = 1;
				cwc.can_kb = 1;
				cwc.can_mouse = 0;
			}
		}
		if (cwc.type == 0) {
			write_log (_T("CW: No Catweasel detected\n"));
			goto fail;
		}
	}

	if (!cwc.direct_type) {
		if (!DeviceIoControl (handle, CW_GET_VERSION, 0, 0, buffer, sizeof (buffer), &len, 0)) {
			write_log (_T("CW: CW_GET_VERSION failed %d\n"), GetLastError ());
			goto fail;
		}
		s = au ((char*)buffer);
		write_log (_T("CW driver version string '%s'\n"), s);
		xfree (s);
		if (!DeviceIoControl (handle, CW_GET_HWVERSION, 0, 0, buffer, sizeof (buffer), &len, 0)) {
			write_log (_T("CW: CW_GET_HWVERSION failed %d\n"), GetLastError ());
			goto fail;
		}
		write_log (_T("CW: v=%d 14=%d 28=%d 56=%d joy=%d dpm=%d sid=%d kb=%d sidfifo=%d\n"),
			buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5],
			buffer[6], buffer[7], ((uae_u32*)(buffer + 8))[0]);
		cwc.can_joy = (buffer[4] & 1) ? 2 : 0;
		cwc.can_sid = buffer[6] ? 1 : 0;
		cwc.can_kb = buffer[7] & 1;
		cwc.can_mouse = (buffer[4] & 2) ? 2 : 0;
		if (!DeviceIoControl (handle, CW_LOCK_EXCLUSIVE, 0, 0, buffer, sizeof (buffer), &len, 0)) {
			write_log (_T("CW: CW_LOCK_EXCLUSIVE failed %d\n"), GetLastError ());
			goto fail;
		}
		model = *((uae_u32*)(buffer + 4));
		base = *((uae_u32*)(buffer + 0));
		cwc.type = model == 0 ? 1 : model == 2 ? 4 : 3;
		cwc.iobase = base;
		if (!cwc.direct_access) {
			if (!DeviceIoControl (handle, CW_UNLOCK_EXCLUSIVE, 0, 0, 0, 0, &len, 0)) {
				write_log (_T("CW: CW_UNLOCK_EXCLUSIVE failed %d\n"), GetLastError ());
			}
		}
		if (cwc.type == CATWEASEL_TYPE_MK4 && cwc.can_sid)
			cwc.can_sid = 2;
	}

	if (cwc.direct_access) {
		if (cwc.type == CATWEASEL_TYPE_MK4) {
			if (cwc.can_mouse) {
				int i;
				catweasel_do_bput (3, 0x81);
				catweasel_do_bput (0xd0, 4|8); // amiga mouse + pullups
				// clear mouse counters
				for (i = 0; i < 2; i++) {
					catweasel_do_bput (0xc4 + i * 8, 0);
					catweasel_do_bput (0xc0 + i * 8, 0);
				}
			}
			catweasel_do_bput (3, 0x41); /* enable MK3-mode */
		}
		if (cwc.can_joy)
			catweasel_do_bput (0xcc, 0); // joystick buttons = input
	}

	//catweasel_init_controller(&cwc);
	_stprintf (tmp, _T("CW: Catweasel MK%d @%08x (%s) enabled. %s."),
		cwc.type, (int)cwc.iobase, name, cwc.direct_access ? _T("DIRECTIO"): _T("API"));
	if (cwc.direct_access) {
		if (cwc.can_sid) {
			TCHAR *p = tmp + _tcslen (tmp);
			catweasel_detect_sid ();
			_stprintf (p, _T(" SID0=%d"), cwc.sid[0]);
			if (cwc.can_sid > 1) {
				p += _tcslen (p);
				_stprintf (p, _T(" SID1=%d"), cwc.sid[1]);
			}
		}
	}
	write_log (_T("%s\n"), tmp);
	detected = 1;

	return 1;
fail:
	catweasel_free ();
	return 0;

}

void catweasel_free (void)
{
	if (cwc.direct_access) {
		if (cwc.type == CATWEASEL_TYPE_MK4)
			catweasel_do_bput(3, 0x61); // enable floppy passthrough
	}
	if (handle != INVALID_HANDLE_VALUE)
		CloseHandle (handle);
	handle = INVALID_HANDLE_VALUE;
	ioport_free ();
	memset (&cwc, 0, sizeof cwc);
	mouse_x[0] = mouse_x[1] = mouse_y[0] = mouse_y[1] = 0;
	mouse_px[0] = mouse_px[1] = mouse_py[0] = mouse_py[1] = 0;
	cwmk3port = cwmk3port1 = cwmk3port2 = 0;
	cwhsync = cwmk3buttonsync = 0;
}

int catweasel_detect (void)
{
	TCHAR name[32];
	int i;
	HANDLE h;

	if (detected)
		return detected < 0 ? 0 : 1;

	detected = -1;
	for (i = 0; i < 4; i++) {
		_stprintf (name, _T("\\\\.\\CAT%u_F0"), i);
		h = CreateFile (name, GENERIC_READ, FILE_SHARE_WRITE|FILE_SHARE_READ, 0,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (h != INVALID_HANDLE_VALUE) {
			CloseHandle (h);
			write_log (_T("CW: Windows driver device detected '%s'\n"), name);
			detected = 1;
			return TRUE;
		}
	}
	if (h == INVALID_HANDLE_VALUE) {
		if (force_direct_catweasel >= 100) {
			if (ioport_init ())
				return TRUE;
			return FALSE;
		}
		if (direct_detect ()) {
			detected = 1;
			return TRUE;
		}
	}
	return FALSE;
}

#endif
