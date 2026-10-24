// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

namespace ps4_input {

// Circle and Options share Escape in menus. Keep Escape held until both are
// released; ignore repeated presses from the same button.
class BackButtons {
public:
	int update(bool options, bool down)
	{
		const bool previous = circle_ || options_;
		(options ? options_ : circle_) = down;
		const bool current = circle_ || options_;
		return previous == current ? -1 : int(current);
	}
	void reset() { circle_ = options_ = false; }
private:
	bool circle_ = false;
	bool options_ = false;
};

} // namespace ps4_input
