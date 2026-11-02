#pragma once
constexpr int MapperAction=720,MapperMode=721,MapperButton=722,MapperModifier=723,MapperListen=724,MapperClear=725,MapperDefaults=726,MapperTimer=927;
diagnostic::mapping::Link mappingLink;
bool mappingListening=false,mappingNeutral=false;uint64_t mappingDeadline=0;uint32_t mappingCaptured=0;int mappingSelected=0;
void MappingText(HWND h,std::wstring const& text){int n=GetWindowTextLengthW(h);std::wstring old(size_t(n)+1,L'\0');GetWindowTextW(h,old.data(),n+1);old.resize(n);if(old!=text)SetWindowTextW(h,text.c_str());}
std::wstring MappingButtons(uint32_t bits){std::wstring out;for(int i=1;i<diagnostic::mapping::ButtonCount;++i)if(bits&diagnostic::mapping::Bit(i)){if(!out.empty())out+=L" + ";out+=diagnostic::mapping::ButtonNames[i];}return out.empty()?L"None":out;}
void StopControllerCapture(){if(!mappingListening)return;mappingLink.Capture(0);mappingListening=false;mappingNeutral=false;mappingCaptured=0;if(mappingListen)SetWindowTextW(mappingListen,L"Listen in VR...");EnableWindow(mappingAction,TRUE);}
void ControllerMappingConflicts(){using namespace diagnostic::mapping;auto const& b=settings.controllerBindings.bindings[mappingSelected];std::wstring text=L"Current: ";text+=ButtonNames[b.button];if(b.modifier){text+=L" + ";text+=ButtonNames[b.modifier];}text+=L" / ";text+=GestureNames[b.gesture];
 for(int i=0;i<ActionCount;++i)if(i!=mappingSelected&&b.button&&settings.controllerBindings.bindings[i].button==b.button&&settings.controllerBindings.bindings[i].modifier==b.modifier&&(Contexts[i]&Contexts[mappingSelected])&&!(i==Support&&mappingSelected==ShieldBlock)&&!(i==ShieldBlock&&mappingSelected==Support)){text+=L"\nAlso assigned to: ";text+=ActionNames[i];text+=L". Both actions may activate in the same context.";break;}
 Telemetry sample;auto now=GetTickCount64();if(mappingLink.Read(sample)&&sample.focused&&sample.stamp<=now&&now-sample.stamp<1500&&sample.available&&(Mask(b)&sample.available)!=Mask(b)){text+=L"\nThis input is not currently reported by the connected controllers.";}
 MappingText(mappingStatus,text);
}
void FillControllerMapper(){using namespace diagnostic::mapping;if(!mappingAction)return;int selected=int(SendMessageW(mappingAction,CB_GETCURSEL,0,0));mappingSelected=std::clamp(selected,0,int(ActionCount)-1);auto const& b=settings.controllerBindings.bindings[mappingSelected];
 SendMessageW(mappingAction,CB_SETCURSEL,mappingSelected,0);SendMessageW(mappingMode,CB_SETCURSEL,b.gesture,0);SendMessageW(mappingButton,CB_SETCURSEL,b.button,0);SendMessageW(mappingModifier,CB_SETCURSEL,b.modifier,0);ControllerMappingConflicts();}
