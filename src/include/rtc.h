void rtc_rebase(void);

struct rtc_clock_data
{
	struct tm value, held_value;
	uae_s64 last_us;
	int phase_us, hour12;
	bool initialized, written, hour12_written, carry_pending;
};

struct rtc_msm_data
{
	uae_u8 clock_control_d;
	uae_u8 clock_control_e;
	uae_u8 clock_control_f;
	int delayed_write;
	bool yearmode;
	struct rtc_clock_data clock;
};

#define RF5C01A_RAM_SIZE 16
struct rtc_ricoh_data
{
	uae_u8 clock_control_d;
	uae_u8 clock_control_e;
	uae_u8 clock_control_f;
	uae_u8 rtc_memory[RF5C01A_RAM_SIZE], rtc_alarm[RF5C01A_RAM_SIZE];
	int delayed_write;
	struct rtc_clock_data clock;
};

uae_u8 get_clock_msm(struct rtc_msm_data *data, int addr, struct tm *ct);
bool put_clock_msm(struct rtc_msm_data *data, int addr, uae_u8 v);
// A clock restored from a savestate continues from now.
void rtc_clock_resume(struct rtc_clock_data *clock);
// Restores the calendar digits of an rtcfile written age_us ago; false if
// they were the host's calendar (the clock keeps following the host).
bool rtc_restore_calendar(struct rtc_clock_data *clock, const uae_u8 *digits, bool ricoh, bool mode24, uae_s64 age_us);

uae_u8 get_clock_ricoh(struct rtc_ricoh_data *data, int addr, struct tm *ct);
void put_clock_ricoh(struct rtc_ricoh_data *data, int addr, uae_u8 v);
