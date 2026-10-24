// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
using s32 = int32_t;
using u64 = uint64_t;
constexpr unsigned SDL_BUTTON_LEFT = 1, SDL_BUTTON_RIGHT = 3;
#define SDL_BUTTON_MASK(button) (1u << ((button) - 1))
namespace core {
template <typename T> struct position2d { T X, Y; position2d(T x=0,T y=0):X(x),Y(y){} };
using position2di = position2d<s32>;
template <typename T> struct rect {
 T left=0,top=0,right=0,bottom=0;
 rect()=default; rect(T l,T t,T r,T b):left(l),top(t),right(r),bottom(b){}
 bool isPointInside(position2di p) const {return p.X>=left && p.X<right && p.Y>=top && p.Y<bottom;}
};
struct dimension { unsigned Width=1280, Height=720; };
}
namespace gui {
class IGUIElement;
enum EGUI_EVENT_TYPE { EGET_EDITBOX_CHANGED, EGET_EDITBOX_ENTER };
enum EGUI_ELEMENT_TYPE { EGUIET_ELEMENT, EGUIET_EDIT_BOX };
}
enum EEVENT_TYPE { EET_GAMEPAD_AXIS_EVENT,EET_GAMEPAD_BUTTON_EVENT,EET_MOUSE_INPUT_EVENT,EET_KEY_INPUT_EVENT,EET_GUI_EVENT };
enum EMOUSE_INPUT_EVENT {EMIE_LMOUSE_PRESSED_DOWN, EMIE_LMOUSE_LEFT_UP, EMIE_RMOUSE_PRESSED_DOWN, EMIE_RMOUSE_LEFT_UP, EMIE_MOUSE_MOVED, EMIE_MOUSE_WHEEL};
enum class GamepadAxis {LEFTX,LEFTY};
enum class GamepadButton {SOUTH,EAST,START,NORTH,WEST,DPAD_UP,DPAD_DOWN};
constexpr int KEY_ESCAPE=27;
struct SEvent {
 EEVENT_TYPE EventType{};
 struct {uint32_t ID=0; GamepadAxis Axis{}; int Value=0;} GamepadAxisEvent;
 struct {uint32_t ID=0; GamepadButton Button{}; bool PressedDown=false;} GamepadButtonEvent;
 struct {s32 X=0,Y=0; uint32_t ButtonStates=0; bool Shift=false,Control=false,Simulated=false; EMOUSE_INPUT_EVENT Event{}; float Wheel=0; unsigned Button=0;} MouseInput;
 struct {wchar_t Char=0; int Key=0,SystemKeyCode=0; bool PressedDown=false,Shift=false,Control=false;} KeyInput;
 struct {gui::IGUIElement *Caller=nullptr,*Element=nullptr; gui::EGUI_EVENT_TYPE EventType{};} GUIEvent;
};
namespace gui {
class IGUIElement {
 int refs_=1;
 IGUIElement *parent_=nullptr;
 std::vector<IGUIElement*> children_;
public:
 inline static int living=0;
 std::function<bool(const SEvent&)> callback;
 core::rect<s32> bounds;
 explicit IGUIElement(IGUIElement *parent=nullptr):parent_(parent) {
  ++living;
  if(parent_) {parent_->children_.push_back(this); grab();}
 }
 virtual ~IGUIElement() {
  --living;
  for(auto *child:children_) {child->parent_=nullptr; child->drop();}
 }
 void grab() {++refs_;}
 void drop() {if(--refs_==0) delete this;}
 void remove() {
  if(!parent_) return;
  auto &children=parent_->children_;
  children.erase(std::find(children.begin(),children.end(),this));
  parent_=nullptr; drop();
 }
 IGUIElement *getParent() const {return parent_;}
 virtual EGUI_ELEMENT_TYPE getType() const {return EGUIET_ELEMENT;}
 virtual bool OnEvent(const SEvent &e) {return callback ? callback(e) : (parent_ ? parent_->OnEvent(e) : false);}
 core::rect<s32> getAbsolutePosition() const {
  auto r=bounds;
  for(auto *p=parent_;p;p=p->getParent()) {r.left+=p->bounds.left;r.right+=p->bounds.left;r.top+=p->bounds.top;r.bottom+=p->bounds.top;}
  return r;
 }
 void bringToFront(IGUIElement*) {}
};
class IGUIEditBox:public IGUIElement {
 std::wstring text_;
public:
 using IGUIElement::IGUIElement;
 EGUI_ELEMENT_TYPE getType() const override {return EGUIET_EDIT_BOX;}
 const wchar_t *getText() const {return text_.c_str();}
 void setText(const wchar_t *t) {text_=t;}
 unsigned getMax() const {return 0;}
};
class IGUIButton:public IGUIElement {
public:
 using IGUIElement::IGUIElement;
 void setText(const wchar_t*) {}
};
class IGUIStaticText:public IGUIElement {public: using IGUIElement::IGUIElement;};
class IGUIEnvironment {
 IGUIElement *root_=new IGUIElement;
public:
 IGUIElement *focus=nullptr;
 ~IGUIEnvironment(){root_->drop();}
 IGUIElement *getRootGUIElement(){return root_;}
 IGUIElement *getFocus(){return focus;}
 IGUIStaticText *addStaticText(const wchar_t*,core::rect<s32> r,bool,bool,IGUIElement *parent,int,bool) {
  auto *e=new IGUIStaticText(parent);e->bounds=r;e->drop();return e;
 }
 IGUIButton *addButton(core::rect<s32> r,IGUIElement *parent,int,const wchar_t*) {
  auto *e=new IGUIButton(parent);e->bounds=r;e->drop();return e;
 }
};
class ICursorControl {
 core::position2di position_{640,360};
public:
 const core::position2di &getPosition(bool=false) const {return position_;}
 void setPosition(s32 x,s32 y){position_={x,y};}
 void setVisible(bool){}
};
}
namespace video {
struct SColor {explicit SColor(uint32_t){}};
class IVideoDriver {
public:
 core::dimension getScreenSize(){return {};}
 void draw2DRectangle(SColor,core::rect<s32>){}
};
}
class IrrlichtDevice {
public:
 gui::IGUIEnvironment env;
 gui::ICursorControl cursor;
 video::IVideoDriver driver;
 std::vector<SEvent> events;
 std::function<void(const SEvent&)> callback;
 gui::IGUIEnvironment *getGUIEnvironment(){return &env;}
 gui::ICursorControl *getCursorControl(){return &cursor;}
 video::IVideoDriver *getVideoDriver(){return &driver;}
 void postEventFromUser(const SEvent &event){events.push_back(event);if(callback)callback(event);}
};
inline gui::IGUIEnvironment *guienv=nullptr;
struct MenuManager {
 gui::IGUIElement *top=nullptr;
 gui::IGUIElement *tryGetTopMenu(){return top;}
};
inline MenuManager g_menumgr;
inline bool isMenuActive(){return g_menumgr.top!=nullptr;}
class RenderingEngine {public:inline static IrrlichtDevice *device=nullptr;static IrrlichtDevice *get_raw_device(){return device;}};
namespace porting {inline u64 getTimeUs(){static u64 time=0; return time+=16667;}}
inline std::ostream &infostream=std::cout;