bool StoreControllerMapping(){using namespace diagnostic::mapping;Binding b{int(SendMessageW(mappingButton,CB_GETCURSEL,0,0)),int(SendMessageW(mappingModifier,CB_GETCURSEL,0,0)),int(SendMessageW(mappingMode,CB_GETCURSEL,0,0))};if(!b.button)b.modifier=None;
 if(!Valid(b)){Message(L"Choose two different buttons, or set Modifier to Unassigned.",true);FillControllerMapper();return false;}
 auto previous=settings.controllerBindings.bindings[mappingSelected];settings.controllerBindings.bindings[mappingSelected]=b;
 if(!settings.controllerBindings.bindings[Menu].button&&!settings.controllerBindings.bindings[RadialOpen].button){settings.controllerBindings.bindings[mappingSelected]=previous;Message(L"Keep Main menu or Open radial assigned so you can still leave gameplay.",true);FillControllerMapper();return false;}
 dirty=true;ControllerMappingConflicts();Message(L"Controller mapping changed. Save settings to apply live.");return true;
}
void CreateControllerMapper(){using namespace diagnostic::mapping;
 mappingLive=Control(L"STATIC",L"Start Gothic 3 in VR to detect controller input.",0);
 mappingActionLabel=Control(L"STATIC",L"Game action",0);mappingAction=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,MapperAction);
 for(auto text:ActionNames)SendMessageW(mappingAction,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));SendMessageW(mappingAction,CB_SETCURSEL,0,0);
 mappingModeLabel=Control(L"STATIC",L"Activation",0);mappingMode=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,MapperMode);for(auto text:GestureNames)SendMessageW(mappingMode,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
 mappingButtonLabel=Control(L"STATIC",L"Button",0);mappingButton=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,MapperButton);
 mappingModifierLabel=Control(L"STATIC",L"Modifier (optional)",0);mappingModifier=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,MapperModifier);
 for(auto text:ButtonNames){SendMessageW(mappingButton,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));SendMessageW(mappingModifier,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));}
 mappingListen=Control(L"BUTTON",L"Listen in VR...",WS_TABSTOP,MapperListen);mappingClear=Control(L"BUTTON",L"Clear action",WS_TABSTOP,MapperClear);mappingDefaults=Control(L"BUTTON",L"Restore controller defaults",WS_TABSTOP,MapperDefaults);
 mappingStatus=Control(L"STATIC",L"",0);mappingHint=Control(L"STATIC",L"Listen: return to the VR game, release all buttons, then press one button or hold two and release.\nDouble tap also works with a modifier. Use Hold for bow string / block / held support.\nSave settings applies live. Axes / tracking stay unchanged. System / Home buttons are reserved.",0);
}
void LayoutControllerMapper(int x,int y,int width,bool compact){
 auto show=[](HWND h,int a,int b,int c,int d){Position(h,a,b,c,d);ShowWindow(h,SW_SHOW);};
 show(mappingLive,x,y,width,40);int first=compact?52:64,gap=compact?38:44;
 HWND labels[]{mappingActionLabel,mappingModeLabel,mappingButtonLabel,mappingModifierLabel},choices[]{mappingAction,mappingMode,mappingButton,mappingModifier};
 for(int i=0;i<4;++i){show(labels[i],x,y+first+i*gap,180,23);show(choices[i],x+185,y+first+i*gap-4,width-185,i?310:420);}
 int buttons=y+(compact?212:252),bw=(width-24)/3;show(mappingListen,x,buttons,bw,34);show(mappingClear,x+bw+12,buttons,bw,34);show(mappingDefaults,x+2*(bw+12),buttons,bw,34);
 show(mappingStatus,x,y+(compact?260:302),width,compact?58:66);show(mappingHint,x,y+(compact?326:382),width,compact?64:84);
}
bool ControllerMapperCommand(int id,int code){using namespace diagnostic::mapping;
 if(id<MapperAction||id>MapperDefaults)return false;
 if(id==MapperAction&&code==CBN_SELCHANGE){StopControllerCapture();FillControllerMapper();return true;}
 if((id==MapperMode||id==MapperButton||id==MapperModifier)&&code==CBN_SELCHANGE&&!syncing){StopControllerCapture();StoreControllerMapping();return true;}
 if(code!=BN_CLICKED)return false;
 if(id==MapperListen){if(mappingListening){StopControllerCapture();ControllerMappingConflicts();return true;}
  Telemetry sample;if(!mappingLink.Open(Ini().c_str())||!mappingLink.Read(sample)||!sample.stamp||GetTickCount64()<sample.stamp||GetTickCount64()-sample.stamp>5000){Message(L"Start Gothic 3 in VR first, then open Controller mapping. Input is captured from the game's active OpenXR session.",true);return true;}
  mappingLink.ClearCaptured();mappingListening=true;mappingNeutral=false;mappingCaptured=0;mappingDeadline=GetTickCount64()+20000;SetWindowTextW(mappingListen,L"Cancel listening");EnableWindow(mappingAction,FALSE);MappingText(mappingStatus,L"Return to the VR game. Release all buttons, then press one button or hold two together and release. Game input is paused while listening.");return true;
 }
 if(id==MapperClear){StopControllerCapture();SendMessageW(mappingButton,CB_SETCURSEL,None,0);SendMessageW(mappingModifier,CB_SETCURSEL,None,0);StoreControllerMapping();return true;}
 if(id==MapperDefaults){StopControllerCapture();settings.controllerBindings=Config{};dirty=true;FillControllerMapper();Message(L"Controller defaults restored. Save settings to apply live.");return true;}return false;
}
void TickControllerMapper(){using namespace diagnostic::mapping;
 if(TabCtrl_GetCurSel(settingsTabs)!=BindingsPage){StopControllerCapture();return;}
 Telemetry sample;auto now=GetTickCount64();bool connected=mappingLink.Open(Ini().c_str())&&mappingLink.Read(sample)&&sample.stamp&&now>=sample.stamp&&now-sample.stamp<=1500;
 if(connected){std::wstring live=sample.focused?L"VR input connected. Pressed: ":L"VR session connected; return focus to the game. Last buttons: ";live+=MappingButtons(sample.buttons);live+=L"\nAvailable: ";
  static wchar_t const* shortNames[]={L"L trigger",L"L grip",L"L primary",L"L secondary",L"L stick",L"L menu",L"R trigger",L"R grip",L"R primary",L"R secondary",L"R stick",L"R menu"};
  bool first=true;for(int i=0;i<12;++i)if(sample.available&(1u<<i)){if(!first)live+=L", ";live+=shortNames[i];first=false;}MappingText(mappingLive,live);}
 else MappingText(mappingLive,L"Waiting for the game in VR. You can also choose buttons manually.");
 if(!mappingListening)return;
 if(now>=mappingDeadline){StopControllerCapture();MappingText(mappingStatus,L"Listening timed out. No binding was changed.");return;}
 mappingLink.Capture(now+1000);if(!connected)return;
 if(!mappingNeutral){if(sample.focused&&!sample.buttons){mappingNeutral=true;mappingLink.ClearCaptured();}return;}
 uint32_t observed=sample.captured?sample.captured:sample.buttons;
 if(observed){uint32_t combined=observed;unsigned count=0;for(auto m=combined;m;m&=m-1)++count;
  if(count>2){mappingCaptured=0;mappingNeutral=false;MappingText(mappingStatus,L"Use at most two buttons. Release all buttons and try again.");return;}
  mappingCaptured=combined;MappingText(mappingStatus,L"Detected: "+MappingButtons(mappingCaptured)+L". Release to assign.");if(sample.buttons)return;
 }
 if(mappingCaptured){int button=0,modifier=0;for(int i=1;i<ButtonCount;++i)if(mappingCaptured&Bit(i)){if(!button)button=i;else modifier=i;}
  StopControllerCapture();SendMessageW(mappingButton,CB_SETCURSEL,button,0);SendMessageW(mappingModifier,CB_SETCURSEL,modifier,0);StoreControllerMapping();
 }
}
