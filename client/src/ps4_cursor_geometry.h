// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace ps4_input {
struct CursorRect {
	int left, top, right, bottom;
	uint32_t color;
};

// The center is the click coordinate. Opaque black outline, white arms and a
// yellow center remain legible on bright menus and dark worlds without fonts.
inline std::array<CursorRect, 5> cursor_geometry(int width, int height, int x, int y)
{
	std::array<CursorRect, 5> result{};
	if (width <= 0 || height <= 0)
		return result;
	x = std::clamp(x, 0, width - 1);
	y = std::clamp(y, 0, height - 1);
	const int radius = std::clamp(height / 45, 8, 32);
	const int thick = std::max(1, radius / 8);
	auto rect = [&](int rx, int ry, uint32_t color) {
		// Widen before arithmetic so even extreme window dimensions are safe.
		return CursorRect{
			int(std::max<int64_t>(0, int64_t(x) - rx)),
			int(std::max<int64_t>(0, int64_t(y) - ry)),
			int(std::min<int64_t>(width, int64_t(x) + rx + 1)),
			int(std::min<int64_t>(height, int64_t(y) + ry + 1)), color};
	};
	result[0] = rect(radius, thick + 2, 0xff000000);
	result[1] = rect(thick + 2, radius, 0xff000000);
	result[2] = rect(radius - 2, thick, 0xffffffff);
	result[3] = rect(thick, radius - 2, 0xffffffff);
	result[4] = rect(thick, thick, 0xffffd740);
	return result;
}
} // namespace ps4_input
