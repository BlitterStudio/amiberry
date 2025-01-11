#include "sysconfig.h"
#include "sysdeps.h"

#include "uae/types.h"

#define LSB_FIRST

extern void write_log(const char *, ...);
#ifdef DEBUGGER
extern void activate_debugger(void);
#endif

#define MIN(a, b) ((a) > (b) ? (a) : (b))

#define osd_printf_debug write_log
#define fatalerror write_log
#define logerror write_log
#define tag() "X"
#define DEBUG_FLAG_ENABLED 0

#define STATE_GENPC 0
#define STATE_GENSP 1
#define STATE_GENPCBASE 2

#define CLEAR_LINE 1

typedef unsigned long offs_t;

#define FALSE 0
#ifndef TRUE
#define TRUE 1
#endif

#define TIMER_CALLBACK_MEMBER(x) int x(void *p, int param, int param2)
extern void standard_irq_callback(int);

#define CONCAT_64(hi,lo)    (((UINT64)(hi) << 32) | (UINT32)(lo))
#define EXTRACT_64HI(val)   ((UINT32)((val) >> 32))
#define EXTRACT_64LO(val)   ((UINT32)(val))
inline INT64 mul_32x32(INT32 a, INT32 b)
{
	return (INT64)a * (INT64)b;
}
inline UINT64 mulu_32x32(UINT32 a, UINT32 b)
{
	return (UINT64)a * (UINT64)b;
}

#ifndef NULL
#define NULL 0
#endif

class direct_read_data
{
public:
	UINT16 read_decrypted_word(UINT32 pc);
	UINT16 read_raw_word(UINT32 pc);
};

class rectangle
{
public:
	int min_x, min_y;
	int max_x, max_y;
	bool interlace;
};
extern rectangle tms_rectangle;
class mscreen
{
public:
	int width_v;
	int height_v;
	rectangle visible_area() { return tms_rectangle; }
	void configure(int width, int height, rectangle visarea);
	int vpos();
	int hpos();
	int width() { return width_v; }
	int height() { return height_v; }
};

extern mscreen *m_screen;

class address_space
{
public:
	UINT8 read_byte(UINT32 a);
	UINT16 read_word(UINT32 a);
	void write_word(UINT32 a, UINT16 v);
	void write_byte(UINT32 a, UINT8 v);
};

#define DECLARE_READ16_MEMBER(name)     UINT16 name(address_space &space, offs_t offset, UINT16 mem_mask = 0xffff)
#define DECLARE_WRITE16_MEMBER(name)    void   name(address_space &space, offs_t offset, UINT16 data, UINT16 mem_mask = 0xffff)
#define READ16_MEMBER(name)             UINT16 name(address_space &space, offs_t offset, UINT16 mem_mask)
#define WRITE16_MEMBER(name)            void   name(address_space &space, offs_t offset, UINT16 data, UINT16 mem_mask)

typedef UINT32 device_state_entry;

typedef UINT32 address_spacenum;
typedef UINT32 address_space_config;
typedef UINT32 screen_device;
typedef UINT32 bitmap_ind16;
#define AS_0 0
#define AS_PROGRAM 1

extern void m_to_shiftreg_cb(address_space, offs_t, UINT16*);
extern void m_from_shiftreg_cb(address_space, offs_t, UINT16*);
extern UINT32 total_cycles(void);
