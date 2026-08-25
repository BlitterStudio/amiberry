#include <iostream>

#include "crt_gpu_allowlist.h"

namespace {

int failures;

void expect_full(const char* renderer)
{
	if (!crt_gpu_supports_full_crt(renderer)) {
		std::cerr << "expected full-quality CRT path for renderer \"" << renderer << "\"\n";
		failures++;
	}
}

void expect_simplified(const char* renderer)
{
	if (crt_gpu_supports_full_crt(renderer)) {
		std::cerr << "expected simplified CRT path for renderer \"" << renderer << "\"\n";
		failures++;
	}
}

} // namespace

int main()
{
	// Unknown / empty / null stay on the simplified path (fail-safe).
	expect_simplified(nullptr);
	expect_simplified("");
	expect_simplified("unknown gpu");
	expect_simplified("llvmpipe (LLVM 19.1.7, 256 bits)");

	// Capable families.
	expect_full("V3D 7.1.0");              // RPi5
	expect_full("Mali-G52 (Panfrost)");    // Bifrost
	expect_full("Mali-G610 (Panfrost)");   // Valhall
	expect_full("Apple A15 GPU");
	expect_full("Apple M2 GPU");
	expect_full("Adreno (TM) 640");
	expect_full("Adreno (TM) 730");
	expect_full("Adreno (TM) 830");
	expect_full("NVIDIA Tegra X1 (nvgpu)");

	// Low-power / older families stay simplified.
	expect_simplified("V3D 4.2.14-stellar");  // RPi4 VideoCore
	expect_simplified("Mali-450 MP2");        // Utgard
	expect_simplified("Mali-T860");           // Midgard
	expect_full("Mali-G71");                  // Bifrost (G-prefix match)
	expect_simplified("Adreno (TM) 540");
	expect_simplified("Adreno (TM) 308");
	expect_simplified("PowerVR Rogue GE8320");
	expect_simplified("VideoCore IV HW");

	if (failures > 0) {
		std::cerr << failures << " crt_gpu_allowlist failure(s)\n";
		return 1;
	}
	std::cout << "crt_gpu_allowlist: all checks passed\n";
	return 0;
}
