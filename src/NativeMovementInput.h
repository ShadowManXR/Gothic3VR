#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <initializer_list>
namespace diagnostic
{
// Game.dll 1.75.20108.14: OnAction reads the session key as a WORD at
// offset zero. The SDK deliberately leaves eSGameKeyMessage incomplete.
// Use only after the binary profile and the two readable bytes are checked.
inline bool HorizontalMovementMessage(void const* message, uint16_t& key)
{
    std::memcpy(&key,message,sizeof(key));
    return key>=1 && key<=4; // Forward, Backward, StrafeLeft, StrafeRight.
}
// CP 1.75.20108.14 OnAction writes these four timers; ProcessMovements
// consumes them independently of the wished vector. Replace only joystick-
// owned directional input, before sending the current native action batch.
// Up/Down, rotation, Sprint, movement/physics properties and goals are untouched.
inline bool ClearControllerDirectionFrame(void* control,size_t size){
    if(!control||size<0xb0)return false;
    auto* bytes=static_cast<unsigned char*>(control);uint32_t released=UINT32_MAX;
    for(size_t offset:{size_t(0x3c),size_t(0x40),size_t(0x44),size_t(0x48)})std::memcpy(bytes+offset,&released,sizeof(released));
    float zero[3]{};std::memcpy(bytes+0x60,zero,sizeof(zero));return true;
}
// A combat target can select target-relative native movement while the VR
// view still faces elsewhere. Scope controller walking to the visible camera;
// restore the native reference afterwards, unless the native handler changed it.
class ControllerReferenceLease {
    int previous=-1;
public:
    bool Active()const{return previous>=0;}
    bool Begin(bool owns,int current){if(!owns||current!=1)return false;previous=current;return true;}
    bool End(int current,int& restore){if(previous<0)return false;int saved=previous;previous=-1;
        if(current!=0)return false;restore=saved;return true;}
};
}
