/*
 * This file is part of the libserialport project.
 *
 * Copyright (C) 2013, 2015 Martin Ling <martin-libserialport@earth.li>
 * Copyright (C) 2014 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2014 Aurelien Jacobs <aurel@gnuage.org>
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

/**
 * @mainpage libserialport API
 *
 * Introduction
 * ============
 *
 * libserialport is a minimal library written in C that is intended to take
 * care of the OS-specific details when writing software that uses serial ports.
 *
 * By writing your serial code to use libserialport, you enable it to work
 * transparently on any platform supported by the library.
 *
 * libserialport is an open source project released under the LGPL3+ license.
 *
 * The library is maintained by the [sigrok](http://sigrok.org/) project. See
 * the [libserialport homepage](http://sigrok.org/wiki/Libserialport) for the
 * latest information.
 *
 * Source code is maintained in git at
 * [git://sigrok.org/libserialport](http://sigrok.org/gitweb/?p=libserialport.git).
 *
 * Bugs are tracked at http://sigrok.org/bugzilla/.
 *
 * The library was conceived and designed by Martin Ling, is maintained by
 * Uwe Hermann, and has received contributions from several other developers.
 * See the git history for full credits.
 *
 * API information
 * ===============
 *
 * The API has been designed from scratch. It does not exactly resemble the
 * serial API of any particular operating system. Instead it aims to provide
 * a set of functions that can reliably be implemented across all operating
 * systems. These form a sufficient basis for higher level behaviour to
 * be implemented in a platform independent manner.
 *
 * If you are porting code written for a particular OS, you may find you need
 * to restructure things somewhat, or do without some specialised features.
 * For particular notes on porting existing code, see @ref Porting.
 *
 * The following subsections will help explain the principles of the API.
 *
 * Headers
 * -------
 *
 * To use libserialport functions in your code, you should include the
 * libserialport.h header, i.e. "#include <libserialport.h>".
 *
 * Namespace
 * ---------
 *
 * All identifiers defined by the public libserialport headers use the prefix
 * sp_ (for functions and data types) or SP_ (for macros and constants).
 *
 * Functions
 * ---------
 *
 * The functions provided by the library are documented in detail in
 * the following sections:
 *
 * - @ref Enumeration (obtaining a list of serial ports on the system)
 * - @ref Ports (opening, closing and getting information about ports)
 * - @ref Configuration (baud rate, parity, etc.)
 * - @ref Signals (modem control lines, breaks, etc.)
 * - @ref Data (reading and writing data, and buffer management)
 * - @ref Waiting (waiting for ports to be ready, integrating with event loops)
 * - @ref Errors (getting error and debugging information)
 *
 * Data structures
 * ---------------
 *
 * The library defines three data structures:
 *
 * - @ref sp_port, which represents a serial port.
 *   See @ref Enumeration.
 * - @ref sp_port_config, which represents a port configuration.
 *   See @ref Configuration.
 * - @ref sp_event_set, which represents a set of events.
 *   See @ref Waiting.
 *
 * All these structures are allocated and freed by library functions. It is
 * the caller's responsibility to ensure that the correct calls are made to
 * free allocated structures after use.
 *
 * Return codes and error handling
 * -------------------------------
 *
 * Most functions have return type @ref sp_return and can return only four
 * possible error values:
 *
 * - @ref SP_ERR_ARG means that a function was called with invalid
 *   arguments. This implies a bug in the caller. The arguments passed would
 *   be invalid regardless of the underlying OS or serial device involved.
 *
 * - @ref SP_ERR_FAIL means that the OS reported a failure. The error code or
 *   message provided by the OS can be obtained by calling sp_last_error_code()
 *   or sp_last_error_message().
 *
 * - @ref SP_ERR_SUPP indicates that there is no support for the requested
 *   operation in the current OS, driver or device. No error message is
 *   available from the OS in this case. There is either no way to request
 *   the operation in the first place, or libserialport does not know how to
 *   do so in the current version.
 *
 * - @ref SP_ERR_MEM indicates that a memory allocation failed.
 *
 * All of these error values are negative.
 *
 * Calls that succeed return @ref SP_OK, which is equal to zero. Some functions
 * declared @ref sp_return can also return a positive value for a successful
 * numeric result, e.g. sp_blocking_read() or sp_blocking_write().
 *
 * An error message is only available via sp_last_error_message() in the case
 * where SP_ERR_FAIL was returned by the previous function call. The error
 * message returned is that provided by the OS, using the current language
 * settings. It is an error to call sp_last_error_code() or
 * sp_last_error_message() except after a previous function call returned
 * SP_ERR_FAIL. The library does not define its own error codes or messages
 * to accompany other return codes.
 *
 * Thread safety
 * -------------
 *
 * Certain combinations of calls can be made concurrently, as follows.
 *
 * - Calls using different ports may always be made concurrently, i.e.
 *   it is safe for separate threads to handle their own ports.
 *
 * - Calls using the same port may be made concurrently when one call
 *   is a read operation and one call is a write operation, i.e. it is safe
 *   to use separate "reader" and "writer" threads for the same port. See
 *   below for which operations meet these definitions.
 *
 * Read operations:
 *
 * - sp_blocking_read()
 * - sp_blocking_read_next()
 * - sp_nonblocking_read()
 * - sp_input_waiting()
 * - sp_flush() with @ref SP_BUF_INPUT only.
 * - sp_wait() with @ref SP_EVENT_RX_READY only.
 *
 * Write operations:
 *
 * - sp_blocking_write()
 * - sp_nonblocking_write()
 * - sp_output_waiting()
 * - sp_drain()
 * - sp_flush() with @ref SP_BUF_OUTPUT only.
 * - sp_wait() with @ref SP_EVENT_TX_READY only.
 *
 * If two calls, on the same port, do not fit into one of these categories
 * each, then they may not be made concurrently.
 *
 * Debugging
 * ---------
 *
 * The library can output extensive tracing and debugging information. The
 * simplest way to use this is to set the environment variable
 * LIBSERIALPORT_DEBUG to any value; messages will then be output to the
 * standard error stream.
 *
 * This behaviour is implemented by a default debug message handling
 * callback. An alternative callback can be set using sp_set_debug_handler(),
 * in order to e.g. redirect the output elsewhere or filter it.
 *
 * No guarantees are made about the content of the debug output; it is chosen
 * to suit the needs of the developers and may change between releases.
 *
 * @anchor Porting
 * Porting
 * -------
 *
 * The following guidelines may help when porting existing OS-specific code
 * to use libserialport.
 *
 * ### Porting from Unix-like systems ###
 *
 * There are two main differences to note when porting code written for Unix.
 *
 * The first is that Unix traditionally provides a wide range of functionality
 * for dealing with serial devices at the OS level; this is exposed through the
 * termios API and dates to the days when serial terminals were common. If your
 * code relies on many of these facilities you will need to adapt it, because
 * libserialport provides only a raw binary channel with no special handling.
 *
 * The second relates to blocking versus non-blocking I/O behaviour. In
 * Unix-like systems this is normally specified by setting the O_NONBLOCK
 * flag on the file descriptor, affecting the semantics of subsequent read()
 * and write() calls.
 *
 * In libserialport, blocking and nonblocking operations are both available at
 * any time. If your existing code ѕets O_NONBLOCK, you should use
 * sp_nonblocking_read() and sp_nonblocking_write() to get the same behaviour
 * as your existing read() and write() calls. If it does not, you should use
 * sp_blocking_read() and sp_blocking_write() instead. You may also find
 * sp_blocking_read_next() useful, which reproduces the semantics of a blocking
 * read() with VTIME = 0 and VMIN = 1 set in termios.
 *
 * Finally, you should take care if your program uses custom signal handlers.
 * The blocking calls provided by libserialport will restart system calls that
 * return with EINTR, so you will need to make your own arrangements if you
 * need to interrupt blocking operations when your signal handlers are called.
 * This is not an issue if you only use the default handlers.
 *
 * ### Porting from Windows ###
 *
 * The main consideration when porting from Windows is that there is no
 * direct equivalent for overlapped I/O operations.
 *
 * If your program does not use overlapped I/O, you can simply use
 * sp_blocking_read() and sp_blocking_write() as direct equivalents for
 * ReadFile() and WriteFile(). You may also find sp_blocking_read_next()
 * useful, which reproduces the special semantics of ReadFile() with
 * ReadIntervalTimeout and ReadTotalTimeoutMultiplier set to MAXDWORD
 * and 0 < ReadTotalTimeoutConstant < MAXDWORD.
 *
 * If your program makes use of overlapped I/O to continue work while a serial
 * operation is in progress, then you can achieve the same results using
 * sp_nonblocking_read() and sp_nonblocking_write().
 *
 * Generally, overlapped I/O is combined with either waiting for completion
 * once there is no more background work to do (using WaitForSingleObject() or
 * WaitForMultipleObjects()), or periodically checking for completion with
 * GetOverlappedResult(). If the aim is to start a new operation for further
 * data once the previous one has completed, you can instead simply call the
 * nonblocking functions again with the next data. If you need to wait for
 * completion, use sp_wait() to determine when the port is ready to send or
 * receive further data.
 */

