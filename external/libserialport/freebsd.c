/*
 * This file is part of the libserialport project.
 *
 * Copyright (C) 2015 Uffe Jakobsen <uffe@uffe.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * FreeBSD platform specific serial port information:
 *
 * FreeBSD has two device nodes for each serial port:
 *
 *  1) a call-in port
 *  2) a call-out port
 *
 * Quoting FreeBSD Handbook section 26.2.1
 * (https://www.freebsd.org/doc/handbook/serial.html)
 *
 * In FreeBSD, each serial port is accessed through an entry in /dev.
 * There are two different kinds of entries:
 *
 * 1) Call-in ports are named /dev/ttyuN where N is the port number,
 *    starting from zero. If a terminal is connected to the first serial port
 *    (COM1), use /dev/ttyu0 to refer to the terminal. If the terminal is on
 *    the second serial port (COM2), use /dev/ttyu1, and so forth.
 *    Generally, the call-in port is used for terminals. Call-in ports require
 *    that the serial line assert the "Data Carrier Detect" signal to work
 *    correctly.
 *
 * 2) Call-out ports are named /dev/cuauN on FreeBSD versions 10.x and higher
 *    and /dev/cuadN on FreeBSD versions 9.x and lower. Call-out ports are
 *    usually not used for terminals, but are used for modems. The call-out
 *    port can be used if the serial cable or the terminal does not support
 *    the "Data Carrier Detect" signal.
 *
 * FreeBSD also provides initialization devices (/dev/ttyuN.init and
 * /dev/cuauN.init or /dev/cuadN.init) and locking devices (/dev/ttyuN.lock
 * and /dev/cuauN.lock or /dev/cuadN.lock). The initialization devices are
 * used to initialize communications port parameters each time a port is
 * opened, such as crtscts for modems which use RTS/CTS signaling for flow
 * control. The locking devices are used to lock flags on ports to prevent
 * users or programs changing certain parameters.
 *
 * In line with the above device naming USB-serial devices have
 * the following naming:
 *
 * 1) call-in ports: /dev/ttyUN where N is the port number
 * 2) call-out ports: /dev/cuaUN where N is the port number
 *
 * See also: ucom(4), https://www.freebsd.org/cgi/man.cgi?query=ucom
 *
 * Getting USB serial port device description:
 *
 * In order to query USB serial ports for device description - the mapping
 * between the kernel device driver instances must be correlated with the
 * above mentioned device nodes.
 *
 * 1) For each USB-serial port (/dev/cuaUN) use libusb to lookup the device
 *    description and the name of the kernel device driver instance.
 * 2) Query sysctl for the kernel device driver instance info.
 * 3) Derive the ttyname and (serial) port count for the kernel device
 *    driver instance.
 * 4) If sysctl ttyname and port matches /dev/cuaUN apply the libusb
 *    device description.
 */

#include <config.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <dirent.h>
#include <libusb20.h>
#include <libusb20_desc.h>
#include "libserialport.h"
#include "libserialport_internal.h"

#define DEV_CUA_PATH "/dev/cua"

//#define DBG(...) printf(__VA_ARGS__)
#define DBG(...)

static char *strrspn(const char *s, const char *charset)
{
	char *t = (char *)s + strlen(s);
	while (t != (char *)s)
		if (!strchr(charset, *(--t)))
			return ++t;
	return t;
}

static int strend(const char *str, const char *pattern)
{
	size_t slen = strlen(str);
	size_t plen = strlen(pattern);
	if (slen >= plen)
		return (!memcmp(pattern, (str + slen - plen), plen));
	return 0;
}

static int libusb_query_port(struct libusb20_device *dev, int idx,
			     char **drv_name_str, char **drv_inst_str)
{
	int rc;
	char *j;
	char sbuf[FILENAME_MAX];

	if (!drv_name_str || !drv_inst_str)
		return -1;

	rc = libusb20_dev_kernel_driver_active(dev, idx);
	if (rc < 0)
		return rc;

	sbuf[0] = 0;
	libusb20_dev_get_iface_desc(dev, idx, (char *)&sbuf,
				    (uint8_t)sizeof(sbuf) - 1);
	if (sbuf[0] == 0)
		return rc;

	DBG("Device interface descriptor: idx=%003d '%s'\n", idx, sbuf);
	j = strchr(sbuf, ':');
	if (j > sbuf) {
		sbuf[j - sbuf] = 0;
		/*
		 * The device driver name may contain digits that
		 * is not a part of the device instance number - like "u3g".
		 */
		j = strrspn(sbuf, "0123456789");
		if (j > sbuf) {
			*drv_name_str = strndup(sbuf, j - sbuf);
			*drv_inst_str = strdup((j));
		}
	}

	return rc;
}

