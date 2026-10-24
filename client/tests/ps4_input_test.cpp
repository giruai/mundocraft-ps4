// SPDX-License-Identifier: LGPL-2.1-or-later
// Compile the production adapter against a small intrusive-reference GUI model.
// This exercises frame dispatch and callbacks, rather than copying its logic.
#include "../src/ps4_input.cpp"
#include <cstdlib>

#define CHECK(condition) do { if (!(condition)) { \
 std::cerr << "FAIL line " << __LINE__ << ": " << #condition << '\n'; std::abort(); \
} } while (false)

namespace {
using namespace ps4_input;
struct Fixture {
 IrrlichtDevice device;
 gui::IGUIElement *menu;
 gui::IGUIEditBox *edit;
 Fixture() {
  CHECK(gui::IGUIElement::living == 1); // device root
  s = MenuInputState{};
  RenderingEngine::device=&device;
  guienv=&device.env;
  menu=new gui::IGUIElement(device.env.getRootGUIElement());menu->drop();
  edit=new gui::IGUIEditBox(menu);edit->drop();edit->setText(L"stone");
  device.env.focus=edit;
  g_menumgr.top=menu;
  device.callback=[](const SEvent &event){on_menu_event(event);};
 }
 ~Fixture() {
  shutdown(&device);
  s=MenuInputState{};
  g_menumgr.top=nullptr;
  RenderingEngine::device=nullptr;
  guienv=nullptr;
 }
 void button(GamepadButton button,bool down) {
  SEvent e;e.EventType=EET_GAMEPAD_BUTTON_EVENT;
  e.GamepadButtonEvent={0,button,down};CHECK(on_menu_event(e));
 }
 void frame(){ps4_input::frame(&device);}
 void keyboard(){button(GamepadButton::NORTH,true);frame();button(GamepadButton::NORTH,false);CHECK(s.keyboard);}
 void point_key(KeyKind kind) {
  for(auto &key:s.keys) if(key.kind==kind){auto r=key.button->getAbsolutePosition();s.cursor_x=(r.left+r.right)/2;s.cursor_y=(r.top+r.bottom)/2;return;}
  CHECK(false);
 }
 void click_key(KeyKind kind) {
  point_key(kind);button(GamepadButton::SOUTH,true);button(GamepadButton::SOUTH,false);frame();
 }
};

void pointer_order_and_masks() {
 Fixture f;
 // All four edges arrive before one frame, including repeated SDL downs/ups.
 f.button(GamepadButton::SOUTH,true);
 f.button(GamepadButton::SOUTH,true);
 f.button(GamepadButton::WEST,true);
 f.button(GamepadButton::SOUTH,false);
 f.button(GamepadButton::SOUTH,false);
 f.button(GamepadButton::WEST,false);
 f.frame();
 const auto &v=f.device.events;CHECK(v.size()==4);
 const EMOUSE_INPUT_EVENT types[]={EMIE_LMOUSE_PRESSED_DOWN,EMIE_RMOUSE_PRESSED_DOWN,EMIE_LMOUSE_LEFT_UP,EMIE_RMOUSE_LEFT_UP};
 const unsigned masks[]={1,5,4,0},buttons[]={1,3,1,3};
 for(size_t i=0;i<4;++i) {
  CHECK(v[i].EventType==EET_MOUSE_INPUT_EVENT);
  CHECK(v[i].MouseInput.Event==types[i]);
  CHECK(v[i].MouseInput.Button==buttons[i]);
  CHECK(v[i].MouseInput.ButtonStates==masks[i]);
  CHECK(v[i].MouseInput.Simulated);
 }
 // Right-first ordering, held-state motion and wheel preserve both buttons.
 f.device.events.clear();
 f.button(GamepadButton::WEST,true);f.button(GamepadButton::SOUTH,true);f.frame();
 CHECK(f.device.events[0].MouseInput.ButtonStates==4);
 CHECK(f.device.events[1].MouseInput.ButtonStates==5);
 f.device.events.clear();
 SEvent axis;axis.EventType=EET_GAMEPAD_AXIS_EVENT;axis.GamepadAxisEvent={0,GamepadAxis::LEFTX,32767};on_menu_event(axis);
 f.button(GamepadButton::DPAD_DOWN,true);f.frame();
 CHECK(f.device.events.size()==2);
 CHECK(f.device.events[0].MouseInput.Event==EMIE_MOUSE_MOVED);
 CHECK(f.device.events[0].MouseInput.ButtonStates==5);
 CHECK(f.device.events[1].MouseInput.Event==EMIE_MOUSE_WHEEL);
 CHECK(f.device.events[1].MouseInput.ButtonStates==5);
 f.button(GamepadButton::WEST,false);f.frame();
 CHECK(f.device.events.back().MouseInput.ButtonStates==1);
 f.button(GamepadButton::SOUTH,false);f.frame();
 CHECK(f.device.events.back().MouseInput.ButtonStates==0);
}

void menu_replacement_quarantines_held_input() {
 Fixture f;
 auto *replacement=new gui::IGUIElement(f.device.env.getRootGUIElement());replacement->drop();
 f.device.callback=[&](const SEvent &event) {
  on_menu_event(event);
  if(event.EventType==EET_MOUSE_INPUT_EVENT && event.MouseInput.Event==EMIE_LMOUSE_PRESSED_DOWN)
   g_menumgr.top=replacement;
 };
 f.button(GamepadButton::SOUTH,true);f.button(GamepadButton::WEST,true);f.frame();
 CHECK(f.device.events.size()==1); // no Square delivered to replacement
 CHECK(!s.pointer.delivered(PointerButtons::Left));
 CHECK(!s.pointer.delivered(PointerButtons::Right));
 f.button(GamepadButton::SOUTH,true);f.button(GamepadButton::WEST,true);f.frame();
 CHECK(f.device.events.size()==1); // repeated held downs also quarantined
 f.button(GamepadButton::SOUTH,false);f.button(GamepadButton::WEST,false);f.frame();
 CHECK(f.device.events.size()==1);
 f.button(GamepadButton::WEST,true);f.button(GamepadButton::WEST,false);f.frame();
 CHECK(f.device.events.size()==3);
 CHECK(f.device.events[1].MouseInput.ButtonStates==4);
 CHECK(f.device.events[2].MouseInput.ButtonStates==0);
}

void gameplay_exit_does_not_leave_stale_physical_holds() {
 Fixture f;
 f.button(GamepadButton::WEST,true);f.frame();
 CHECK(f.device.events.size()==1);
 g_menumgr.top=nullptr;f.frame();
 // Release occurs during gameplay, where the adapter is not called.
 g_menumgr.top=f.menu;
 f.button(GamepadButton::WEST,true);f.button(GamepadButton::WEST,false);f.frame();
 CHECK(f.device.events.size()==3);
 CHECK(f.device.events[1].MouseInput.ButtonStates==4);
 CHECK(f.device.events[2].MouseInput.ButtonStates==0);
}

void active_controller_change_clears_old_masks() {
 Fixture f;
 f.button(GamepadButton::WEST,true);f.frame();
 SEvent e;e.EventType=EET_GAMEPAD_BUTTON_EVENT;
 e.GamepadButtonEvent={1,GamepadButton::SOUTH,true};CHECK(on_menu_event(e));f.frame();
 CHECK(f.device.events.size()==2);
 CHECK(f.device.events.back().MouseInput.ButtonStates==1);
 e.GamepadButtonEvent={1,GamepadButton::SOUTH,false};CHECK(on_menu_event(e));f.frame();
 CHECK(f.device.events.back().MouseInput.ButtonStates==0);
}

void keyboard_waits_for_native_drag_release() {
 Fixture f;
 f.button(GamepadButton::SOUTH,true);f.frame();
 f.button(GamepadButton::NORTH,true);f.frame();CHECK(!s.keyboard);
 f.button(GamepadButton::SOUTH,false);f.frame();CHECK(s.keyboard);
 CHECK(f.device.events.size()==2);
 CHECK(f.device.events.back().MouseInput.Event==EMIE_LMOUSE_LEFT_UP);
 f.button(GamepadButton::WEST,true);f.button(GamepadButton::WEST,false);f.frame();
 CHECK(f.device.events.size()==2); // Square is consumed inside OSK
 f.click_key(KeyKind::Cancel);CHECK(!s.keyboard);
 CHECK(f.device.events.size()==2);
}

void done_submits_after_detach_and_survives_removal() {
 Fixture f;f.keyboard();
 activate_key(&f.device.env,10); // first letter in second row: q
 CHECK(std::wstring(f.edit->getText())==L"stoneq");
 int enters=0,changes=0;
 f.menu->callback=[&](const SEvent &event) {
  if(event.GUIEvent.EventType==gui::EGET_EDITBOX_CHANGED){++changes;return true;}
  ++enters;
  CHECK(event.GUIEvent.EventType==gui::EGET_EDITBOX_ENTER);
  CHECK(event.GUIEvent.Caller==f.edit);
  CHECK(event.GUIEvent.Element==nullptr);
  CHECK(std::wstring(f.edit->getText())==L"stoneq");
  CHECK(!s.keyboard && !s.target && s.keys.empty());
  CHECK(event.GUIEvent.Caller->getParent()==f.menu);
  f.edit->remove();
  f.menu->remove();
  // Their callbacks and event Caller must stay alive until OnEvent returns.
  CHECK(gui::IGUIElement::living==3);
  g_menumgr.top=nullptr;
  return true;
 };
 f.point_key(KeyKind::Done);
 f.button(GamepadButton::SOUTH,true);f.button(GamepadButton::SOUTH,false);
 f.button(GamepadButton::WEST,true);f.button(GamepadButton::WEST,false);
 f.button(GamepadButton::SOUTH,true);f.button(GamepadButton::SOUTH,false);
 f.button(GamepadButton::EAST,true);f.button(GamepadButton::EAST,false);
 f.frame();
 CHECK(enters==1 && changes==0);
 CHECK(!s.keyboard && !s.target);
 CHECK(f.device.events.empty());
 CHECK(gui::IGUIElement::living==1);
}

void cancel_restores_without_submission() {
 Fixture f;f.keyboard();
 int changes=0,enters=0;
 f.menu->callback=[&](const SEvent &event) {
  if(event.GUIEvent.EventType==gui::EGET_EDITBOX_CHANGED)++changes;else ++enters;
  return true;
 };
 f.edit->setText(L"changed");f.click_key(KeyKind::Cancel);
 CHECK(std::wstring(f.edit->getText())==L"stone");
 CHECK(changes==1 && enters==0 && !s.keyboard);
 f.keyboard();f.edit->setText(L"changed again");
 f.button(GamepadButton::EAST,true);f.button(GamepadButton::EAST,false);f.frame();
 CHECK(std::wstring(f.edit->getText())==L"stone");
 CHECK(changes==2 && enters==0 && f.device.events.empty());
}

void detached_targets_and_shutdown_never_submit() {
 {Fixture f;f.keyboard();
  int events=0;f.menu->callback=[&](const SEvent &){++events;return true;};
  f.edit->remove();f.click_key(KeyKind::Done);
  CHECK(events==0 && !s.keyboard && !s.target);
 }
 {Fixture f;f.keyboard();
  int events=0;f.menu->callback=[&](const SEvent &){++events;return true;};
  shutdown(&f.device);CHECK(events==0 && !s.keyboard && !s.target);
 }
}

void cancel_callback_and_menu_sync_can_remove_form() {
 {Fixture f;f.keyboard();
  f.edit->setText(L"changed");int changes=0;
  f.menu->callback=[&](const SEvent &event) {
   CHECK(event.GUIEvent.EventType==gui::EGET_EDITBOX_CHANGED);
   CHECK(std::wstring(f.edit->getText())==L"stone");
   CHECK(!s.keyboard && !s.target);
   ++changes;f.edit->remove();f.menu->remove();g_menumgr.top=nullptr;
   CHECK(gui::IGUIElement::living==3);return true;
  };
  f.click_key(KeyKind::Cancel);
  CHECK(changes==1 && gui::IGUIElement::living==1);
 }
 {Fixture f;f.keyboard();
  auto *replacement=new gui::IGUIElement(f.device.env.getRootGUIElement());replacement->drop();
  f.menu->callback=[&](const SEvent &event) {
   CHECK(event.GUIEvent.EventType==gui::EGET_EDITBOX_CHANGED);
   replacement->remove();g_menumgr.top=nullptr;return true;
  };
  g_menumgr.top=replacement;
  f.frame();
  CHECK(!s.keyboard && !s.target && s.menu==nullptr);
  CHECK(f.device.events.empty());
 }
}

void done_same_menu_has_no_click_leak_and_blocks_square_release() {
 Fixture f;f.keyboard();int enters=0;
 f.menu->callback=[&](const SEvent &event){CHECK(event.GUIEvent.EventType==gui::EGET_EDITBOX_ENTER);++enters;return true;};
 f.point_key(KeyKind::Done);
 f.button(GamepadButton::WEST,true);
 f.button(GamepadButton::SOUTH,true);f.button(GamepadButton::SOUTH,false);
 f.button(GamepadButton::SOUTH,true);f.button(GamepadButton::SOUTH,false);
 f.button(GamepadButton::START,true);f.button(GamepadButton::START,false);
 f.frame();CHECK(enters==1 && !s.keyboard && f.device.events.empty());
 f.button(GamepadButton::WEST,false);f.frame();CHECK(f.device.events.empty());
 f.button(GamepadButton::WEST,true);f.button(GamepadButton::WEST,false);f.frame();
 CHECK(f.device.events.size()==2);
 CHECK(f.device.events[0].MouseInput.ButtonStates==4);
 CHECK(f.device.events[1].MouseInput.ButtonStates==0);
}

void direct_mouse_done_is_consumed() {
 Fixture f;f.keyboard();int enters=0;
 f.menu->callback=[&](const SEvent &){++enters;return true;};f.point_key(KeyKind::Done);
 SEvent mouse;mouse.EventType=EET_MOUSE_INPUT_EVENT;
 mouse.MouseInput.X=s.cursor_x;mouse.MouseInput.Y=s.cursor_y;
 mouse.MouseInput.Event=EMIE_LMOUSE_PRESSED_DOWN;CHECK(on_menu_event(mouse));
 mouse.MouseInput.Event=EMIE_LMOUSE_LEFT_UP;CHECK(on_menu_event(mouse));
 CHECK(enters==1 && !s.keyboard && f.device.events.empty());
}
}

int main() {
 pointer_order_and_masks();
 CHECK(gui::IGUIElement::living==0);
 menu_replacement_quarantines_held_input();
 gameplay_exit_does_not_leave_stale_physical_holds();
 active_controller_change_clears_old_masks();
 keyboard_waits_for_native_drag_release();
 done_submits_after_detach_and_survives_removal();
 cancel_restores_without_submission();
 detached_targets_and_shutdown_never_submit();
 cancel_callback_and_menu_sync_can_remove_form();
 done_same_menu_has_no_click_leak_and_blocks_square_release();
 direct_mouse_done_is_consumed();
 CHECK(gui::IGUIElement::living==0);
 std::cout << "PASS PS4 input adapter pointer / keyboard / callback lifetime tests\n";
}
