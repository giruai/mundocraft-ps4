// Luanti PS4 menu controller adapter.
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ps4_input.h"
#include "ps4_menu_buttons.h"
#include "ps4_pointer_buttons.h"
#include "ps4_cursor_geometry.h"

#include "gui/mainmenumanager.h"
#include "porting.h"
#include "client/renderingengine.h"
#include "log.h"

#include <IrrlichtDevice.h>
#include <IEventReceiver.h>
#include <ICursorControl.h>
#include <IGUIEnvironment.h>
#include <IGUIElement.h>
#include <IGUIEditBox.h>
#include <IGUIStaticText.h>
#include <IGUIButton.h>
#include <IVideoDriver.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace ps4_input {
namespace {

enum class KeyKind { Character, Shift, Space, Backspace, Done, Cancel };

struct KeyboardKey {
	gui::IGUIButton *button = nullptr;
	KeyKind kind = KeyKind::Character;
	wchar_t character = 0;
};

struct MenuInputState {
	uint32_t active_gamepad = UINT32_MAX;
	int axis_x = 0;
	int axis_y = 0;
	PointerButtons pointer;
	BackButtons back;
	std::vector<bool> circle_edges;
	bool triangle = false;
	bool open_pending = false;
	int pending_wheel = 0;
	bool dpad_up = false;
	bool dpad_down = false;
	bool cursor_known = false;
	float cursor_x = 0;
	float cursor_y = 0;
	u64 last_frame_us = 0;
	gui::IGUIElement *menu = nullptr; // identity only; never dereferenced
	gui::IGUIStaticText *keyboard = nullptr; // grabbed while attached
	gui::IGUIEditBox *target = nullptr; // grabbed until keyboard closes
	std::wstring original_text;
	std::vector<KeyboardKey> keys;
	bool shift = false;
	int key_pressed = -1;
};

MenuInputState s;

bool is_attached(gui::IGUIElement *element, gui::IGUIElement *root)
{
	for (auto *p = element; p; p = p->getParent()) {
		if (p == root)
			return true;
	}
	return false;
}

void notify_edit(gui::IGUIEditBox *edit, gui::EGUI_EVENT_TYPE kind)
{
	// Both the form and its edit box may be removed synchronously by OnEvent.
	// Keep the receiver and Caller alive until dispatch has returned.
	if (auto *parent = edit->getParent()) {
		edit->grab();
		parent->grab();
		SEvent event;
		event.EventType = EET_GUI_EVENT;
		event.GUIEvent.Caller = edit;
		event.GUIEvent.Element = nullptr;
		event.GUIEvent.EventType = kind;
		parent->OnEvent(event);
		parent->drop();
		edit->drop();
	}
}

void close_keyboard(gui::IGUIEnvironment *env, bool commit, bool submit = false)
{
	if (!s.keyboard)
		return;

	// Retain the grabbed objects locally, and finish detaching the overlay and
	// clearing global state before any callback can replace the form or reenter.
	auto *keyboard = s.keyboard;
	auto *target = s.target;
	std::wstring original;
	original.swap(s.original_text);
	s.keyboard = nullptr;
	s.target = nullptr;
	s.keys.clear();
	s.shift = false;
	s.key_pressed = -1;
	s.pointer.cancel();
	if (keyboard->getParent())
		keyboard->remove();
	keyboard->drop();

	if (target) {
		if (env && is_attached(target, env->getRootGUIElement())) {
			if (!commit) {
				target->setText(original.c_str());
				notify_edit(target, gui::EGET_EDITBOX_CHANGED);
			} else if (submit) {
				// The same event used by Luanti's native edit box / mobile
				// input completion. Formspec owns field_enter and close_on_enter.
				notify_edit(target, gui::EGET_EDITBOX_ENTER);
			}
		}
		target->drop();
	}
	std::fill(original.begin(), original.end(), L'\0');
}

void sync_menu(IrrlichtDevice *device)
{
	auto *menu = g_menumgr.tryGetTopMenu();
	if (s.menu == menu)
		return;
	close_keyboard(device->getGUIEnvironment(), false);
	s.menu = g_menumgr.tryGetTopMenu();
	s.pointer.cancel();
	s.back.reset();
	s.circle_edges.clear();
	s.open_pending = false;
	s.pending_wheel = 0;
}

void add_key(gui::IGUIEnvironment *env, const core::rect<s32> &rect,
		const wchar_t *label, KeyKind kind, wchar_t character)
{
	auto *button = env->addButton(rect, s.keyboard, -1, label);
	if (button)
		s.keys.push_back({button, kind, character});
}

void open_keyboard(IrrlichtDevice *device)
{
	if (s.keyboard)
		return;
	auto *env = device->getGUIEnvironment();
	auto *focus = env->getFocus();
	if (!focus || focus->getType() != gui::EGUIET_EDIT_BOX)
		return;

	auto *target = static_cast<gui::IGUIEditBox *>(focus);
	if (!is_attached(target, env->getRootGUIElement()))
		return;
	const auto size = device->getVideoDriver()->getScreenSize();
	const s32 width = std::min<s32>(840, static_cast<s32>(size.Width) - 24);
	const s32 key_height = size.Height < 500 ? 34 : 42;
	const s32 gap = 4;
	const s32 panel_height = 40 + 8 * (key_height + gap) + 8;
	if (width < 250 || static_cast<s32>(size.Height) < panel_height + 8)
		return;
	const s32 x = (static_cast<s32>(size.Width) - width) / 2;
	const s32 y = static_cast<s32>(size.Height) - panel_height - 8;

	s.keyboard = env->addStaticText(L"Keyboard - Circle cancels",
			core::rect<s32>(x, y, x + width, y + panel_height),
			true, false, env->getRootGUIElement(), -1, true);
	if (!s.keyboard)
		return;
	s.keyboard->grab();
	s.target = target;
	s.target->grab();
	s.original_text = target->getText();
	s.shift = false;
	s.key_pressed = -1;
	const wchar_t *rows[] = {
		L"1234567890", L"qwertyuiop", L"asdfghjkl",
		L"zxcvbnm", L".:/-_@!?#%+", L"\"'$^&*()=;,<>",
		L"[]{}|~\\`"
	};
	const s32 inner_width = width - 24;
	const s32 columns = 13;
	const s32 key_width = (inner_width - gap * (columns - 1)) / columns;
	for (s32 row = 0; row < 7; ++row) {
		const s32 top = 36 + row * (key_height + gap);
		for (s32 col = 0; rows[row][col]; ++col) {
			const s32 left = 12 + col * (key_width + gap);
			wchar_t label[] = {rows[row][col], 0};
			add_key(env, core::rect<s32>(left, top, left + key_width,
					top + key_height), label, KeyKind::Character, rows[row][col]);
		}
	}
	const s32 special_top = 36 + 7 * (key_height + gap);
	const s32 special_width = (inner_width - gap * 4) / 5;
	const struct { const wchar_t *label; KeyKind kind; } special[] = {
		{L"Shift", KeyKind::Shift}, {L"Space", KeyKind::Space},
		{L"Back", KeyKind::Backspace}, {L"Done", KeyKind::Done},
		{L"Cancel", KeyKind::Cancel}
	};
	for (s32 i = 0; i < 5; ++i) {
		const s32 left = 12 + i * (special_width + gap);
		add_key(env, core::rect<s32>(left, special_top, left + special_width,
				special_top + key_height), special[i].label, special[i].kind, 0);
	}
	env->getRootGUIElement()->bringToFront(s.keyboard);
}

int key_at(s32 x, s32 y)
{
	const core::position2di point(x, y);
	for (size_t i = 0; i < s.keys.size(); ++i) {
		if (s.keys[i].button->getAbsolutePosition().isPointInside(point))
			return static_cast<int>(i);
	}
	return -1;
}

void activate_key(gui::IGUIEnvironment *env, int index)
{
	if (index < 0 || index >= static_cast<int>(s.keys.size()) || !s.target)
		return;
	const auto key = s.keys[index];
	if (key.kind == KeyKind::Done || key.kind == KeyKind::Cancel) {
		close_keyboard(env, key.kind == KeyKind::Done, key.kind == KeyKind::Done);
		return;
	}
	if (key.kind == KeyKind::Shift) {
		s.shift = !s.shift;
		for (auto &item : s.keys) {
			if (item.kind != KeyKind::Character)
				continue;
			wchar_t value = item.character;
			if (s.shift && value >= L'a' && value <= L'z')
				value -= L'a' - L'A';
			wchar_t label[] = {value, 0};
			item.button->setText(label);
		}
		return;
	}
	if (!is_attached(s.target, env->getRootGUIElement())) {
		close_keyboard(env, true);
		return;
	}
	std::wstring text = s.target->getText();
	if (key.kind == KeyKind::Backspace) {
		if (text.empty())
			return;
		text.pop_back();
	} else {
		const auto max_length = s.target->getMax();
		if (max_length && text.size() >= max_length)
			return;
		wchar_t value = key.kind == KeyKind::Space ? L' ' : key.character;
		if (s.shift && value >= L'a' && value <= L'z')
			value -= L'a' - L'A';
		text.push_back(value);
	}
	s.target->setText(text.c_str());
	notify_edit(s.target, gui::EGET_EDITBOX_CHANGED);
	std::fill(text.begin(), text.end(), L'\0');
}

void ensure_cursor(IrrlichtDevice *device)
{
	auto *control = device->getCursorControl();
	const auto size = device->getVideoDriver()->getScreenSize();
	if (!s.cursor_known) {
		const auto pos = control->getPosition(false);
		s.cursor_x = pos.X >= 0 && pos.X < static_cast<s32>(size.Width) ? pos.X : size.Width / 2;
		s.cursor_y = pos.Y >= 0 && pos.Y < static_cast<s32>(size.Height) ? pos.Y : size.Height / 2;
		s.cursor_known = true;
	}
	s.cursor_x = std::clamp(s.cursor_x, 0.0f, static_cast<float>(size.Width ? size.Width - 1 : 0));
	s.cursor_y = std::clamp(s.cursor_y, 0.0f, static_cast<float>(size.Height ? size.Height - 1 : 0));
	control->setPosition(static_cast<s32>(s.cursor_x), static_cast<s32>(s.cursor_y));
	control->setVisible(true);
}

void post_mouse(IrrlichtDevice *device, EMOUSE_INPUT_EVENT type, float wheel = 0)
{
	SEvent event;
	event.EventType = EET_MOUSE_INPUT_EVENT;
	event.MouseInput.X = static_cast<s32>(s.cursor_x);
	event.MouseInput.Y = static_cast<s32>(s.cursor_y);
	event.MouseInput.ButtonStates =
			(s.pointer.delivered(PointerButtons::Left) ? SDL_BUTTON_MASK(SDL_BUTTON_LEFT) : 0) |
			(s.pointer.delivered(PointerButtons::Right) ? SDL_BUTTON_MASK(SDL_BUTTON_RIGHT) : 0);
	event.MouseInput.Shift = false;
	event.MouseInput.Control = false;
	event.MouseInput.Simulated = true;
	event.MouseInput.Event = type;
	if (type == EMIE_MOUSE_WHEEL)
		event.MouseInput.Wheel = wheel;
	else
		event.MouseInput.Button = (type == EMIE_RMOUSE_PRESSED_DOWN ||
				type == EMIE_RMOUSE_LEFT_UP) ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
	device->postEventFromUser(event);
}

void post_escape(IrrlichtDevice *device, bool down)
{
	SEvent event;
	event.EventType = EET_KEY_INPUT_EVENT;
	event.KeyInput.Char = 0;
	event.KeyInput.Key = KEY_ESCAPE;
	event.KeyInput.SystemKeyCode = 0;
	event.KeyInput.PressedDown = down;
	event.KeyInput.Shift = false;
	event.KeyInput.Control = false;
	device->postEventFromUser(event);
}

void clear_menu_state(IrrlichtDevice *device)
{
	auto *env = device ? device->getGUIEnvironment() : nullptr;
	close_keyboard(env, true);
	s.menu = nullptr;
	s.active_gamepad = UINT32_MAX;
	s.axis_x = s.axis_y = 0;
	// Gameplay events do not pass through this adapter. Forget physical holds
	// here, since their releases may happen before the next menu is opened.
	s.pointer.reset();
	s.back.reset();
	s.circle_edges.clear();
	s.triangle = false;
	s.dpad_up = s.dpad_down = false;
	s.open_pending = false;
	s.pending_wheel = 0;
	s.cursor_known = false;
	s.last_frame_us = 0;
}

} // namespace