#ifndef LIBSERIALPORT_LIBSERIALPORT_H
#define LIBSERIALPORT_LIBSERIALPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/** Return values. */
enum sp_return {
	/** Operation completed successfully. */
	SP_OK = 0,
	/** Invalid arguments were passed to the function. */
	SP_ERR_ARG = -1,
	/** A system error occurred while executing the operation. */
	SP_ERR_FAIL = -2,
	/** A memory allocation failed while executing the operation. */
	SP_ERR_MEM = -3,
	/** The requested operation is not supported by this system or device. */
	SP_ERR_SUPP = -4
};

/** Port access modes. */
enum sp_mode {
	/** Open port for read access. */
	SP_MODE_READ = 1,
	/** Open port for write access. */
	SP_MODE_WRITE = 2,
	/** Open port for read and write access. @since 0.1.1 */
	SP_MODE_READ_WRITE = 3
};

/** Port events. */
enum sp_event {
	/** Data received and ready to read. */
	SP_EVENT_RX_READY = 1,
	/** Ready to transmit new data. */
	SP_EVENT_TX_READY = 2,
	/** Error occurred. */
	SP_EVENT_ERROR = 4
};

/** Buffer selection. */
enum sp_buffer {
	/** Input buffer. */
	SP_BUF_INPUT = 1,
	/** Output buffer. */
	SP_BUF_OUTPUT = 2,
	/** Both buffers. */
	SP_BUF_BOTH = 3
};

/** Parity settings. */
enum sp_parity {
	/** Special value to indicate setting should be left alone. */
	SP_PARITY_INVALID = -1,
	/** No parity. */
	SP_PARITY_NONE = 0,
	/** Odd parity. */
	SP_PARITY_ODD = 1,
	/** Even parity. */
	SP_PARITY_EVEN = 2,
	/** Mark parity. */
	SP_PARITY_MARK = 3,
	/** Space parity. */
	SP_PARITY_SPACE = 4
};

