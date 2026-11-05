#include <cstdlib>
#include <iostream>

#include "amiberry_frame_pacing.h"

static void expect(const bool condition, const char* message)
{
	if (!condition) {
		std::cerr << message << '\n';
		std::exit(1);
	}
}

static void test_sleep_boundaries()
{
	const amiberry_frame_pacer pacer;
	expect(pacer.sleep_us(-20000) == 0, "late frame must not sleep");
	expect(pacer.sleep_us(0) == 0, "deadline must not sleep");
	expect(pacer.sleep_us(200) == 0, "reserve the final spin window");
	expect(pacer.sleep_us(249) == 0, "avoid tiny sleep requests");
	expect(pacer.sleep_us(250) == 50, "sleep down to the guard");
	expect(pacer.sleep_us(1200) == 1000, "sleep at most one millisecond");
	expect(pacer.sleep_us(20000) == 1000, "service audio between bounded sleeps");
}

static void test_scheduler_overshoot()
{
	amiberry_frame_pacer pacer;
	pacer.observe_sleep(1000, 1600);
	expect(pacer.sleep_us(749) == 0, "reserve scheduler overshoot plus slack");
	expect(pacer.sleep_us(750) == 50, "do not reserve more than needed");
	for (int i = 0; i < 500; ++i)
		pacer.observe_sleep(1000, 1000);
	expect(pacer.sleep_us(250) == 50, "recover from isolated scheduler spikes");
	for (int i = 0; i < 1000; ++i)
		pacer.observe_sleep(1000, 1000);
	expect(pacer.sleep_us(249) == 0, "retain minimum guard after recovery");
}

static void test_long_descheduling_is_bounded()
{
	amiberry_frame_pacer pacer;
	pacer.observe_sleep(1000, 5000000);
	expect(pacer.sleep_us(2049) == 0, "cap guard at the old two-millisecond window");
	expect(pacer.sleep_us(2050) == 50, "a long descheduling must not disable sleep");
	pacer.observe_sleep(1000, 0);
	expect(pacer.sleep_us(2049) == 50, "early wakeup must allow guard recovery");
}

static void test_frame_deadlines()
{
	for (const int frame_us : {16683, 20032}) {
		for (const int overshoot_us : {0, 50, 300, 900, 1800}) {
			amiberry_frame_pacer pacer;
			for (int frame = 0; frame < 100; ++frame) {
				// A stable scheduler is learned before measuring steady-state frames.
				pacer.observe_sleep(1000, 1000 + overshoot_us);
				int remaining = frame_us;
				int sleeps = 0;
				for (;;) {
					const int request = pacer.sleep_us(remaining);
					if (!request)
						break;
					remaining -= request + overshoot_us;
					pacer.observe_sleep(request, request + overshoot_us);
					expect(++sleeps <= 25, "frame wait must make bounded progress");
				}
				expect(remaining >= 0, "calibrated sleeps must not cross deadline");
				expect(remaining < 2050, "do not expand the original spin window");
			}
		}
	}
}

int main()
{
	test_sleep_boundaries();
	test_scheduler_overshoot();
	test_long_descheduling_is_bounded();
	test_frame_deadlines();
	std::cout << "Frame pacing tests passed\n";
}
