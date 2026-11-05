#pragma once
#include <windows.h>
#include <string>
#include <cwchar>
#include "ControllerMapping.h"
namespace diagnostic::mapping {
inline Config Load(wchar_t const* path){
 Config out;out.leftHanded=GetPrivateProfileIntW(L"ControllerBindings",L"LeftHanded",0,path)!=0;
 for(unsigned i=0;i<ActionCount;++i){auto& b=out.bindings[i];
  auto read=[&](wchar_t const* suffix,int fallback){std::wstring key=std::wstring(Keys[i])+suffix;return int(GetPrivateProfileIntW(L"ControllerBindings",key.c_str(),fallback,path));};
  Binding candidate{read(L".Button",b.button),read(L".Modifier",b.modifier),read(L".Gesture",b.gesture)};
  if(Valid(candidate))b=candidate;
 }return out;
}
inline bool Save(wchar_t const* path,Config const& config){
 for(unsigned i=0;i<ActionCount;++i){auto const& b=config.bindings[i];if(!Valid(b))return false;
  auto write=[&](wchar_t const* suffix,int value){std::wstring key=std::wstring(Keys[i])+suffix;wchar_t text[24];swprintf_s(text,L"%d",value);return WritePrivateProfileStringW(L"ControllerBindings",key.c_str(),text,path)!=FALSE;};
  if(!write(L".Button",b.button)||!write(L".Modifier",b.modifier)||!write(L".Gesture",b.gesture))return false;
 }return WritePrivateProfileStringW(L"ControllerBindings",L"LeftHanded",config.leftHanded?L"1":L"0",path)!=FALSE;
}
}
