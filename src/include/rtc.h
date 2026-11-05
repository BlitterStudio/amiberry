struct rtc_counter
{
	// Emulated RTC time: the chip's own counters, seeded from the host
	// clock (+ cs_rtc_adjust) at the first access, advanced by emulated
	// time and set by guest writes.
	bool init;
	int sec, min, hour, mday, mon, year, wday;  // hour 0-23, mon 1-12
	int leap;                // RP5C01 leap year counter (0 = leap year)
	uae_s64 last;            // get_cycles() of the last update
	uae_s64 phase;           // cycles since the last second carry
	bool pending;            // MSM HOLD: one carry held back
	bool control_written;    // MSM: the guest wrote CD since reset (HOLD honoured)
	bool h1_pending;         // 12-hour mode: H1 written, waiting for H10
	int h1_digit;
};

struct rtc_msm_data
{
	uae_u8 clock_control_d;
	uae_u8 clock_control_e;
	uae_u8 clock_control_f;
	int delayed_write;
	bool yearmode;
	struct rtc_counter cnt;
};

#define RF5C01A_RAM_SIZE 16
struct rtc_ricoh_data
{
	uae_u8 clock_control_d;
	uae_u8 clock_control_e;
	uae_u8 clock_control_f;
	uae_u8 rtc_memory[RF5C01A_RAM_SIZE], rtc_alarm[RF5C01A_RAM_SIZE];
	int delayed_write;
	struct rtc_counter cnt;
};

uae_u8 get_clock_msm(struct rtc_msm_data *data, int addr, struct tm *ct);
bool put_clock_msm(struct rtc_msm_data *data, int addr, uae_u8 v);

uae_u8 get_clock_ricoh(struct rtc_ricoh_data *data, int addr, struct tm *ct);
void put_clock_ricoh(struct rtc_ricoh_data *data, int addr, uae_u8 v);

// Restart the emulated counters from the host clock at the next access
void rtc_counter_reset(struct rtc_counter *c);