static int sysctl_query_dev_drv(const char *drv_name_str,
	const char *drv_inst_str, char **ttyname, int *const ttyport_cnt)
{
	int rc;
	char sbuf[FILENAME_MAX];
	char tbuf[FILENAME_MAX];
	size_t tbuf_len;

	if (!ttyname || !ttyport_cnt)
		return -1;

	snprintf(sbuf, sizeof(sbuf), "dev.%s.%s.ttyname", drv_name_str,
		 drv_inst_str);
	tbuf_len = sizeof(tbuf) - 1;
	if ((rc = sysctlbyname(sbuf, tbuf, &tbuf_len, NULL, 0)) != 0)
		return rc;

	tbuf[tbuf_len] = 0;
	*ttyname = strndup(tbuf, tbuf_len);
	DBG("sysctl: '%s' (%d) (%d): '%.*s'\n", sbuf, rc, (int)tbuf_len,
	    (int)tbuf_len, tbuf);

	snprintf(sbuf, sizeof(sbuf), "dev.%s.%s.ttyports",
		 drv_name_str, drv_inst_str);
	tbuf_len = sizeof(tbuf) - 1;
	rc = sysctlbyname(sbuf, tbuf, &tbuf_len, NULL, 0);
	if (rc == 0) {
		*ttyport_cnt = *(uint32_t *)tbuf;
		DBG("sysctl: '%s' (%d) (%d): '%d'\n", sbuf, rc,
		    (int)tbuf_len, (int)ttyport_cnt);
	} else {
		*ttyport_cnt = 0;
	}

	return rc;
}

static int populate_port_struct_from_libusb_desc(struct sp_port *const port,
						 struct libusb20_device *dev)
{
	char tbuf[FILENAME_MAX];

	/* Populate port structure from libusb description. */
	struct LIBUSB20_DEVICE_DESC_DECODED *dev_desc =
		libusb20_dev_get_device_desc(dev);

	if (!dev_desc)
		return -1;

	port->transport = SP_TRANSPORT_USB;
	port->usb_vid = dev_desc->idVendor;
	port->usb_pid = dev_desc->idProduct;
	port->usb_bus = libusb20_dev_get_bus_number(dev);
	port->usb_address = libusb20_dev_get_address(dev);
	if (libusb20_dev_req_string_simple_sync
	    (dev, dev_desc->iManufacturer, tbuf, sizeof(tbuf)) == 0) {
		port->usb_manufacturer = strdup(tbuf);
	}
	if (libusb20_dev_req_string_simple_sync
	    (dev, dev_desc->iProduct, tbuf, sizeof(tbuf)) == 0) {
		port->usb_product = strdup(tbuf);
	}
	if (libusb20_dev_req_string_simple_sync
	    (dev, dev_desc->iSerialNumber, tbuf, sizeof(tbuf)) == 0) {
		port->usb_serial = strdup(tbuf);
	}
	/* If present, add serial to description for better identification. */
	tbuf[0] = '\0';
	if (port->usb_product && port->usb_product[0])
		strncat(tbuf, port->usb_product, sizeof(tbuf) - 1);
	else
		strncat(tbuf, libusb20_dev_get_desc(dev), sizeof(tbuf) - 1);
	if (port->usb_serial && port->usb_serial[0]) {
		strncat(tbuf, " ", sizeof(tbuf) - 1);
		strncat(tbuf, port->usb_serial, sizeof(tbuf) - 1);
	}
	port->description = strdup(tbuf);
	port->bluetooth_address = NULL;

	return 0;
}

