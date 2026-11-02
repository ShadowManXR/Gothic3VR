#pragma once
#include "ControllerData.h"
#include "ControllerMapping.h"
#include "MenuPointer.h"
#include "QuickRadial.h"
#include "ControllerLocomotion.h"
#include "LockpickSteps.h"
#include "ControllerMoveButtons.h"
#include "MenuSingleClick.h"
#include "ControllerLocatorGesture.h"
#include <cmath>
#include <algorithm>
namespace diagnostic {
enum ControllerCommand {Forward,Backward,Left,Right,TurnLeft,TurnRight,Use1,Use2,Jump,Inventory,Weapon,Back,UiClick,Quick1,Quick2,Quick3,Quick4,Quick5,Quick6,Quick7,Quick8,Quick9,Quick0,Character,Journal,Map,Magic,QuickSave,Sprint,Sneak,WalkToggle,MenuLeft,MenuRight,MainMenu,UiRightClick,LocatorToggle,CommandCount};
struct ControllerCommands {bool held[CommandCount]{};bool pointer=false,recenter=false;float u=0,v=0,turnX=0,moveX=0,moveZ=0;bool sneakToggleButton=false,quickHeal=false,lockpick=false,captured=false,shieldGrip=false;RadialState radial;int pulse=-1;};
class ControllerTurn {
    uint64_t previous=0;float remainder=0;
public:
    int Step(float axis,int speedPercent,uint64_t now) {
        uint64_t elapsed=previous&&now>=previous?now-previous:0;previous=now;
        if(!std::isfinite(axis)||std::fabs(axis)<=.4f){remainder=0;return 0;}
        float magnitude=(std::min(std::fabs(axis),1.f)-.4f)/.6f;
        remainder+=(axis<0?-1.f:1.f)*magnitude*600.f*float(speedPercent)*.01f*float(std::min(elapsed,uint64_t(50)))*.001f;
        int pixels=int(remainder);remainder-=float(pixels);return pixels;
    }
};
inline bool ControllerPanelHit(ControllerPose const& pose,float width,float height,float distance,float vertical,float& u,float& v) {
    if(!pose.valid||width<=0||height<=0||distance<=0)return false;
    auto const* q=pose.quaternion;float norm=0;for(unsigned i=0;i<4;++i){if(!std::isfinite(q[i]))return false;norm+=q[i]*q[i];}
    if(std::fabs(norm-1)>.02f)return false;
    for(float value:pose.position)if(!std::isfinite(value))return false;
    float dx=-2*(q[0]*q[2]+q[3]*q[1]),dy=-2*(q[1]*q[2]-q[3]*q[0]),dz=-(1-2*(q[0]*q[0]+q[1]*q[1]));
    if(dz>=-.001f)return false;float t=(-distance-pose.position[2])/dz;if(t<=0)return false;
    u=.5f+(pose.position[0]+t*dx)/width;v=.5f-(pose.position[1]+t*dy-vertical)/height;
    return std::isfinite(u)&&std::isfinite(v)&&u>=0&&u<=1&&v>=0&&v<=1;
}
class ControllerTranslator {
    bool previousMenu=false,previousDirectional=false,previousLockpick=false,lockpickLatched=false,initialized=false,armed=false,menuAxisArmed=false;int menuSide=0,menuPulseSide=0;uint64_t menuPulseBegan=0;NativeStickDirections directions;LockpickSteps lockpickSteps;
    MenuPointer menuPointer;ControllerCameraReset cameraReset;QuickRadial quickRadial;ControllerMoveButtons moveButtons;
    MenuSingleClick tradeClick;
    ControllerLocatorGesture locatorGesture;
    bool locatorHealPending=false;
    float previousPanel[4]{};bool radialTurnCaptured=false;
public:
    void CancelSprint(){moveButtons.CancelSprint();}
    void ResetPointer(){menuPointer.Reset();quickRadial.Cancel();moveButtons.Reset();tradeClick.Reset();locatorGesture.Reset();locatorHealPending=false;}
    ControllerCommands Translate(ControllerFrame const& frame,uint64_t now,bool allowed,bool menu,float width,float height,float distance,float vertical,bool radialAllowed=true,bool lockpick=false,bool directionalActivity=false,bool trade=false,bool locatorEnabled=false) {
        ControllerCommands out;bool fresh=frame.focused&&now>=frame.timeMs&&now-frame.timeMs<=100;
        if(!allowed||!frame.focused||!radialAllowed||(initialized&&menu!=previousMenu))quickRadial.Cancel();
        if(!allowed||!fresh){moveButtons.Reset();tradeClick.Reset();locatorGesture.Reset();locatorHealPending=false;armed=false;directions.Reset();lockpickSteps.Reset();lockpickLatched=false;menuAxisArmed=false;menuSide=menuPulseSide=0;menuPulseBegan=0;menuPointer.Reset();cameraReset.Step(false,false,false,now);
            // A slow render is not a physical release or a menu cancellation.
            // Preserve the wheel and button edges, but emit no native inputs
            // until a fresh sample arrives. Real focus/world loss cancels above.
            out.radial=quickRadial.State();return out;}
        auto const& left=frame.hands[0];auto const& right=frame.hands[1];
        auto locator=locatorGesture.Step(locatorEnabled&&armed&&left.active&&!menu&&!directionalActivity&&radialAllowed&&!quickRadial.State().visible,left.primary,left.trigger);
        if(frame.bindingsMapped){locator.pulse=locatorEnabled&&mapping::Pushed(frame,mapping::Locator);locator.consumeX=locatorEnabled&&mapping::Chord(frame,mapping::Locator);locator.suppressHeal=locator.consumeX;}
        if(!locatorEnabled||menu||directionalActivity||!left.active||!radialAllowed)locatorHealPending=false;
        if(locator.suppressHeal){moveButtons.CancelHeal();locatorHealPending=false;}
        out.recenter=frame.bindingsMapped?mapping::Pushed(frame,mapping::Recenter):cameraReset.Step(left.active&&right.active,left.stickClick,right.stickClick,frame.timeMs);
        if(frame.bindingsMapped?(mapping::Chord(frame,mapping::Recenter)||mapping::Active(frame,mapping::Recenter)):(left.active&&right.active&&left.stickClick&&right.stickClick)){moveButtons.Reset();tradeClick.Reset();locatorHealPending=false;armed=false;menuPointer.Reset();quickRadial.Cancel();lockpickSteps.Reset();out.captured=true;return out;}
        bool menuChanged=initialized&&(menu!=previousMenu||directionalActivity!=previousDirectional);if(menuChanged)quickRadial.Cancel();previousMenu=menu;previousDirectional=directionalActivity;
        if(menuChanged)lockpickLatched=false;
        if(lockpick)lockpickLatched=true;else if(!menu&&!directionalActivity)lockpickLatched=false;
        lockpick=lockpickLatched; // A transient routine/target flag cannot turn a held lockpick gesture into generic repeated input.
        auto radial=quickRadial.Step(radialAllowed&&left.active&&right.active,frame.bindingsMapped?mapping::Active(frame,mapping::RadialOpen):right.stickClick,frame.bindingsMapped?false:left.stickClick,frame.bindingsMapped?mapping::Active(frame,mapping::RadialPage):right.secondary,right.stick[0],right.stick[1],frame.timeMs,frame.bindingsMapped?mapping::Active(frame,mapping::RadialConfirm):right.primary,frame.bindingsMapped?mapping::Active(frame,mapping::RadialCancel):left.primary);
        if(radial.capture){radialTurnCaptured=true;moveButtons.Reset();tradeClick.Reset();locatorHealPending=false;armed=false;directions.Reset();lockpickSteps.Reset();menuAxisArmed=false;menuSide=menuPulseSide=0;menuPulseBegan=0;menuPointer.Reset();out.radial=radial.state;out.captured=true;
            if(radial.choice>=0){static constexpr int actions[]={Inventory,Character,Journal,Map,Magic,MainMenu,QuickSave,Back};out.pulse=radial.choice<10?Quick1+radial.choice:actions[radial.choice-10];}
            return out;}
        float panel[4]={width,height,distance,vertical};bool layoutChanged=false;
        for(unsigned i=0;i<4;++i){layoutChanged|=panel[i]!=previousPanel[i];previousPanel[i]=panel[i];}
        if(!initialized||menuChanged||layoutChanged||lockpick!=previousLockpick){initialized=true;armed=false;directions.Reset();lockpickSteps.Reset();menuAxisArmed=false;menuSide=menuPulseSide=0;menuPulseBegan=0;menuPointer.Reset();}previousLockpick=lockpick;
        // A held trigger that opens a conversation must not click its first
        // option; a held inventory button must not immediately close it.
        if(!left.primary&&!left.secondary&&!left.menu&&!right.primary&&!right.secondary&&right.squeeze<.4f&&right.trigger<.4f)armed=true;
        // Release the shared radial/turn axis before camera turning resumes.
        if(!right.active||(std::isfinite(right.stick[0])&&std::isfinite(right.stick[1])&&std::hypot(right.stick[0],right.stick[1])<.25f))radialTurnCaptured=false;
        auto moveActions=moveButtons.Step(left.active,!menu&&!directionalActivity,left.trigger,left.stickClick,frame.timeMs,std::isfinite(left.stick[0])&&std::isfinite(left.stick[1])&&std::hypot(left.stick[0],left.stick[1])>.25f,frame.bindingsMapped,mapping::Pushed(frame,mapping::Heal),mapping::Pushed(frame,mapping::Sprint));
        if(!menu||!trade)tradeClick.Reset();
        if(menu||directionalActivity){directions.Reset();out.lockpick=lockpick;
            bool axisValid=left.active&&std::isfinite(left.stick[0]);float axis=axisValid?left.stick[0]:0;
            if(!axisValid){menuAxisArmed=false;menuSide=menuPulseSide=0;menuPulseBegan=0;}else if(std::fabs(axis)<.25f)menuAxisArmed=true;
            if(menuAxisArmed){if(axis>.55f)menuSide=1;else if(axis<-.55f)menuSide=-1;else if(std::fabs(axis)<.35f)menuSide=0;}
            if(menuSide!=menuPulseSide){menuPulseSide=menuSide;menuPulseBegan=menuSide?frame.timeMs:0;}
            bool pulse=menuAxisArmed&&menuPulseBegan&&frame.timeMs>=menuPulseBegan&&frame.timeMs-menuPulseBegan<80;
            out.held[MenuLeft]=pulse&&menuSide<0;out.held[MenuRight]=pulse&&menuSide>0;
            if(lockpick){int step=lockpickSteps.Step(axisValid,left.stick[0],left.stick[1],frame.timeMs);out.held[MenuLeft]=step<0;out.held[MenuRight]=step>0;}
            if(menu){out.pointer=right.active&&ControllerPanelHit(right.aimInView,width,height,distance,vertical,out.u,out.v);out.held[UiClick]=armed&&out.pointer&&(frame.bindingsMapped?mapping::Active(frame,mapping::UiSelect):right.trigger>.65f);
                out.held[UiRightClick]=tradeClick.Step(trade&&armed&&out.pointer&&!out.held[UiClick],frame.bindingsMapped?mapping::Active(frame,mapping::UiOneItem):right.squeeze>.65f,frame.timeMs);
                if(out.pointer)menuPointer.Filter(out.u,out.v,frame.timeMs,out.held[UiClick]||out.held[UiRightClick]);else menuPointer.Reset();}
            else {menuPointer.Reset();out.held[Use1]=right.active&&armed&&right.trigger>.65f;out.held[Use2]=right.active&&armed&&right.squeeze>.65f;}}
        else {
            if(left.active){directions.Step(left.stick[0],left.stick[1],out.moveX,out.moveZ);out.held[Forward]=out.moveZ>.1f;out.held[Backward]=out.moveZ<-.1f;out.held[Left]=out.moveX<-.1f;out.held[Right]=out.moveX>.1f;out.sneakToggleButton=left.secondary;out.held[Sprint]=moveActions.sprint&&std::hypot(out.moveX,out.moveZ)>.25f;}
            if(right.active){out.turnX=radialTurnCaptured?0:right.stick[0];out.held[Jump]=armed&&right.primary;out.held[Use1]=armed&&right.trigger>.65f;}
            out.held[Use2]=right.active&&armed&&right.squeeze>.65f;out.held[Weapon]=right.active&&armed&&right.secondary;
            // Equipment decides whether this can become a native block. Keep
            // support grip separate from Use2 for every other weapon/menu.
            out.shieldGrip=left.active&&armed&&(frame.bindingsMapped?mapping::Active(frame,mapping::ShieldBlock):left.squeeze>.65f);
            out.quickHeal=left.active&&armed&&!locator.suppressHeal&&moveActions.heal;
            // With the locator available, commit the second heal tap on release.
            // X may arrive after the trigger press; it must still cancel healing.
            if(locatorEnabled){if(out.quickHeal)locatorHealPending=true;out.quickHeal=false;
                if(left.trigger<.4f&&locatorHealPending){out.quickHeal=true;locatorHealPending=false;}}
            if(locator.pulse)out.pulse=LocatorToggle;
        }
        out.held[Inventory]=left.active&&armed&&left.primary&&!locator.consumeX;out.held[Back]=left.active&&armed&&left.menu;

        return out;
    }
};
}
