#pragma once
#include "ControllerData.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace diagnostic::mapping {
enum Button {None,LeftTrigger,LeftGrip,LeftPrimary,LeftSecondary,LeftStick,LeftMenu,
 RightTrigger,RightGrip,RightPrimary,RightSecondary,RightStick,RightMenu,RightStickUp,RightStickDown,LeftStickUp,LeftStickDown,ButtonCount};
enum Gesture {Hold,Press,DoubleTap,HoldHalfSecond,GestureCount};
enum Action {Interact,Block,Support,ShieldBlock,Jump,Inventory,Weapon,Menu,Sprint,Sneak,Heal,
 RadialOpen,RadialConfirm,RadialCancel,RadialPage,Recenter,Locator,ReturnHuman,UiSelect,UiOneItem,OffhandTrigger,ActionCount};
inline constexpr wchar_t const* ButtonNames[]={L"Unassigned",L"Left trigger",L"Left grip",L"Left primary (X / A)",L"Left secondary (Y / B)",L"Left stick click (L3)",L"Left application menu",L"Right trigger",L"Right grip",L"Right primary (A)",L"Right secondary (B)",L"Right stick click (R3)",L"Right application menu",L"Right stick up",L"Right stick down",L"Left stick up",L"Left stick down"};
inline constexpr wchar_t const* GestureNames[]={L"Hold",L"Single press",L"Double tap (450 ms)",L"Hold for 0.5 seconds"};
inline constexpr wchar_t const* Keys[]={L"Interact",L"Block",L"SupportGrip",L"ShieldBlock",L"Jump",L"Inventory",L"DrawWeapon",L"MainMenu",L"Sprint",L"Sneak",L"QuickHeal",L"RadialOpen",L"RadialConfirm",L"RadialCancel",L"RadialPage",L"Recenter",L"ItemLocator",L"ReturnHuman",L"MenuSelect",L"TradeOneItem",L"OffhandTrigger"};
inline constexpr wchar_t const* ActionNames[]={L"Interact / fire / bow string / power strike",L"Weapon block",L"Support grip (2H / staff / crossbow)",L"Shield block (1H + shield only)",L"Jump",L"Inventory",L"Draw / sheathe weapon",L"Main menu / back",L"Sprint until stop",L"Toggle sneak",L"Quick heal",L"Open / close radial",L"Radial: confirm",L"Radial: cancel",L"Radial: next page",L"Recenter camera",L"Toggle item locator",L"Return from animal form",L"Menu: select / drag stack",L"Trade: one item",L"Off-hand trigger / power strike"};
// Contexts are for conflict explanations; the existing game translator owns
// actual gameplay/menu/weapon restrictions and native skill checks.
inline constexpr unsigned Contexts[]={1,1,1,1,1,1,1,3,1,1,1,7,4,4,4,7,1,8,2,2,1};
struct Binding {int button=None,modifier=None,gesture=Hold;bool operator==(Binding const& x)const{return button==x.button&&modifier==x.modifier&&gesture==x.gesture;}};
struct Config {
 bool leftHanded=false;
 std::array<Binding,ActionCount> bindings{{
 {RightTrigger,None,Hold},{RightGrip,None,Hold},{LeftGrip,None,Hold},{LeftGrip,None,Hold},
 {RightStickUp,None,Press},{LeftPrimary,None,Hold},{RightSecondary,None,Hold},{LeftMenu,None,Hold},
 {LeftStick,None,Press},{RightStickDown,None,Press},{LeftTrigger,None,DoubleTap},
 {RightStick,None,Press},{RightPrimary,None,Press},{LeftPrimary,None,Press},{RightSecondary,None,Press},
 {RightStick,LeftStick,HoldHalfSecond},{LeftPrimary,LeftTrigger,Press},{LeftPrimary,LeftSecondary,Hold},
 {RightTrigger,None,Hold},{RightGrip,None,Press},{LeftTrigger,None,Hold}}};
 bool operator==(Config const& x)const{return leftHanded==x.leftHanded&&bindings==x.bindings;}
};
inline bool Valid(Binding const& b){return b.button>=None&&b.button<ButtonCount&&b.modifier>=None&&b.modifier<ButtonCount&&b.gesture>=Hold&&b.gesture<GestureCount&&(b.modifier==None||b.modifier!=b.button);}
inline uint32_t Bit(int button){return button>None&&button<ButtonCount?1u<<(button-1):0;}
// Application-menu buttons stay where the runtime exposes them.
inline int LayoutButton(int button,bool leftHanded){
 if(!leftHanded)return button;
 if(button>=LeftTrigger&&button<=LeftStick)return button+6;
 if(button>=RightTrigger&&button<=RightStick)return button-6;
 if(button==RightStickUp||button==RightStickDown)return button+2;
 if(button==LeftStickUp||button==LeftStickDown)return button-2;
 return button;
}
inline uint32_t Mask(Binding const& b,bool leftHanded=false){return Bit(LayoutButton(b.button,leftHanded))|Bit(LayoutButton(b.modifier,leftHanded));}
inline bool Active(ControllerFrame const& f,Action action){return f.bindingsMapped&&(f.bindingHeld&(1u<<action));}
inline bool Pushed(ControllerFrame const& f,Action action){return f.bindingsMapped&&(f.bindingPressed&(1u<<action));}
inline bool Chord(ControllerFrame const& f,Action action){return f.bindingsMapped&&(f.bindingChord&(1u<<action));}
class Engine {
 struct State {bool down=false,fired=false;uint64_t began=0,tap=0;};
 std::array<State,ActionCount> states{};Config configured;bool configuredOnce=false,armed=false;
 uint32_t analog=0,consumed=0,enabledBefore=~0u;uint64_t previous=0;ControllerFrame cached;
 static void Digital(ControllerFrame& f){for(auto& h:f.hands){h.trigger=h.squeeze=0;h.primary=h.secondary=h.stickClick=h.menu=false;}f.bindingHeld=f.bindingPressed=f.bindingChord=0;}
public:
 void Reset(){states={};armed=false;analog=consumed=0;previous=0;cached={};}
 uint32_t Buttons(ControllerFrame const& f){
  uint32_t bits=0;
  for(unsigned h=0;h<2;++h){auto const& hand=f.hands[h];unsigned shift=h*6;bool active=hand.active||((f.availableButtons>>shift)&63u)!=0;
   for(unsigned i=0;i<2;++i){auto bit=1u<<(shift+i);float value=i?hand.squeeze:hand.trigger;
    if(!active||!std::isfinite(value)||value<.4f)analog&=~bit;else if(value>.65f)analog|=bit;
   }
   if(!active)continue;bits|=analog&((3u)<<shift);
   if(hand.primary)bits|=1u<<(shift+2);if(hand.secondary)bits|=1u<<(shift+3);
   if(hand.stickClick)bits|=1u<<(shift+4);if(hand.menu)bits|=1u<<(shift+5);
  }
  for(unsigned h=0;h<2;++h){
   auto const& hand=f.hands[h];float x=hand.stick[0],y=hand.stick[1];
   auto up=Bit(h?RightStickUp:LeftStickUp),down=Bit(h?RightStickDown:LeftStickDown),vertical=up|down;
   auto prior=analog&vertical;analog&=~vertical;
   if((hand.active||(f.availableButtons&vertical))&&std::isfinite(x)&&std::isfinite(y)){
    if(y>=.4f&&(prior==up||(y>.65f&&y>std::fabs(x))))analog|=up;
    else if(y<=-.4f&&(prior==down||(-y>.65f&&-y>std::fabs(x))))analog|=down;
   }
   bits|=analog&vertical;
  }
  return bits;
 }
 ControllerFrame Step(ControllerFrame const& raw,Config const& config,uint64_t now,bool suppress=false,uint32_t enabled=~0u){
  bool changed=!configuredOnce||!(configured==config);if(changed){configured=config;configuredOnce=true;Reset();}
  if(enabled!=enabledBefore){Reset();enabledBefore=enabled;}
  uint32_t buttons=Buttons(raw);auto out=raw;Digital(out);out.bindingsMapped=true;
  if(config.leftHanded)for(unsigned axis=0;axis<2;++axis)std::swap(out.hands[0].stick[axis],out.hands[1].stick[axis]);
  bool fresh=raw.focused&&raw.timeMs&&now>=raw.timeMs&&now-raw.timeMs<=100;
  if(!raw.focused||suppress||(previous&&raw.timeMs<previous)){Reset();out.focused=false;return out;}
  // Streaming/render stalls are not button releases or focus loss. Suspend
  // output while stale, retaining edges so held grips resume without a new
  // press, detach, synthetic double tap or neutral lockout after zone loads.
  if(!fresh){for(unsigned i=0;i<ActionCount;++i)if(config.bindings[i].gesture==DoubleTap)states[i].tap=0;out.focused=false;return out;}
  if(raw.timeMs==previous)return cached;
  previous=raw.timeMs;
  if(!armed){bool sticks=true;for(auto const& h:raw.hands)sticks&=std::isfinite(h.stick[0])&&std::isfinite(h.stick[1])&&std::hypot(h.stick[0],h.stick[1])<.25f;
   if(!buttons&&sticks)armed=true;cached=out;cached.focused=false;return cached;}
  consumed&=buttons;
  for(unsigned i=0;i<ActionCount;++i){auto const& b=config.bindings[i];auto mask=Mask(b,config.leftHanded);
   if((enabled&(1u<<i))&&b.button&&b.modifier&&(buttons&mask)==mask){out.bindingChord|=1u<<i;consumed|=mask;}
  }
  for(unsigned i=0;i<ActionCount;++i){auto const& b=config.bindings[i];auto& s=states[i];auto mask=Mask(b,config.leftHanded);
   bool down=(enabled&(1u<<i))&&b.button&&(buttons&mask)==mask;
   if(!b.modifier&&(consumed&mask)&&!(i==Support&&b.gesture==Hold)){down=false;s.tap=0;}
   bool rising=down&&!s.down;bool active=false;
   if(s.tap&&raw.timeMs-s.tap>450)s.tap=0;
   if(rising){s.began=raw.timeMs;s.fired=false;}
   switch(b.gesture){
    case Hold:active=down;break;
    case Press:active=rising;break;
    case DoubleTap:if(rising){if(s.tap&&raw.timeMs-s.tap>=80){active=true;s.tap=0;}else s.tap=raw.timeMs;}break;
    case HoldHalfSecond:active=down&&raw.timeMs>=s.began&&raw.timeMs-s.began>=500;break;
   }
   if(active)out.bindingHeld|=1u<<i;
   if(active&&!s.fired){out.bindingPressed|=1u<<i;s.fired=true;}
   if(!down)s.fired=false;s.down=down;
  }
  auto on=[&](Action a){return Active(out,a);};
  out.hands[0].trigger=on(OffhandTrigger)?1.f:0.f;
  out.hands[0].squeeze=on(Support)?1.f:0.f;
  out.hands[1].trigger=on(Interact)?1.f:0.f;out.hands[1].squeeze=on(Block)?1.f:0.f;
  out.hands[0].primary=on(Inventory);out.hands[0].secondary=on(Sneak);out.hands[0].menu=on(Menu);
  out.hands[0].stickClick=on(Sprint);out.hands[1].stickClick=on(RadialOpen);
  out.hands[1].primary=on(Jump);out.hands[1].secondary=on(Weapon);
  cached=out;return out;
 }
};
}