SP_PRIV enum sp_return get_port_details(struct sp_port *port)
{
	int rc;
	struct libusb20_backend *be;
	struct libusb20_device *dev, *dev_last;
	char tbuf[FILENAME_MAX];
	char *cua_sfx;
	int cua_dev_found;
	uint8_t idx;
	int sub_inst;

	DBG("Portname: '%s'\n", port->name);

	if (!strncmp(port->name, DEV_CUA_PATH, strlen(DEV_CUA_PATH))) {
		cua_sfx = port->name + strlen(DEV_CUA_PATH);
		DBG("'%s': '%s'\n", DEV_CUA_PATH, cua_sfx);
	} else {
		RETURN_ERROR(SP_ERR_ARG, "Device name not recognized");
	}

	/* Native UART enumeration. */
	if ((cua_sfx[0] == 'u') || (cua_sfx[0] == 'd')) {
		port->transport = SP_TRANSPORT_NATIVE;
		snprintf(tbuf, sizeof(tbuf), "cua%s", cua_sfx);
		port->description = strdup(tbuf);
		RETURN_OK();
	}

	/* USB device enumeration. */
	dev = dev_last = NULL;
	be = libusb20_be_alloc_default();
	cua_dev_found = 0;
	while (cua_dev_found == 0) {
		dev = libusb20_be_device_foreach(be, dev_last);
		if (!dev)
			break;

		libusb20_dev_open(dev, 0);
		DBG("Device descriptor: '%s'\n", libusb20_dev_get_desc(dev));

		for (idx = 0; idx <= UINT8_MAX - 1; idx++) {
			char *drv_name_str = NULL;
			char *drv_inst_str = NULL;
			char *ttyname = NULL;
			int ttyport_cnt;

			rc = libusb_query_port(dev, idx, &drv_name_str, &drv_inst_str);
			if (rc == 0) {
				rc = sysctl_query_dev_drv(drv_name_str,
					  drv_inst_str, &ttyname, &ttyport_cnt);
				if (rc == 0) {
					/* Handle multiple subinstances of serial ports in the same driver instance. */
					for (sub_inst = 0; sub_inst < ttyport_cnt; sub_inst++) {
						if (ttyport_cnt == 1)
							snprintf(tbuf, sizeof(tbuf), "%s", ttyname);
						else
							snprintf(tbuf, sizeof(tbuf), "%s.%d", ttyname, sub_inst);
						if (!strcmp(cua_sfx, tbuf)) {
							DBG("MATCH: '%s' == '%s'\n", cua_sfx, tbuf);
							cua_dev_found = 1;
							populate_port_struct_from_libusb_desc(port, dev);
							break; /* Break out of sub instance loop. */
						}
					}
				}
			}

			/* Clean up. */
			if (ttyname)
				free(ttyname);
			if (drv_name_str)
				free(drv_name_str);
			if (drv_inst_str)
				free(drv_inst_str);
			if (cua_dev_found)
				break; /* Break out of USB device port idx loop. */
		}
		libusb20_dev_close(dev);
		dev_last = dev;
	}
	libusb20_be_free(be);

	if (cua_dev_found == 0)
		DBG("WARN: Found no match '%s' %s'\n", port->name, cua_sfx);

	RETURN_OK();
}

SP_PRIV enum sp_return list_ports(struct sp_port ***list)
{
	DIR *dir;
	struct dirent entry;
	struct dirent *result;
	struct termios tios;
	char name[PATH_MAX];
	int fd, ret;

	DEBUG("Enumerating tty devices");
	if (!(dir = opendir("/dev")))
		RETURN_FAIL("Could not open dir /dev");

	DEBUG("Iterating over results");
	while (!readdir_r(dir, &entry, &result) && result) {
		ret = SP_OK;
		if (entry.d_type != DT_CHR)
			continue;
		if (strncmp(entry.d_name, "cuaU", 4) != 0)
			if (strncmp(entry.d_name, "cuau", 4) != 0)
				if (strncmp(entry.d_name, "cuad", 4) != 0)
					continue;
		if (strend(entry.d_name, ".init"))
			continue;
		if (strend(entry.d_name, ".lock"))
			continue;

		snprintf(name, sizeof(name), "/dev/%s", entry.d_name);
		DEBUG_FMT("Found device %s", name);

		/* Check that we can open tty/cua device in rw mode - we need that. */
		if ((fd = open(name, O_RDWR | O_NONBLOCK | O_NOCTTY | O_TTY_INIT | O_CLOEXEC)) < 0) {
			DEBUG("Open failed, skipping");
			continue;
		}

		/* Sanity check if we got a real tty. */
		if (!isatty(fd)) {
			close(fd);
			continue;
		}

		ret = tcgetattr(fd, &tios);
		close(fd);
		if (ret < 0 || cfgetospeed(&tios) <= 0 || cfgetispeed(&tios) <= 0)
			continue;

		DEBUG_FMT("Found port %s", name);
		DBG("%s: %s\n", __func__, entry.d_name);

		*list = list_append(*list, name);
		if (!list) {
			SET_ERROR(ret, SP_ERR_MEM, "List append failed");
			break;
		}
	}
	closedir(dir);

	return ret;
}