/** RTS pin behaviour. */
enum sp_rts {
	/** Special value to indicate setting should be left alone. */
	SP_RTS_INVALID = -1,
	/** RTS off. */
	SP_RTS_OFF = 0,
	/** RTS on. */
	SP_RTS_ON = 1,
	/** RTS used for flow control. */
	SP_RTS_FLOW_CONTROL = 2
};

/** CTS pin behaviour. */
enum sp_cts {
	/** Special value to indicate setting should be left alone. */
	SP_CTS_INVALID = -1,
	/** CTS ignored. */
	SP_CTS_IGNORE = 0,
	/** CTS used for flow control. */
	SP_CTS_FLOW_CONTROL = 1
};

/** DTR pin behaviour. */
enum sp_dtr {
	/** Special value to indicate setting should be left alone. */
	SP_DTR_INVALID = -1,
	/** DTR off. */
	SP_DTR_OFF = 0,
	/** DTR on. */
	SP_DTR_ON = 1,
	/** DTR used for flow control. */
	SP_DTR_FLOW_CONTROL = 2
};

/** DSR pin behaviour. */
enum sp_dsr {
	/** Special value to indicate setting should be left alone. */
	SP_DSR_INVALID = -1,
	/** DSR ignored. */
	SP_DSR_IGNORE = 0,
	/** DSR used for flow control. */
	SP_DSR_FLOW_CONTROL = 1
};

/** XON/XOFF flow control behaviour. */
enum sp_xonxoff {
	/** Special value to indicate setting should be left alone. */
	SP_XONXOFF_INVALID = -1,
	/** XON/XOFF disabled. */
	SP_XONXOFF_DISABLED = 0,
	/** XON/XOFF enabled for input only. */
	SP_XONXOFF_IN = 1,
	/** XON/XOFF enabled for output only. */
	SP_XONXOFF_OUT = 2,
	/** XON/XOFF enabled for input and output. */
	SP_XONXOFF_INOUT = 3
};

/** Standard flow control combinations. */
enum sp_flowcontrol {
	/** No flow control. */
	SP_FLOWCONTROL_NONE = 0,
	/** Software flow control using XON/XOFF characters. */
	SP_FLOWCONTROL_XONXOFF = 1,
	/** Hardware flow control using RTS/CTS signals. */
	SP_FLOWCONTROL_RTSCTS = 2,
	/** Hardware flow control using DTR/DSR signals. */
	SP_FLOWCONTROL_DTRDSR = 3
};

/** Input signals. */
enum sp_signal {
	/** Clear to send. */
	SP_SIG_CTS = 1,
	/** Data set ready. */
	SP_SIG_DSR = 2,
	/** Data carrier detect. */
	SP_SIG_DCD = 4,
	/** Ring indicator. */
	SP_SIG_RI = 8
};

/**
 * Transport types.
 *
 * @since 0.1.1
 */
enum sp_transport {
	/** Native platform serial port. @since 0.1.1 */
	SP_TRANSPORT_NATIVE,
	/** USB serial port adapter. @since 0.1.1 */
	SP_TRANSPORT_USB,
	/** Bluetooth serial port adapter. @since 0.1.1 */
	SP_TRANSPORT_BLUETOOTH
};

/**
 * @struct sp_port
 * An opaque structure representing a serial port.
 */
struct sp_port;

/**
 * @struct sp_port_config
 * An opaque structure representing the configuration for a serial port.
 */
struct sp_port_config;

/**
 * @struct sp_event_set
 * A set of handles to wait on for events.
 */
struct sp_event_set {
	/** Array of OS-specific handles. */
	void *handles;
	/** Array of bitmasks indicating which events apply for each handle. */
	enum sp_event *masks;
	/** Number of handles. */
	unsigned int count;
};

/**
 * @defgroup Enumeration Port enumeration
 *
 * Enumerating the serial ports of a system.
 *
 * @{
 */

/**
 * Obtain a pointer to a new sp_port structure representing the named port.
 *
 * The user should allocate a variable of type "struct sp_port *" and pass a
 * pointer to this to receive the result.
 *
 * The result should be freed after use by calling sp_free_port().
 *
 * @param[in] portname The OS-specific name of a serial port. Must not be NULL.
 * @param[out] port_ptr If any error is returned, the variable pointed to by
 *                      port_ptr will be set to NULL. Otherwise, it will be set
 *                      to point to the newly allocated port. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_port_by_name(const char *portname, struct sp_port **port_ptr);

/**
 * Free a port structure obtained from sp_get_port_by_name() or sp_copy_port().
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @since 0.1.0
 */
void sp_free_port(struct sp_port *port);

/**
 * List the serial ports available on the system.
 *
 * The result obtained is an array of pointers to sp_port structures,
 * terminated by a NULL. The user should allocate a variable of type
 * "struct sp_port **" and pass a pointer to this to receive the result.
 *
 * The result should be freed after use by calling sp_free_port_list().
 * If a port from the list is to be used after freeing the list, it must be
 * copied first using sp_copy_port().
 *
 * @param[out] list_ptr If any error is returned, the variable pointed to by
 *                      list_ptr will be set to NULL. Otherwise, it will be set
 *                      to point to the newly allocated array. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_list_ports(struct sp_port ***list_ptr);

/**
 * Make a new copy of an sp_port structure.
 *
 * The user should allocate a variable of type "struct sp_port *" and pass a
 * pointer to this to receive the result.
 *
 * The copy should be freed after use by calling sp_free_port().
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[out] copy_ptr If any error is returned, the variable pointed to by
 *                      copy_ptr will be set to NULL. Otherwise, it will be set
 *                      to point to the newly allocated copy. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_copy_port(const struct sp_port *port, struct sp_port **copy_ptr);

/**
 * Free a port list obtained from sp_list_ports().
 *
 * This will also free all the sp_port structures referred to from the list;
 * any that are to be retained must be copied first using sp_copy_port().
 *
 * @param[in] ports Pointer to a list of port structures. Must not be NULL.
 *
 * @since 0.1.0
 */
