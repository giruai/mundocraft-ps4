// Luanti PS4 menu controller adapter.
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

class IrrlichtDevice;
struct SEvent;

namespace ps4_input {

// Called only while a Luanti menu is active. Returns true for consumed events.
bool on_menu_event(const SEvent &event);

// Called once after IrrlichtDevice::run() has pumped SDL events.
void frame(IrrlichtDevice *device);

// Draw after all menu GUI layers, while the frame is still open.
void draw_cursor(IrrlichtDevice *device);

// Release GUI references before the Irrlicht device is destroyed.
void shutdown(IrrlichtDevice *device);

} // namespace ps4_input
