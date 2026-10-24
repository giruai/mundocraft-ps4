// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <array>
#include <vector>

namespace ps4_input {

// Keep physical state separate from the state already delivered to Irrlicht.
// A complete press/release pair can arrive between two rendered frames.
class PointerButtons {
public:
	enum Button { Left, Right };
	struct Edge { Button button; bool down; };

	void update(Button button, bool down)
	{
		if (physical_[button] == down)
			return;
		physical_[button] = down;
		if (blocked_[button]) {
			if (!down)
				blocked_[button] = false;
			return;
		}
		edges_.push_back({button, down});
	}

	std::vector<Edge> take_edges()
	{
		std::vector<Edge> edges;
		edges.swap(edges_);
		return edges;
	}

	void apply(const Edge &edge) { delivered_[edge.button] = edge.down; }
	bool delivered(Button button) const { return delivered_[button]; }
	bool idle() const
	{
		return !physical_[Left] && !physical_[Right] &&
				!delivered_[Left] && !delivered_[Right] && edges_.empty();
	}

	// A held button belongs to the old screen/overlay until it is released.
	void cancel()
	{
		blocked_ = physical_;
		delivered_.fill(false);
		edges_.clear();
	}

	// Changing the active controller also changes the source of physical state.
	void reset() { *this = PointerButtons{}; }

private:
	std::array<bool, 2> physical_{};
	std::array<bool, 2> blocked_{};
	std::array<bool, 2> delivered_{};
	std::vector<Edge> edges_;
};

} // namespace ps4_input