void sp_free_port_list(struct sp_port **ports);

/**
 * @}
 * @defgroup Ports Port handling
 *
 * Opening, closing and querying ports.
 *
 * @{
 */

/**
 * Open the specified serial port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] flags Flags to use when opening the serial port.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_open(struct sp_port *port, enum sp_mode flags);

/**
 * Close the specified serial port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_close(struct sp_port *port);

/**
 * Get the name of a port.
 *
 * The name returned is whatever is normally used to refer to a port on the
 * current operating system; e.g. for Windows it will usually be a "COMn"
 * device name, and for Unix it will be a device path beginning with "/dev/".
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return The port name, or NULL if an invalid port is passed. The name
 *         string is part of the port structure and may not be used after
 *         the port structure has been freed.
 *
 * @since 0.1.0
 */
char *sp_get_port_name(const struct sp_port *port);

/**
 * Get a description for a port, to present to end user.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return The port description, or NULL if an invalid port is passed.
 *         The description string is part of the port structure and may not
 *         be used after the port structure has been freed.
 *
 * @since 0.1.1
 */
char *sp_get_port_description(const struct sp_port *port);

/**
 * Get the transport type used by a port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return The port transport type.
 *
 * @since 0.1.1
 */
enum sp_transport sp_get_port_transport(const struct sp_port *port);

/**
 * Get the USB bus number and address on bus of a USB serial adapter port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[out] usb_bus Pointer to a variable to store the USB bus.
 *                     Can be NULL (in that case it will be ignored).
 * @param[out] usb_address Pointer to a variable to store the USB address.
 *                         Can be NULL (in that case it will be ignored).
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.1
 */
enum sp_return sp_get_port_usb_bus_address(const struct sp_port *port,
                                           int *usb_bus, int *usb_address);

/**
 * Get the USB Vendor ID and Product ID of a USB serial adapter port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[out] usb_vid Pointer to a variable to store the USB VID.
 *                     Can be NULL (in that case it will be ignored).
 * @param[out] usb_pid Pointer to a variable to store the USB PID.
 *                     Can be NULL (in that case it will be ignored).
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.1
 */
enum sp_return sp_get_port_usb_vid_pid(const struct sp_port *port, int *usb_vid, int *usb_pid);

/**
 * Get the USB manufacturer string of a USB serial adapter port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return The port manufacturer string, or NULL if an invalid port is passed.
 *         The manufacturer string is part of the port structure and may not
 *         be used after the port structure has been freed.
 *
 * @since 0.1.1
 */
char *sp_get_port_usb_manufacturer(const struct sp_port *port);

/**
 * Get the USB product string of a USB serial adapter port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return The port product string, or NULL if an invalid port is passed.
 *         The product string is part of the port structure and may not be
 *         used after the port structure has been freed.
 *
 * @since 0.1.1
 */
char *sp_get_port_usb_product(const struct sp_port *port);

/**
 * Get the USB serial number string of a USB serial adapter port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return The port serial number, or NULL if an invalid port is passed.
 *         The serial number string is part of the port structure and may
 *         not be used after the port structure has been freed.
 *
 * @since 0.1.1
 */
char *sp_get_port_usb_serial(const struct sp_port *port);

/**
 * Get the MAC address of a Bluetooth serial adapter port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return The port MAC address, or NULL if an invalid port is passed.
 *         The MAC address string is part of the port structure and may not
 *         be used after the port structure has been freed.
 *
 * @since 0.1.1
 */
char *sp_get_port_bluetooth_address(const struct sp_port *port);

/**
 * Get the operating system handle for a port.
 *
 * The type of the handle depends on the operating system. On Unix based
 * systems, the handle is a file descriptor of type "int". On Windows, the
 * handle is of type "HANDLE". The user should allocate a variable of the
 * appropriate type and pass a pointer to this to receive the result.
 *
 * To obtain a valid handle, the port must first be opened by calling
 * sp_open() using the same port structure.
 *
 * After the port is closed or the port structure freed, the handle may
 * no longer be valid.
 *
 * @warning This feature is provided so that programs may make use of
 *          OS-specific functionality where desired. Doing so obviously
 *          comes at a cost in portability. It also cannot be guaranteed
 *          that direct usage of the OS handle will not conflict with the
 *          library's own usage of the port. Be careful.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[out] result_ptr If any error is returned, the variable pointed to by
 *                        result_ptr will have unknown contents and should not
 *                        be used. Otherwise, it will be set to point to the
 *                        OS handle. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_port_handle(const struct sp_port *port, void *result_ptr);

/**
 * @}
 *
 * @defgroup Configuration Configuration
 *
 * Setting and querying serial port parameters.
 * @{
 */

/**
 * Allocate a port configuration structure.
 *
 * The user should allocate a variable of type "struct sp_port_config *" and
 * pass a pointer to this to receive the result. The variable will be updated
 * to point to the new configuration structure. The structure is opaque and
 * must be accessed via the functions provided.
 *
 * All parameters in the structure will be initialised to special values which
 * are ignored by sp_set_config().
 *
 * The structure should be freed after use by calling sp_free_config().
 *
 * @param[out] config_ptr If any error is returned, the variable pointed to by
 *                        config_ptr will be set to NULL. Otherwise, it will
 *                        be set to point to the allocated config structure.
 *                        Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_new_config(struct sp_port_config **config_ptr);

/**
 * Free a port configuration structure.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 *
 * @since 0.1.0
 */
