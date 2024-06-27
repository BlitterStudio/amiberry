#ifndef C2COMM_H
#define C2COMM_H

// Commands are sent in Setup packets as Vendor.Other requests, this prevents changing the content of the packet
// bmRequestType is Vendor.Other with Data transfer direction, the type of C2 request is in bRequest
// bRequest types:
// SET sets an option, GET reads the option value as a string
// S: String, read only
// C: Command, executes the command
// V: Value, system variable
// T: Timing, system timing value

#define C2_REQ_TYPE 0x43 // Vendor.Other
#define C2_REQ_SET  0x00 // set option to 32 bit value (wValue<<16|wIndex)
#define C2_REQ_GET  0x80 // read option value
#define C2_REQ_MASK 0x7f // mask for valid options

// helpers
#define C2_REQ_TYPE_IN  (C2_REQ_TYPE|0x80)
#define C2_REQ_TYPE_OUT (C2_REQ_TYPE)

// valid requests
enum {
	c2Opt_S_Status=0,
	c2Opt_S_Info,
	c2Opt_S_Result,
	c2Opt_S_Data,
	c2Opt_S_Index,
	c2Opt_C_Reset,
	c2Opt_C_Device,
	c2Opt_C_Motor,
	c2Opt_C_Density,
	c2Opt_C_Side,
	c2Opt_C_Track,
	c2Opt_C_Stream,
	c2Opt_V_Min_Track,
	c2Opt_V_Max_Track,
	c2Opt_T_Set_Line,
	c2Opt_T_Density_Select,
	c2Opt_T_Drive_Select, 
	c2Opt_T_Side_Select,
	c2Opt_T_Direction_Select,
	c2Opt_T_Spin_Up,
	c2Opt_T_Step_After_Motor,
	c2Opt_T_Step_Signal,
	c2Opt_T_Step,
	c2Opt_T_Track0_Signal,
	c2Opt_T_Direction_Change,
	c2Opt_T_Head_Settling,
	c2Opt_T_Write_Gate_Off,
	c2Opt_T_Write_Gate_On,
	c2Opt_T_Bypass_Off,
	c2Opt_T_Bypass_On,
	c2Opt_Last
};

// Stream end result; C2OOBStreamEnd.result for reading, c2Opt_S_Result for writing
// WSIB: Write Stream Information Block
// WSSB:  Write Stream Setup Block
enum {
	c2SEROk=0,   // no error
	c2SERBuffer, // buffering problem; overflow during read, underflow during write
	c2SERIndexTimeout, // no index signal detected during timeout period
	c2SERTransferTimeout, // timeout detected during transfer
	c2SERProcessTimeout, // timeout detected during a process
	c2SERStop, // streaming stopped due to command or other request
	c2SERReset, // streaming stopped due reset request
	c2SERConnection, // streaming stopped due to usb connection state
	c2SERReceive, // streaming stopped due to error in receive
	c2SERWriteBusy, // write is in progress
	c2SERInfoSign, // invalid WSIB signature
	c2SERInfoVersion, // invalid WSIB version
	c2SERInfoFRPW, // invalid WSIB flux reversal pulse width
	c2SERInfoProcessTimeout, // invalid WSIB process timeout
	c2SERInfoSetupSize, // invalid WSIB setup block size
	c2SERInfoWriteSize, // invalid WSIB data block size
	c2SERInfoStreamSize, // invalid WSIB stream size
	c2SERSetupMissingEnd, // no SetupEnd was found in WSSB
	c2SERSetupIncomplete, // incomplete encoding in WSSB
	c2SERSetupInvalid, // invalid encoding in WSSB
	c2SERSetupIQFull, // index queue became full during the processing of WSSB
	c2SERWriteSize, // incomplete write stream (there is still data to receive, but it's not needed)
	c2SERWriteState, // incorrect write state after streaming completed
	c2SERWriteAbort, // write streaming aborted due to encoding error or executing a memory guard area
	c2SERWriteProtect, // disk is write protected
	c2SERWriteError // generic write error
};

// info request types
enum {
	c2InfoInvalid=0,
	c2InfoFirmware,
	c2InfoHardware,
	c2InfoLast
};

