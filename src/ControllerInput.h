#pragma once
#include <windows.h>
#include "ControllerControls.h"
#include <cstring>
#include <cwchar>
namespace diagnostic {
struct ControllerBinding {int type=-1,offset=0;};
class ControllerInput {
    ControllerBinding bindings[CommandCount]{};bool held[2][256]{};ControllerTurn turn;int pulse=-1;uint64_t pulseUntil=0;
    static bool Supported(int type,int offset){return (type==0&&offset>0&&offset<256)||(type==1&&offset>=3&&offset<=5);}
    static bool Emit(unsigned type,unsigned offset,bool down) {
        INPUT input{};
        if(type==0){input.type=INPUT_KEYBOARD;input.ki.wScan=WORD(offset&0x7f);input.ki.dwFlags=KEYEVENTF_SCANCODE|(offset&0x80?KEYEVENTF_EXTENDEDKEY:0)|(down?0:KEYEVENTF_KEYUP);}
        else{input.type=INPUT_MOUSE;input.mi.dwFlags=offset==3?(down?MOUSEEVENTF_LEFTDOWN:MOUSEEVENTF_LEFTUP):offset==4?(down?MOUSEEVENTF_RIGHTDOWN:MOUSEEVENTF_RIGHTUP):(down?MOUSEEVENTF_MIDDLEDOWN:MOUSEEVENTF_MIDDLEUP);}
        return SendInput(1,&input,sizeof(input))==1;
    }
public:
    static void NativeMovementOnly(ControllerCommands& commands){
        for(unsigned key=Forward;key<=Right;++key)commands.held[key]=false;
        commands.held[Sprint]=false;
    }
    static bool Send(unsigned type,unsigned offset,bool down){return Emit(type,offset,down);}
    ControllerBinding Binding(unsigned command)const{return command<CommandCount?bindings[command]:ControllerBinding{};}
    void Initialize(wchar_t const* nativeOptions) {
        wchar_t const* sections[]={L"SessionKey.Forward",L"SessionKey.Backward",L"SessionKey.StrafeLeft",L"SessionKey.StrafeRight",L"SessionKey.RotateCamLeft",L"SessionKey.RotateCamRight",L"SessionKey.Use1",L"SessionKey.Use2",L"SessionKey.Up",L"SessionKey.InventoryMode",L"SessionKey.WeaponMode"};
        for(unsigned command=0;command<11;++command)for(unsigned key=1;key<=2;++key){wchar_t name[32];swprintf_s(name,L"Key%u.Type",key);int type=GetPrivateProfileIntW(sections[command],name,-1,nativeOptions);swprintf_s(name,L"Key%u.Offset",key);int offset=GetPrivateProfileIntW(sections[command],name,0,nativeOptions);if(Supported(type,offset)){bindings[command]={type,offset};break;}}
        bindings[MenuLeft]={0,0x1e};bindings[MenuRight]={0,0x20}; // A/D: actual native lockpick controls, independent of routine action
        bindings[Back]={0,1};bindings[MainMenu]=bindings[Back];bindings[UiClick]={1,3};bindings[UiRightClick]={1,4};
        bindings[LocatorToggle]={0,0x64}; // F13, reserved by the optional locator import.
        wchar_t const* movementModes[]={L"SessionKey.Walk",L"SessionKey.Down",L"SessionKey.WalkToggle"};
        for(unsigned i=0;i<3;++i)for(unsigned key=1;key<=2;++key){wchar_t name[32];swprintf_s(name,L"Key%u.Type",key);int type=GetPrivateProfileIntW(movementModes[i],name,-1,nativeOptions);swprintf_s(name,L"Key%u.Offset",key);int offset=GetPrivateProfileIntW(movementModes[i],name,0,nativeOptions);if(Supported(type,offset)){bindings[Sprint+i]={type,offset};break;}}
        wchar_t const* extra[]={L"SessionKey.QuickUse1",L"SessionKey.QuickUse2",L"SessionKey.QuickUse3",L"SessionKey.QuickUse4",L"SessionKey.QuickUse5",L"SessionKey.QuickUse6",L"SessionKey.QuickUse7",L"SessionKey.QuickUse8",L"SessionKey.QuickUse9",L"SessionKey.QuickUse0",L"SessionKey.JournalModeCharScreen",L"SessionKey.JournalModeLog",L"SessionKey.JournalModeMap",L"SessionKey.JournalModeMagBook",L"SessionKey.QuickSave"};
        for(unsigned i=0;i<15;++i)for(unsigned key=1;key<=2;++key){wchar_t name[32];swprintf_s(name,L"Key%u.Type",key);int type=GetPrivateProfileIntW(extra[i],name,-1,nativeOptions);swprintf_s(name,L"Key%u.Offset",key);int offset=GetPrivateProfileIntW(extra[i],name,0,nativeOptions);if(Supported(type,offset)){bindings[Quick1+i]={type,offset};break;}}
    }
    template<class Sink> void Apply(ControllerCommands const& commands,Sink emit) {
        bool desired[2][256]{};for(unsigned command=0;command<CommandCount;++command){auto const& binding=bindings[command];if(commands.held[command]&&Supported(binding.type,binding.offset))desired[binding.type][binding.offset]=true;}
        // Release before pressing when modes or physical bindings change.
        for(unsigned type=0;type<2;++type)for(unsigned offset=0;offset<256;++offset)if(held[type][offset]&&!desired[type][offset]&&emit(type,offset,false))held[type][offset]=false;
        for(unsigned type=0;type<2;++type)for(unsigned offset=0;offset<256;++offset)if(!held[type][offset]&&desired[type][offset]&&emit(type,offset,true))held[type][offset]=true;
    }
    void Apply(ControllerCommands const& commands){Apply(commands,Emit);}
    template<class Sink> void Update(ControllerCommands commands,uint64_t now,bool allowed,Sink emit) {
        if(!allowed){pulse=-1;pulseUntil=0;Apply(ControllerCommands{},emit);return;}
        if(commands.pulse>=0&&commands.pulse<CommandCount){pulse=commands.pulse;pulseUntil=now+80;}
        if(pulse>=0){if(now<pulseUntil)commands.held[pulse]=true;else pulse=-1;}
        Apply(commands,emit);
    }
    void Update(ControllerCommands commands,uint64_t now,bool allowed){Update(commands,now,allowed,Emit);}
    void Turn(ControllerCommands const& commands,int speedPercent,uint64_t now){int pixels=turn.Step(commands.turnX,speedPercent,now);if(pixels){INPUT input{};input.type=INPUT_MOUSE;input.mi.dx=pixels;input.mi.dwFlags=MOUSEEVENTF_MOVE;SendInput(1,&input,sizeof(input));}}
    void Release(){pulse=-1;pulseUntil=0;Apply(ControllerCommands{});}
};
}