void sp_free_config(struct sp_port_config *config);

/**
 * Get the current configuration of the specified serial port.
 *
 * The user should allocate a configuration structure using sp_new_config()
 * and pass this as the config parameter. The configuration structure will
 * be updated with the port configuration.
 *
 * Any parameters that are configured with settings not recognised or
 * supported by libserialport, will be set to special values that are
 * ignored by sp_set_config().
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[out] config Pointer to a configuration structure that will hold
 *                    the result. Upon errors the contents of the config
 *                    struct will not be changed. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_config(struct sp_port *port, struct sp_port_config *config);

/**
 * Set the configuration for the specified serial port.
 *
 * For each parameter in the configuration, there is a special value (usually
 * -1, but see the documentation for each field). These values will be ignored
 * and the corresponding setting left unchanged on the port.
 *
 * Upon errors, the configuration of the serial port is unknown since
 * partial/incomplete config updates may have happened.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_config(struct sp_port *port, const struct sp_port_config *config);

/**
 * Set the baud rate for the specified serial port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] baudrate Baud rate in bits per second.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_baudrate(struct sp_port *port, int baudrate);

/**
 * Get the baud rate from a port configuration.
 *
 * The user should allocate a variable of type int and
 * pass a pointer to this to receive the result.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[out] baudrate_ptr Pointer to a variable to store the result. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_config_baudrate(const struct sp_port_config *config, int *baudrate_ptr);

/**
 * Set the baud rate in a port configuration.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[in] baudrate Baud rate in bits per second, or -1 to retain the current setting.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_config_baudrate(struct sp_port_config *config, int baudrate);

/**
 * Set the data bits for the specified serial port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] bits Number of data bits.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_bits(struct sp_port *port, int bits);

/**
 * Get the data bits from a port configuration.
 *
 * The user should allocate a variable of type int and
 * pass a pointer to this to receive the result.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[out] bits_ptr Pointer to a variable to store the result. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_config_bits(const struct sp_port_config *config, int *bits_ptr);

/**
 * Set the data bits in a port configuration.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[in] bits Number of data bits, or -1 to retain the current setting.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_config_bits(struct sp_port_config *config, int bits);

/**
 * Set the parity setting for the specified serial port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] parity Parity setting.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_parity(struct sp_port *port, enum sp_parity parity);

/**
 * Get the parity setting from a port configuration.
 *
 * The user should allocate a variable of type enum sp_parity and
 * pass a pointer to this to receive the result.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[out] parity_ptr Pointer to a variable to store the result. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_config_parity(const struct sp_port_config *config, enum sp_parity *parity_ptr);

/**
 * Set the parity setting in a port configuration.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[in] parity Parity setting, or -1 to retain the current setting.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_config_parity(struct sp_port_config *config, enum sp_parity parity);

/**
 * Set the stop bits for the specified serial port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] stopbits Number of stop bits.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_stopbits(struct sp_port *port, int stopbits);

/**
 * Get the stop bits from a port configuration.
 *
 * The user should allocate a variable of type int and
 * pass a pointer to this to receive the result.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[out] stopbits_ptr Pointer to a variable to store the result. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_config_stopbits(const struct sp_port_config *config, int *stopbits_ptr);

/**
 * Set the stop bits in a port configuration.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[in] stopbits Number of stop bits, or -1 to retain the current setting.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_config_stopbits(struct sp_port_config *config, int stopbits);

/**
 * Set the RTS pin behaviour for the specified serial port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] rts RTS pin mode.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_rts(struct sp_port *port, enum sp_rts rts);

/**
 * Get the RTS pin behaviour from a port configuration.
 *
 * The user should allocate a variable of type enum sp_rts and
 * pass a pointer to this to receive the result.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[out] rts_ptr Pointer to a variable to store the result. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_config_rts(const struct sp_port_config *config, enum sp_rts *rts_ptr);

/**
 * Set the RTS pin behaviour in a port configuration.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[in] rts RTS pin mode, or -1 to retain the current setting.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_config_rts(struct sp_port_config *config, enum sp_rts rts);

/**
 * Set the CTS pin behaviour for the specified serial port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] cts CTS pin mode.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_cts(struct sp_port *port, enum sp_cts cts);

/**
 * Get the CTS pin behaviour from a port configuration.
 *
 * The user should allocate a variable of type enum sp_cts and
 * pass a pointer to this to receive the result.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[out] cts_ptr Pointer to a variable to store the result. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_config_cts(const struct sp_port_config *config, enum sp_cts *cts_ptr);

/**
 * Set the CTS pin behaviour in a port configuration.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[in] cts CTS pin mode, or -1 to retain the current setting.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_config_cts(struct sp_port_config *config, enum sp_cts cts);

/**
 * Set the DTR pin behaviour for the specified serial port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] dtr DTR pin mode.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_dtr(struct sp_port *port, enum sp_dtr dtr);

/**
 * Get the DTR pin behaviour from a port configuration.
 *
 * The user should allocate a variable of type enum sp_dtr and
 * pass a pointer to this to receive the result.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[out] dtr_ptr Pointer to a variable to store the result. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_config_dtr(const struct sp_port_config *config, enum sp_dtr *dtr_ptr);

/**
 * Set the DTR pin behaviour in a port configuration.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[in] dtr DTR pin mode, or -1 to retain the current setting.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_config_dtr(struct sp_port_config *config, enum sp_dtr dtr);

/**
 * Set the DSR pin behaviour for the specified serial port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] dsr DSR pin mode.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_dsr(struct sp_port *port, enum sp_dsr dsr);

/**
 * Get the DSR pin behaviour from a port configuration.
 *
 * The user should allocate a variable of type enum sp_dsr and
 * pass a pointer to this to receive the result.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[out] dsr_ptr Pointer to a variable to store the result. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_config_dsr(const struct sp_port_config *config, enum sp_dsr *dsr_ptr);

/**
 * Set the DSR pin behaviour in a port configuration.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[in] dsr DSR pin mode, or -1 to retain the current setting.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_config_dsr(struct sp_port_config *config, enum sp_dsr dsr);

/**
 * Set the XON/XOFF configuration for the specified serial port.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] xon_xoff XON/XOFF mode.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_xon_xoff(struct sp_port *port, enum sp_xonxoff xon_xoff);

/**
 * Get the XON/XOFF configuration from a port configuration.
 *
 * The user should allocate a variable of type enum sp_xonxoff and
 * pass a pointer to this to receive the result.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[out] xon_xoff_ptr Pointer to a variable to store the result. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_config_xon_xoff(const struct sp_port_config *config, enum sp_xonxoff *xon_xoff_ptr);

/**
 * Set the XON/XOFF configuration in a port configuration.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[in] xon_xoff XON/XOFF mode, or -1 to retain the current setting.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_config_xon_xoff(struct sp_port_config *config, enum sp_xonxoff xon_xoff);

/**
 * Set the flow control type in a port configuration.
 *
 * This function is a wrapper that sets the RTS, CTS, DTR, DSR and
 * XON/XOFF settings as necessary for the specified flow control
 * type. For more fine-grained control of these settings, use their
 * individual configuration functions.
 *
 * @param[in] config Pointer to a configuration structure. Must not be NULL.
 * @param[in] flowcontrol Flow control setting to use.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_config_flowcontrol(struct sp_port_config *config, enum sp_flowcontrol flowcontrol);

/**
 * Set the flow control type for the specified serial port.
 *
 * This function is a wrapper that sets the RTS, CTS, DTR, DSR and
 * XON/XOFF settings as necessary for the specified flow control
 * type. For more fine-grained control of these settings, use their
 * individual configuration functions.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] flowcontrol Flow control setting to use.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_set_flowcontrol(struct sp_port *port, enum sp_flowcontrol flowcontrol);

/**
 * @}
 *
 * @defgroup Data Data handling
 *
 * Reading, writing, and flushing data.
 *
 * @{
 */