// streaming functions
enum {
	c2StreamStop=0,
	c2StreamRead,
	c2StreamWrite
};

// status values
enum {
	c2StatusReady=0,
	c2StatusBusy,
	c2StatusCommand
};



// A write stream has the following contents in the following order:
// 1, write stream information block
// 2, write stream setup block
// 3, write stream entries (data block)
// Except for the stream entries, all blocks must fit into the write buffer size reported by the Diag command.
// All blocks must be padded to be the smallest exact multiple of the alignment (WA) parameter reported by Diag.
// When reading stream entries, the write buffer is used as a ring buffer and the number of entries are only
// limited by the maximum allowed sequences describing each usage of the buffer.

// magic signature that enables write stream processing
#define C2_WSSIGN0 'K'
#define C2_WSSIGN1 'F'
#define C2_WSSIGN2 'W'
#define C2_WSSIGNR 1

// write stream setup commands and escape code
enum {
	c2wInvalid=0, // invalid command for error detection
	c2wSetupEnd, // setup finished, write stream start
	c2wTableidx, // define the first value that is a table entry index, arg: index start value-1 1 byte
	c2wTime2, // flux reversal time table entry, arg: entry index# 1 byte, 2 bytes big endian time
	c2wWGOn, // turn write gate on instantly during the setup process, wait the preset time for this signal
	c2wIQAdd, // add the new request to the index queue and set the next request to default values
	c2wIQActAll, // activate the entire index queue
	c2wIQWGOff, // index queue: write gate off
	c2wIQWGOn, // index queue: write gate on
	c2wIQResume2, // index queue: resume stream processing, arg: 2 bytes big endian time
	c2wIQEnd, // index queue: stop stream processing
	c2wIQWSuspend, // index queue: wait for suspend command processed by the write stream
	c2wEscape=0 // normal write stream escape code, not for setup stream
};

// A new write stream entry is either a time command or a control command.
// A time command is a byte with a direct time value or time table entry index.
// Any command value below the value defined by c2wTableidx is a direct time value.
// A command value higher than or equal to the value defined by c2wTableidx is a time table entry index.
// c2wEscape is an invalid time value and is used as an escape code, initiating a control command.
// A control command is always 4 bytes starting with the c2wEscape command byte, followed by
// 1 byte control code, 2 bytes big endian time:
// c2wEscape, control, time 15-8, time 7-0
//
// c2wTableidx should define the real value-1.
// If no table entry used set index start to 256(-1), since a command byte value will never reach 256:
// <c2wTableidx><255>
// If all values are indices, set index start to 1(-1):
// <c2wTableidx><0>
// This way it is possible to define all time commands as direct values 1-255 or all time commands
// as table indices 1-255; 0 is c2wEscape and start a control command

// WRAP:    continue stream processing with 0: next command byte; 1: buffer wrap around
// WGSET:   0: set write gate to WGVAL bit; 1: -
// WGVAL:   write gate 0: off; 1: on; WGVALB is the bit position used by the firmware
// STATE:   0: change stream processing state according to SUSPEND/END bits; 1: -
// SUSPEND: stream processing 0: stop according to END bit; 1: suspend, can be resumed by index queue
// END:     0: abort stream processing with error; 1: end stream processing, no error
// IQACT:   0: -; 1: activate index queue entry
#define C2_WSC_WRAP    0x01
#define C2_WSC_WGSET   0x02
#define C2_WSC_WGVAL   0x04
#define C2_WSC_WGVALB  2
#define C2_WSC_STATE   0x08
#define C2_WSC_SUSPEND 0x10
#define C2_WSC_END     0x20
#define C2_WSC_IQACT   0x40

// extra control bits normally not used, slightly slower to execute but by design it does not have to be fast
// each of them active if the bit value is 0, DEF is the default inactive state
#define C2_WSC_EXTCTRLMASK (C2_WSC_IQACT | C2_WSC_WGSET | C2_WSC_STATE)
#define C2_WSC_EXTCTRLDEF  (C2_WSC_WGSET | C2_WSC_STATE)