bool on_menu_event(const SEvent &event)
{
	if (!isMenuActive())
		return false;
	if (auto *device = RenderingEngine::get_raw_device())
		sync_menu(device);
	if (!isMenuActive())
		return true;
	if (event.EventType == EET_GAMEPAD_AXIS_EVENT) {
		const auto &axis = event.GamepadAxisEvent;
		if (s.active_gamepad != axis.ID) {
			clear_menu_state(RenderingEngine::get_raw_device());
			s.pointer.reset();
			s.active_gamepad = axis.ID;
			s.menu = g_menumgr.tryGetTopMenu();
		}
		if (axis.Axis == GamepadAxis::LEFTX)
			s.axis_x = axis.Value;
		else if (axis.Axis == GamepadAxis::LEFTY)
			s.axis_y = axis.Value;
		return true;
	}
	if (event.EventType == EET_GAMEPAD_BUTTON_EVENT) {
		const auto &button = event.GamepadButtonEvent;
		if (s.active_gamepad != button.ID) {
			clear_menu_state(RenderingEngine::get_raw_device());
			s.pointer.reset();
			s.active_gamepad = button.ID;
			s.menu = g_menumgr.tryGetTopMenu();
		}
		switch (button.Button) {
		case GamepadButton::SOUTH:
			s.pointer.update(PointerButtons::Left, button.PressedDown);
			break;
		case GamepadButton::WEST:
			s.pointer.update(PointerButtons::Right, button.PressedDown);
			break;
		case GamepadButton::EAST:
		case GamepadButton::START: {
			const int edge = s.back.update(button.Button == GamepadButton::START, button.PressedDown);
			if (edge >= 0) {
				s.circle_edges.push_back(edge != 0);
				static unsigned logs = 0;
				if (logs++ < 16)
					infostream << "PS4_INPUT menu_back source="
							<< (button.Button == GamepadButton::START ? "Options" : "Circle")
							<< " down=" << edge << std::endl;
			}
			break;
		}
		case GamepadButton::NORTH:
			if (button.PressedDown && !s.triangle)
				s.open_pending = true;
			s.triangle = button.PressedDown;
			break;
		case GamepadButton::DPAD_UP:
			if (button.PressedDown && !s.dpad_up)
				++s.pending_wheel;
			s.dpad_up = button.PressedDown;
			break;
		case GamepadButton::DPAD_DOWN:
			if (button.PressedDown && !s.dpad_down)
				--s.pending_wheel;
			s.dpad_down = button.PressedDown;
			break;
		default: break;
		}
		return true;
	}
	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		s.cursor_x = event.MouseInput.X;
		s.cursor_y = event.MouseInput.Y;
		s.cursor_known = true;
		if (s.keyboard) {
			if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
				s.key_pressed = key_at(event.MouseInput.X, event.MouseInput.Y);
			} else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) {
				int released = key_at(event.MouseInput.X, event.MouseInput.Y);
				if (released == s.key_pressed)
					activate_key(guienv, released);
				s.key_pressed = -1;
			}
			return true;
		}
	}
	return false;
}