/**
 * Read bytes from the specified serial port, blocking until complete.
 *
 * @warning If your program runs on Unix, defines its own signal handlers, and
 *          needs to abort blocking reads when these are called, then you
 *          should not use this function. It repeats system calls that return
 *          with EINTR. To be able to abort a read from a signal handler, you
 *          should implement your own blocking read using sp_nonblocking_read()
 *          together with a blocking method that makes sense for your program.
 *          E.g. you can obtain the file descriptor for an open port using
 *          sp_get_port_handle() and use this to call select() or pselect(),
 *          with appropriate arrangements to return if a signal is received.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[out] buf Buffer in which to store the bytes read. Must not be NULL.
 * @param[in] count Requested number of bytes to read.
 * @param[in] timeout_ms Timeout in milliseconds, or zero to wait indefinitely.
 *
 * @return The number of bytes read on success, or a negative error code. If
 *         the number of bytes returned is less than that requested, the
 *         timeout was reached before the requested number of bytes was
 *         available. If timeout is zero, the function will always return
 *         either the requested number of bytes or a negative error code.
 *
 * @since 0.1.0
 */
enum sp_return sp_blocking_read(struct sp_port *port, void *buf, size_t count, unsigned int timeout_ms);

/**
 * Read bytes from the specified serial port, returning as soon as any data is
 * available.
 *
 * @warning If your program runs on Unix, defines its own signal handlers, and
 *          needs to abort blocking reads when these are called, then you
 *          should not use this function. It repeats system calls that return
 *          with EINTR. To be able to abort a read from a signal handler, you
 *          should implement your own blocking read using sp_nonblocking_read()
 *          together with a blocking method that makes sense for your program.
 *          E.g. you can obtain the file descriptor for an open port using
 *          sp_get_port_handle() and use this to call select() or pselect(),
 *          with appropriate arrangements to return if a signal is received.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[out] buf Buffer in which to store the bytes read. Must not be NULL.
 * @param[in] count Maximum number of bytes to read. Must not be zero.
 * @param[in] timeout_ms Timeout in milliseconds, or zero to wait indefinitely.
 *
 * @return The number of bytes read on success, or a negative error code. If
 *         the result is zero, the timeout was reached before any bytes were
 *         available. If timeout_ms is zero, the function will always return
 *         either at least one byte, or a negative error code.
 *
 * @since 0.1.1
 */
enum sp_return sp_blocking_read_next(struct sp_port *port, void *buf, size_t count, unsigned int timeout_ms);

/**
 * Read bytes from the specified serial port, without blocking.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[out] buf Buffer in which to store the bytes read. Must not be NULL.
 * @param[in] count Maximum number of bytes to read.
 *
 * @return The number of bytes read on success, or a negative error code. The
 *         number of bytes returned may be any number from zero to the maximum
 *         that was requested.
 *
 * @since 0.1.0
 */
enum sp_return sp_nonblocking_read(struct sp_port *port, void *buf, size_t count);