// The firmware stream buffer is guarded by 0 bytes, which if WRAP is not in the stream would eventually
// read at least 2 bytes of 0 values. Other errors are likely to result in two 0s sooner.
// <0, 0> is the code sequence for the following control command:
// <escape>, <next command byte/set write gate off/abort with error>
// In other words, a streaming error due to buffer overrun, bad commands or missing WRAP aborts stream processing
// and disables the writing hardware.



// read stream encoding (by device)
enum {
	c2eValue=8,
	c2eNop1=c2eValue,
	c2eNop2,
	c2eNop3,
	c2eOverflow16,
	c2eValue16,
	c2eOOB,
	c2eSample
};

// A read stream has a code that defines how to proceed with processing, possibly a longer code sequence.
// Nop codes are used to skip data in the buffer, ignoring the affected stream area. This makes it possible for
// the firmware to create data in its ring buffer without the need to break up a single code sequence when the
// ring buffer filling wraps.
// The overflow value represent an addition to bit 16 (ie 65536) of the final value of the cell being represented.
// There is no limit on the number of overflows present in a stream, thus the counter resolution is virtually 
// unlimited, although the decoder is using 32 bits currently.
// The OOB code starts an Out of Band information sequence, e.g. index signal, transfer status, stream information.
// A repeated OOB code makes it possible to detect the end of the read stream while reading from the device with a
// simple check regardless of the current stream alignment.
// The encoding of sample values is as follow:
// sample range: encoding
// 0x0000...0x000d: 00, sample
// 0x000e...0x00ff: sample
// 0x0100...0x07ff: sample 15:8, sample 7:0
// 0x0800...0xffff: <value16>, sample 15:8, sample 7:0
// 0x10000: <overflow> repeated for each 65536 sampling period counted, until terminated by a normal sample
// For more details see the stream decoder source.


// OOB type used in OOB Header
enum {
	c2otInvalid=0,
	c2otStreamRead,
	c2otIndex,
	c2otStreamEnd,
	c2otInfo,
	c2otEnd=c2eOOB
};

#if !defined(__GNUC__)
#pragma pack(push, 1)
#define __attribute__(x)
#endif

// write stream information block
typedef struct {
	uint8_t sign[4];  // signature
	uint32_t frpw; // flux reversal pulse width in nanoseconds
	uint32_t processtimeout; // timeout for the entire write stream process in milliseconds
	uint32_t setupsize; // size of setup block in bytes
	uint32_t writesize; // size of write entry data block in bytes
	uint32_t streamsize; // complete size of stream, sum of all block sizes including the info block
} __attribute__ ((packed)) __attribute__((aligned)) C2WSInfo;

// OOB Header
typedef struct {
	uint8_t sign;  // c2eOOB
	uint8_t type;  // type of OOB following the header
	uint16_t size; // size of following data
} __attribute__ ((packed)) __attribute__((aligned)) C2OOBHdr;

// OOB Stream Read
typedef struct {
	uint32_t streampos; // start offset of this transfer
	uint32_t trtime;    // elapsed time since last transfer in ms
} __attribute__ ((packed)) __attribute__((aligned)) C2OOBStreamRead;

// OOB Disk Index
typedef struct {
	uint32_t streampos; // next position in stream when index detected
	uint32_t timer;     // timer value when index detected
	uint32_t systime;   // system clock time when index detected
} __attribute__ ((packed)) __attribute__((aligned)) C2OOBDiskIndex;

// OOB Stream End
typedef struct {
	uint32_t streampos; // end offset of transfer
	uint32_t result;    // error code
} __attribute__ ((packed)) __attribute__((aligned)) C2OOBStreamEnd;

// all possible data types for oob messages
typedef union {
	C2OOBStreamRead read;
	C2OOBDiskIndex index;
	C2OOBStreamEnd end;
} __attribute__ ((packed)) __attribute__((aligned)) C2OOBData;

// complete oob message
typedef struct {
	C2OOBHdr header;
	C2OOBData data;
} __attribute__ ((packed)) __attribute__((aligned)) C2OOBMessage;

#if !defined(__GNUC__)
#pragma pack(pop)
#endif

#endif
