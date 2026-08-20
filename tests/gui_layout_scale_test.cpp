#include <cstdint>
#include <cstring>
#include <iostream>

#include "gui_layout_scale.h"

static int failures;

// Exact-value comparison: the computation is specified to be exact where the
// involved factors (1, 2, 0.5) and correctly-rounded division make it exact.
static void expect_eq(const float actual, const float expected, const char* message)
{
	if (actual != expected) {
		std::cerr << message << ": expected " << expected << ", got " << actual << '\n';
		failures++;
	}
}

// Bit-identical comparison: the non-Android default (100%) must be a pure
// pass-through, not merely equal to within an epsilon — desktop layout has to
// stay byte-identical to stock behavior.
static void expect_bits_eq(const float actual, const float expected, const char* message)
{
	std::uint32_t actual_bits;
	std::uint32_t expected_bits;
	std::memcpy(&actual_bits, &actual, sizeof actual_bits);
	std::memcpy(&expected_bits, &expected, sizeof expected_bits);
	if (actual_bits != expected_bits) {
		std::cerr << message << ": expected bits " << std::hex << expected_bits
			<< ", got " << actual_bits << std::dec << '\n';
		failures++;
	}
}

int main()
{
	// Percent 100 is an exact pass-through for every stock scale the DPI
	// layer can produce. 1.7f specifically catches a wrong multiply ordering
	// ((stock * percent) / 100): 1.7f * 100.0f rounds away and dividing back
	// does not recover the original bits.
	const float stock_scales[] = {1.0f, 1.25f, 1.3333333f, 1.5f, 1.7f, 2.0f, 2.4f, 2.5f, 3.75f};
	for (const float stock : stock_scales) {
		expect_bits_eq(effective_layout_scale(stock, 100.0f), stock,
			"100% must pass the stock scale through unchanged");
	}

	// Android large-panel default: a 1600px panel yields stock 2.0
	// (max(1600, x) / 800), and 70% of it is exactly stock * 0.7f.
	expect_eq(effective_layout_scale(2.0f, 70.0f), 2.0f * 0.7f,
		"70% of stock 2.0 must equal stock * 0.7f");
	expect_eq(effective_layout_scale(2.4f, 70.0f), 2.4f * 0.7f,
		"70% of stock 2.4 must equal stock * 0.7f");

	// Percent bounds clamp before the multiply.
	expect_eq(effective_layout_scale(2.0f, 49.0f), 2.0f * 0.5f,
		"49% must clamp to the 50% minimum");
	expect_eq(effective_layout_scale(2.0f, 201.0f), 2.0f * 2.0f,
		"201% must clamp to the 200% maximum");
	expect_eq(effective_layout_scale(1.0f, 0.0f), 0.5f,
		"0% must clamp to the 50% minimum");
	expect_eq(effective_layout_scale(1.0f, 1000.0f), 2.0f,
		"1000% must clamp to the 200% maximum");

	// The bounds are exact factors of two, so the clamped multiply itself
	// stays exact — the limits are preserved through the multiplication.
	expect_eq(effective_layout_scale(1.25f, 50.0f), 0.625f,
		"50% of stock 1.25 must be exactly 0.625");
	expect_eq(effective_layout_scale(1.25f, 200.0f), 2.5f,
		"200% of stock 1.25 must be exactly 2.5");

	// The clamp helper the settings row uses.
	expect_eq(clamp_gui_layout_scale_percent(49.0f), 50.0f, "clamp(49) must be 50");
	expect_eq(clamp_gui_layout_scale_percent(201.0f), 200.0f, "clamp(201) must be 200");
	expect_eq(clamp_gui_layout_scale_percent(-10.0f), 50.0f, "clamp(-10) must be 50");
	expect_eq(clamp_gui_layout_scale_percent(1.0e6f), 200.0f, "clamp(1e6) must be 200");
	expect_eq(clamp_gui_layout_scale_percent(50.0f), 50.0f, "clamp(50) must stay 50");
	expect_eq(clamp_gui_layout_scale_percent(200.0f), 200.0f, "clamp(200) must stay 200");
	expect_eq(clamp_gui_layout_scale_percent(100.0f), 100.0f, "clamp(100) must stay 100");
	expect_eq(clamp_gui_layout_scale_percent(70.0f), 70.0f, "clamp(70) must stay 70");

	// The published bounds back the settings row's spinner limits; the
	// minimum doubles as the lockout guarantee (the smallest allowed scale
	// keeps the settings UI readable enough to raise it again).
	expect_eq(gui_layout_scale_min_percent, 50.0f, "minimum percent must be 50");
	expect_eq(gui_layout_scale_max_percent, 200.0f, "maximum percent must be 200");

	return failures == 0 ? 0 : 1;
}