/**
 * Write bytes to the specified serial port, blocking until complete.
 *
 * Note that this function only ensures that the accepted bytes have been
 * written to the OS; they may be held in driver or hardware buffers and not
 * yet physically transmitted. To check whether all written bytes have actually
 * been transmitted, use the sp_output_waiting() function. To wait until all
 * written bytes have actually been transmitted, use the sp_drain() function.
 *
 * @warning If your program runs on Unix, defines its own signal handlers, and
 *          needs to abort blocking writes when these are called, then you
 *          should not use this function. It repeats system calls that return
 *          with EINTR. To be able to abort a write from a signal handler, you
 *          should implement your own blocking write using sp_nonblocking_write()
 *          together with a blocking method that makes sense for your program.
 *          E.g. you can obtain the file descriptor for an open port using
 *          sp_get_port_handle() and use this to call select() or pselect(),
 *          with appropriate arrangements to return if a signal is received.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] buf Buffer containing the bytes to write. Must not be NULL.
 * @param[in] count Requested number of bytes to write.
 * @param[in] timeout_ms Timeout in milliseconds, or zero to wait indefinitely.
 *
 * @return The number of bytes written on success, or a negative error code.
 *         If the number of bytes returned is less than that requested, the
 *         timeout was reached before the requested number of bytes was
 *         written. If timeout is zero, the function will always return
 *         either the requested number of bytes or a negative error code. In
 *         the event of an error there is no way to determine how many bytes
 *         were sent before the error occurred.
 *
 * @since 0.1.0
 */
enum sp_return sp_blocking_write(struct sp_port *port, const void *buf, size_t count, unsigned int timeout_ms);

/**
 * Write bytes to the specified serial port, without blocking.
 *
 * Note that this function only ensures that the accepted bytes have been
 * written to the OS; they may be held in driver or hardware buffers and not
 * yet physically transmitted. To check whether all written bytes have actually
 * been transmitted, use the sp_output_waiting() function. To wait until all
 * written bytes have actually been transmitted, use the sp_drain() function.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] buf Buffer containing the bytes to write. Must not be NULL.
 * @param[in] count Maximum number of bytes to write.
 *
 * @return The number of bytes written on success, or a negative error code.
 *         The number of bytes returned may be any number from zero to the
 *         maximum that was requested.
 *
 * @since 0.1.0
 */
enum sp_return sp_nonblocking_write(struct sp_port *port, const void *buf, size_t count);

/**
 * Gets the number of bytes waiting in the input buffer.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return Number of bytes waiting on success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_input_waiting(struct sp_port *port);

/**
 * Gets the number of bytes waiting in the output buffer.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return Number of bytes waiting on success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_output_waiting(struct sp_port *port);

/**
 * Flush serial port buffers. Data in the selected buffer(s) is discarded.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] buffers Which buffer(s) to flush.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_flush(struct sp_port *port, enum sp_buffer buffers);

/**
 * Wait for buffered data to be transmitted.
 *
 * @warning If your program runs on Unix, defines its own signal handlers, and
 *          needs to abort draining the output buffer when when these are
 *          called, then you should not use this function. It repeats system
 *          calls that return with EINTR. To be able to abort a drain from a
 *          signal handler, you would need to implement your own blocking
 *          drain by polling the result of sp_output_waiting().
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_drain(struct sp_port *port);

/**
 * @}
 *
 * @defgroup Waiting Waiting
 *
 * Waiting for events and timeout handling.
 *
 * @{
 */

/**
 * Allocate storage for a set of events.
 *
 * The user should allocate a variable of type struct sp_event_set *,
 * then pass a pointer to this variable to receive the result.
 *
 * The result should be freed after use by calling sp_free_event_set().
 *
 * @param[out] result_ptr If any error is returned, the variable pointed to by
 *                        result_ptr will be set to NULL. Otherwise, it will
 *                        be set to point to the event set. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_new_event_set(struct sp_event_set **result_ptr);

/**
 * Add events to a struct sp_event_set for a given port.
 *
 * The port must first be opened by calling sp_open() using the same port
 * structure.
 *
 * After the port is closed or the port structure freed, the results may
 * no longer be valid.
 *
 * @param[in,out] event_set Event set to update. Must not be NULL.
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[in] mask Bitmask of events to be waited for.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_add_port_events(struct sp_event_set *event_set,
	const struct sp_port *port, enum sp_event mask);

/**
 * Wait for any of a set of events to occur.
 *
 * @param[in] event_set Event set to wait on. Must not be NULL.
 * @param[in] timeout_ms Timeout in milliseconds, or zero to wait indefinitely.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_wait(struct sp_event_set *event_set, unsigned int timeout_ms);

/**
 * Free a structure allocated by sp_new_event_set().
 *
 * @param[in] event_set Event set to free. Must not be NULL.
 *
 * @since 0.1.0
 */
void sp_free_event_set(struct sp_event_set *event_set);

/**
 * @}
 *
 * @defgroup Signals Signals
 *
 * Port signalling operations.
 *
 * @{
 */

/**
 * Gets the status of the control signals for the specified port.
 *
 * The user should allocate a variable of type "enum sp_signal" and pass a
 * pointer to this variable to receive the result. The result is a bitmask
 * in which individual signals can be checked by bitwise OR with values of
 * the sp_signal enum.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 * @param[out] signal_mask Pointer to a variable to receive the result.
 *                         Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_get_signals(struct sp_port *port, enum sp_signal *signal_mask);

/**
 * Put the port transmit line into the break state.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_start_break(struct sp_port *port);

/**
 * Take the port transmit line out of the break state.
 *
 * @param[in] port Pointer to a port structure. Must not be NULL.
 *
 * @return SP_OK upon success, a negative error code otherwise.
 *
 * @since 0.1.0
 */
