
struct rtc_msm_data
{
	uae_u8 clock_control_d;
	uae_u8 clock_control_e;
	uae_u8 clock_control_f;
	bool yearmode;
};

#define RF5C01A_RAM_SIZE 16
struct rtc_ricoh_data
{
	uae_u8 clock_control_d;
	uae_u8 clock_control_e;
	uae_u8 clock_control_f;
	uae_u8 rtc_memory[RF5C01A_RAM_SIZE], rtc_alarm[RF5C01A_RAM_SIZE];
};

uae_u8 get_clock_msm(struct rtc_msm_data *data, int addr, struct tm *ct);
bool put_clock_msm(struct rtc_msm_data *data, int addr, uae_u8 v);

uae_u8 get_clock_ricoh(struct rtc_ricoh_data *data, int addr, struct tm *ct);
void put_clock_ricoh(struct rtc_ricoh_data *data, int addr, uae_u8 v);