void frame(IrrlichtDevice *device)
{
	if (!device)
		return;
	if (!isMenuActive()) {
		if (s.menu || s.keyboard)
			clear_menu_state(device);
		return;
	}
	auto *env = device->getGUIEnvironment();
	sync_menu(device);
	if (!isMenuActive()) {
		clear_menu_state(device);
		return;
	}
	auto *current_menu = g_menumgr.tryGetTopMenu();
	// Keep the identity alive throughout dispatch. A replacement must not reuse
	// its address and receive the remaining edges from this frame.
	if (current_menu)
		current_menu->grab();
	if (s.keyboard && (!s.target ||
			!is_attached(s.target, env->getRootGUIElement()) ||
			!s.keyboard->getParent()))
		close_keyboard(env, true);

	const auto size = device->getVideoDriver()->getScreenSize();
	const u64 now = porting::getTimeUs();
	const float dt = s.last_frame_us && now > s.last_frame_us
			? std::min<float>((now - s.last_frame_us) / 1000000.0f, 0.05f) : 0.0f;
	s.last_frame_us = now;
	ensure_cursor(device);
	auto axis_fraction = [](int value) {
		const float deadzone = 6000.0f;
		const float magnitude = std::abs(static_cast<float>(value));
		if (magnitude <= deadzone)
			return 0.0f;
		const float scaled = (magnitude - deadzone) / (32767.0f - deadzone);
		return std::copysign(std::min(scaled, 1.0f), static_cast<float>(value));
	};
	const float dx = axis_fraction(s.axis_x) * 900.0f * dt;
	const float dy = axis_fraction(s.axis_y) * 900.0f * dt;
	if (dx || dy) {
		s.cursor_x = std::clamp(s.cursor_x + dx, 0.0f,
				static_cast<float>(size.Width - 1));
		s.cursor_y = std::clamp(s.cursor_y + dy, 0.0f,
				static_cast<float>(size.Height - 1));
		device->getCursorControl()->setPosition(static_cast<s32>(s.cursor_x),
				static_cast<s32>(s.cursor_y));
		if (!s.keyboard)
			post_mouse(device, EMIE_MOUSE_MOVED);
	}
	// Use snapshots: callbacks may close the keyboard and clear queued state.
	const auto pointer_edges = s.pointer.take_edges();
	std::vector<bool> back_edges;
	back_edges.swap(s.circle_edges);
	if (s.keyboard) {
		for (const auto &edge : pointer_edges) {
			if (!s.keyboard || !isMenuActive() ||
					g_menumgr.tryGetTopMenu() != current_menu)
				break;
			if (edge.button != PointerButtons::Left)
				continue;
			const int hovered = key_at(static_cast<s32>(s.cursor_x),
					static_cast<s32>(s.cursor_y));
			if (edge.down)
				s.key_pressed = hovered;
			else {
				const int pressed = s.key_pressed;
				s.key_pressed = -1;
				if (pressed == hovered)
					activate_key(env, hovered);
			}
		}
		if (s.keyboard) {
			for (bool down : back_edges) {
				if (down) {
					close_keyboard(env, false);
					break;
				}
			}
		}
		s.pending_wheel = 0;
		s.open_pending = false;
	} else {
		// A click/escape can destroy or replace the menu synchronously. Do not
		// deliver the rest of this frame's queued input to the next screen.
		for (const auto &edge : pointer_edges) {
			if (!isMenuActive() || g_menumgr.tryGetTopMenu() != current_menu)
				break;
			s.pointer.apply(edge);
			const auto type = edge.button == PointerButtons::Left
					? (edge.down ? EMIE_LMOUSE_PRESSED_DOWN : EMIE_LMOUSE_LEFT_UP)
					: (edge.down ? EMIE_RMOUSE_PRESSED_DOWN : EMIE_RMOUSE_LEFT_UP);
			post_mouse(device, type);
		}
		for (bool down : back_edges) {
			if (!isMenuActive() || g_menumgr.tryGetTopMenu() != current_menu)
				break;
			post_escape(device, down);
		}
		if (s.pending_wheel && isMenuActive() && g_menumgr.tryGetTopMenu() == current_menu) {
			post_mouse(device, EMIE_MOUSE_WHEEL, static_cast<float>(s.pending_wheel));
			s.pending_wheel = 0;
		}
		// Finish any native mouse gesture before switching to the overlay.
		// Triangle remains pending while a button is held, so opening cannot
		// strand a pressed Irrlicht button or turn its release into an OSK key.
		if (s.open_pending && s.pointer.idle() && isMenuActive() &&
				g_menumgr.tryGetTopMenu() == current_menu) {
			s.open_pending = false;
			open_keyboard(device);
		}
	}
	if (!isMenuActive())
		clear_menu_state(device);
	else
		sync_menu(device);
	if (current_menu)
		current_menu->drop();
}

void draw_cursor(IrrlichtDevice *device)
{
	if (!device || !isMenuActive() || !s.cursor_known)
		return;
	auto *driver = device->getVideoDriver();
	const auto size = driver->getScreenSize();
	for (const auto &rect : cursor_geometry(size.Width, size.Height,
			static_cast<int>(s.cursor_x), static_cast<int>(s.cursor_y))) {
		if (rect.right > rect.left && rect.bottom > rect.top)
			driver->draw2DRectangle(video::SColor(rect.color),
					core::rect<s32>(rect.left, rect.top, rect.right, rect.bottom));
	}
}

void shutdown(IrrlichtDevice *device)
{
	clear_menu_state(device);
}

} // namespace ps4_input
