#pragma once

namespace gcn {
	struct Color {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;

		constexpr Color() : r(0), g(0), b(0), a(255) {}
		constexpr Color(int rr, int gg, int bb, int aa = 255)
			: r(static_cast<unsigned char>(rr))
			, g(static_cast<unsigned char>(gg))
			, b(static_cast<unsigned char>(bb))
			, a(static_cast<unsigned char>(aa))
		{
		}
	};
} // namespace gcn