enum sp_return sp_end_break(struct sp_port *port);

/**
 * @}
 *
 * @defgroup Errors Errors
 *
 * Obtaining error information.
 *
 * @{
 */

/**
 * Get the error code for a failed operation.
 *
 * In order to obtain the correct result, this function should be called
 * straight after the failure, before executing any other system operations.
 * The result is thread-specific, and only valid when called immediately
 * after a previous call returning SP_ERR_FAIL.
 *
 * @return The system's numeric code for the error that caused the last
 *         operation to fail.
 *
 * @since 0.1.0
 */
int sp_last_error_code(void);

/**
 * Get the error message for a failed operation.
 *
 * In order to obtain the correct result, this function should be called
 * straight after the failure, before executing other system operations.
 * The result is thread-specific, and only valid when called immediately
 * after a previous call returning SP_ERR_FAIL.
 *
 * @return The system's message for the error that caused the last
 *         operation to fail. This string may be allocated by the function,
 *         and should be freed after use by calling sp_free_error_message().
 *
 * @since 0.1.0
 */
char *sp_last_error_message(void);

/**
 * Free an error message returned by sp_last_error_message().
 *
 * @param[in] message The error message string to free. Must not be NULL.
 *
 * @since 0.1.0
 */
void sp_free_error_message(char *message);

/**
 * Set the handler function for library debugging messages.
 *
 * Debugging messages are generated by the library during each operation,
 * to help in diagnosing problems. The handler will be called for each
 * message. The handler can be set to NULL to ignore all debug messages.
 *
 * The handler function should accept a format string and variable length
 * argument list, in the same manner as e.g. printf().
 *
 * The default handler is sp_default_debug_handler().
 *
 * @param[in] handler The handler function to use. Can be NULL (in that case
 *                    all debug messages will be ignored).
 *
 * @since 0.1.0
 */
void sp_set_debug_handler(void (*handler)(const char *format, ...));

/**
 * Default handler function for library debugging messages.
 *
 * This function prints debug messages to the standard error stream if the
 * environment variable LIBSERIALPORT_DEBUG is set. Otherwise, they are
 * ignored.
 *
 * @param[in] format The format string to use. Must not be NULL.
 * @param[in] ... The variable length argument list to use.
 *
 * @since 0.1.0
 */
void sp_default_debug_handler(const char *format, ...);

/** @} */

/**
 * @defgroup Versions Versions
 *
 * Version number querying functions, definitions, and macros.
 *
 * This set of API calls returns two different version numbers related
 * to libserialport. The "package version" is the release version number of the
 * libserialport tarball in the usual "major.minor.micro" format, e.g. "0.1.0".
 *
 * The "library version" is independent of that; it is the libtool version
 * number in the "current:revision:age" format, e.g. "2:0:0".
 * See http://www.gnu.org/software/libtool/manual/libtool.html#Libtool-versioning for details.
 *
 * Both version numbers (and/or individual components of them) can be
 * retrieved via the API calls at runtime, and/or they can be checked at
 * compile/preprocessor time using the respective macros.
 *
 * @{
 */

/*
 * Package version macros (can be used for conditional compilation).
 */

/** The libserialport package 'major' version number. */
#undef SP_PACKAGE_VERSION_MAJOR

/** The libserialport package 'minor' version number. */
#undef SP_PACKAGE_VERSION_MINOR

/** The libserialport package 'micro' version number. */
#undef SP_PACKAGE_VERSION_MICRO

/** The libserialport package version ("major.minor.micro") as string. */
#undef SP_PACKAGE_VERSION_STRING

/*
 * Library/libtool version macros (can be used for conditional compilation).
 */

/** The libserialport libtool 'current' version number. */
#undef SP_LIB_VERSION_CURRENT

/** The libserialport libtool 'revision' version number. */
#undef SP_LIB_VERSION_REVISION

/** The libserialport libtool 'age' version number. */
#undef SP_LIB_VERSION_AGE

/** The libserialport libtool version ("current:revision:age") as string. */
#undef SP_LIB_VERSION_STRING

/**
 * Get the major libserialport package version number.
 *
 * @return The major package version number.
 *
 * @since 0.1.0
 */
int sp_get_major_package_version(void);

/**
 * Get the minor libserialport package version number.
 *
 * @return The minor package version number.
 *
 * @since 0.1.0
 */
int sp_get_minor_package_version(void);

/**
 * Get the micro libserialport package version number.
 *
 * @return The micro package version number.
 *
 * @since 0.1.0
 */
int sp_get_micro_package_version(void);

/**
 * Get the libserialport package version number as a string.
 *
 * @return The package version number string. The returned string is
 *         static and thus should NOT be free'd by the caller.
 *
 * @since 0.1.0
 */
const char *sp_get_package_version_string(void);

/**
 * Get the "current" part of the libserialport library version number.
 *
 * @return The "current" library version number.
 *
 * @since 0.1.0
 */
int sp_get_current_lib_version(void);

/**
 * Get the "revision" part of the libserialport library version number.
 *
 * @return The "revision" library version number.
 *
 * @since 0.1.0
 */
int sp_get_revision_lib_version(void);

/**
 * Get the "age" part of the libserialport library version number.
 *
 * @return The "age" library version number.
 *
 * @since 0.1.0
 */
int sp_get_age_lib_version(void);

/**
 * Get the libserialport library version number as a string.
 *
 * @return The library version number string. The returned string is
 *         static and thus should NOT be free'd by the caller.
 *
 * @since 0.1.0
 */
const char *sp_get_lib_version_string(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
