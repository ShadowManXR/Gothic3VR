#pragma once
// Included inside the launcher's anonymous namespace, after its layout helpers.
void FillNativeIni(){
 bool before=syncing;syncing=true;
 for(unsigned i=0;i<diagnostic::ini_tweaks::Count;++i){auto const& f=diagnostic::ini_tweaks::Fields[i];bool enabled=nativeIni.ready&&nativeIni.available[i];
  if(f.kind==diagnostic::ini_tweaks::Kind::Number){
   SendMessageW(iniTracks[i],TBM_SETRANGEMIN,FALSE,f.minimum);SendMessageW(iniTracks[i],TBM_SETRANGEMAX,FALSE,f.maximum);SendMessageW(iniTracks[i],TBM_SETLINESIZE,0,f.step);SendMessageW(iniTracks[i],TBM_SETPAGESIZE,0,f.step);
   SendMessageW(iniTracks[i],TBM_SETPOS,TRUE,nativeIni.values[i]);SetDlgItemInt(window,IniValueBase+i,UINT(nativeIni.values[i]),TRUE);EnableWindow(iniTracks[i],enabled);EnableWindow(iniValues[i],enabled);
  }else if(f.kind==diagnostic::ini_tweaks::Kind::Choice){SendMessageW(iniValues[i],CB_SETCURSEL,nativeIni.values[i],0);EnableWindow(iniValues[i],enabled);}
  else{SendMessageW(iniValues[i],BM_SETCHECK,nativeIni.values[i]?BST_CHECKED:BST_UNCHECKED,0);EnableWindow(iniValues[i],enabled);}
  iniTooltips[i]=std::wstring(f.hint)+L"\nINI key: "+nativeIni.bindings[i].key;
  TOOLINFOW tip{};tip.cbSize=sizeof(tip);tip.hwnd=window;tip.uId=i+1;tip.lpszText=iniTooltips[i].data();SendMessageW(iniTooltipWindow,TTM_UPDATETIPTEXTW,0,LPARAM(&tip));
 }
 wchar_t const* tiers[]={L"Off",L"Low",L"Medium",L"High",L"Very high"};
 auto caption=nativeIni.ready?L"Active tiers: detail "+std::wstring(tiers[nativeIni.highTier])+L", distant "+tiers[nativeIni.lowTier]+L", vegetation "+tiers[nativeIni.vegetationTier]+L".\nDistances and ROI: 100 native units = 1 metre.":std::wstring(L"Native INIs are unavailable. Run the flat game once, select its executable, then Reload INIs.");
 SetWindowTextW(iniHint,caption.c_str());syncing=before;
}
void LoadNativeIni(){
 try{nativeIni=diagnostic::ini_tweaks::Load(std::filesystem::path(diagnostic::ParentDirectory(executable))/L"Ini/ge3.ini",OptionsIni());}
 catch(...){nativeIni={};}
 nativeIniDirty=false;FillNativeIni();
}
void CreateNativeIniControls(){
 iniTooltipWindow=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,0,0,0,0,window,nullptr,instance,nullptr);
 SetWindowPos(iniTooltipWindow,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
 SendMessageW(iniTooltipWindow,WM_SETFONT,WPARAM(font),FALSE);
 SendMessageW(iniTooltipWindow,TTM_SETMAXTIPWIDTH,0,Scale(520));SendMessageW(iniTooltipWindow,TTM_SETDELAYTIME,TTDT_AUTOPOP,15000);
 SendMessageW(iniTooltipWindow,TTM_SETDELAYTIME,TTDT_INITIAL,400);SendMessageW(iniTooltipWindow,TTM_SETDELAYTIME,TTDT_RESHOW,100);
 for(unsigned i=0;i<diagnostic::ini_tweaks::Count;++i){auto const& f=diagnostic::ini_tweaks::Fields[i];
  if(f.kind==diagnostic::ini_tweaks::Kind::Toggle)iniValues[i]=Control(L"BUTTON",f.label,WS_TABSTOP|BS_AUTOCHECKBOX,IniValueBase+i);
  else{
   iniLabels[i]=Control(L"STATIC",f.label,0);
   if(f.kind==diagnostic::ini_tweaks::Kind::Number){iniTracks[i]=Control(TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,IniTrackBase+i);iniValues[i]=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,IniValueBase+i);SendMessageW(iniValues[i],EM_SETLIMITTEXT,8,0);iniUnits[i]=Control(L"STATIC",i==6?L"FPS":L"units",0);}
   else{
    iniValues[i]=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,IniValueBase+i);
    if(i==9){for(auto text:{L"Linear",L"Anisotropic 2x",L"Anisotropic 4x",L"Anisotropic 8x",L"Anisotropic 16x"})SendMessageW(iniValues[i],CB_ADDSTRING,0,LPARAM(text));}
    else{for(int q=0;q<=f.maximum;++q){wchar_t const* choices[]={L"Off",L"Low",L"Medium",L"High",L"Very high"};SendMessageW(iniValues[i],CB_ADDSTRING,0,LPARAM(choices[q]));}}
   }
  }
  TOOLINFOW tip{};tip.cbSize=sizeof(tip);tip.uFlags=TTF_TRANSPARENT;tip.hwnd=window;tip.hinst=instance;tip.uId=i+1;tip.lpszText=const_cast<wchar_t*>(f.hint);SendMessageW(iniTooltipWindow,TTM_ADDTOOLW,0,LPARAM(&tip));
 }
 gameIniHint=Control(L"STATIC",L"Fog: 100% = original game fog; 0% = no fog. Fog applies live.\nDistances: 100 units = 1 metre. Higher values increase rendering cost.\nClose Gothic 3 before saving distance changes, then restart.\nDistant landscape distance must exceed high-detail distance.",0);
 iniHint=Control(L"STATIC",L"",0);iniHintRight=Control(L"STATIC",L"Restart Gothic 3 after applying changes.\nCP manages resource memory caches automatically.",0);
 iniReload=Control(L"BUTTON",L"Reload INIs",WS_TABSTOP,IniReload);iniBackups=Control(L"BUTTON",L"Open backups",WS_TABSTOP,IniBackups);
 LoadNativeIni();
}
bool CollectNativeIni(){
 if(!nativeIniDirty)return true;
 if(!nativeIni.ready){Message(L"Native INIs are unavailable. Run the flat game once and Reload INIs.",true);return false;}
 for(unsigned i=0;i<diagnostic::ini_tweaks::Count;++i)if(nativeIni.available[i]){auto const& f=diagnostic::ini_tweaks::Fields[i];int value=0;
  if(f.kind==diagnostic::ini_tweaks::Kind::Number){wchar_t text[64];GetWindowTextW(iniValues[i],text,64);wchar_t* end=nullptr;long n=wcstol(text,&end,10);
   if(end==text||*end||n<f.minimum||n>f.maximum){wchar_t error[256];swprintf_s(error,L"%s: enter a whole number between %d and %d.",f.label,f.minimum,f.maximum);Message(error,true);TabCtrl_SetCurSel(settingsTabs,diagnostic::ini_tweaks::GameIniField(i)?GameIniPage:IniTweaksPage);Layout();SetFocus(iniValues[i]);return false;}value=int(n);
  }else if(f.kind==diagnostic::ini_tweaks::Kind::Choice)value=int(SendMessageW(iniValues[i],CB_GETCURSEL,0,0));
  else value=SendMessageW(iniValues[i],BM_GETCHECK,0,0)==BST_CHECKED;
  nativeIni.values[i]=value;
 }
 return true;
}
void OpenIniBackups(){
 try{auto root=diagnostic::ini_tweaks::BackupRoot(std::filesystem::path(diagnostic::ParentDirectory(executable))/L"Ini/ge3.ini");
  if(root.empty()||!std::filesystem::is_directory(root)){Message(L"No INI backups yet. A backup is created when you save changed INI tweaks.");return;}
  if(INT_PTR(ShellExecuteW(window,L"open",root.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32)Message(L"Could not open the INI backup folder.",true);
 }catch(...){Message(L"Could not locate the INI backup folder.",true);}
}

void UpdateNativeTooltipRects(){
 if(!iniTooltipWindow)return;
 int page=TabCtrl_GetCurSel(settingsTabs);bool nativePage=page==IniTweaksPage||page==GameIniPage;
 if(!nativePage)SendMessageW(iniTooltipWindow,TTM_POP,0,0);
 for(unsigned i=0;i<diagnostic::ini_tweaks::Count;++i){
  RECT bounds{};bool first=true;
  if(nativePage)for(HWND control:{iniLabels[i],iniTracks[i],iniValues[i],iniUnits[i]})if(control&&(GetWindowLongPtrW(control,GWL_STYLE)&WS_VISIBLE)){
   RECT rect;GetWindowRect(control,&rect);MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&rect),2);
   if(first){bounds=rect;first=false;}else{RECT joined;UnionRect(&joined,&bounds,&rect);bounds=joined;}
  }
  TOOLINFOW tip{};tip.cbSize=sizeof(tip);tip.hwnd=window;tip.uId=i+1;tip.rect=bounds;SendMessageW(iniTooltipWindow,TTM_NEWTOOLRECTW,0,LPARAM(&tip));
 }
}
void RelayNativeTooltip(MSG const& input){
 if(!iniTooltipWindow||(TabCtrl_GetCurSel(settingsTabs)!=IniTweaksPage&&TabCtrl_GetCurSel(settingsTabs)!=GameIniPage)||input.message!=WM_MOUSEMOVE)return;
 if(input.hwnd!=window&&!IsChild(window,input.hwnd))return;
 MSG relayed=input;POINT p{short(LOWORD(input.lParam)),short(HIWORD(input.lParam))};
 MapWindowPoints(input.hwnd,window,&p,1);relayed.hwnd=window;relayed.lParam=MAKELPARAM(p.x,p.y);
 SendMessageW(iniTooltipWindow,TTM_RELAYEVENT,0,LPARAM(&relayed));
}
bool NativeTooltipCheck(std::wstring& report){
 NMCUSTOMDRAW tooltipDraw{};tooltipDraw.hdr.hwndFrom=iniTooltipWindow;tooltipDraw.dwDrawStage=CDDS_PREPAINT;
 if(DrawTrack(tooltipDraw)!=CDRF_DODEFAULT){report+=L"Tooltip rendering was intercepted by the slider painter.\r\n";return false;}
 if(!IsWindow(iniTooltipWindow)||SendMessageW(iniTooltipWindow,TTM_GETTOOLCOUNT,0,0)!=diagnostic::ini_tweaks::Count){report+=L"Native tooltip registration failed: "+std::to_wstring(SendMessageW(iniTooltipWindow,TTM_GETTOOLCOUNT,0,0))+L"\r\n";return false;}
 for(unsigned i=0;i<diagnostic::ini_tweaks::Count;++i){
  HWND control=iniLabels[i]?iniLabels[i]:iniValues[i];if(!IsWindowVisible(control))continue;RECT r;GetWindowRect(control,&r);MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&r),2);
  TTHITTESTINFOW hit{};hit.hwnd=window;hit.pt={(r.left+r.right)/2,(r.top+r.bottom)/2};hit.ti.cbSize=sizeof(hit.ti);
  if(!SendMessageW(iniTooltipWindow,TTM_HITTESTW,0,LPARAM(&hit))||hit.ti.uId!=i+1){report+=L"Native tooltip hit test failed: "+std::to_wstring(i)+L"\r\n";return false;}
  wchar_t text[2048]{};TOOLINFOW tip{};tip.cbSize=sizeof(tip);tip.hwnd=window;tip.uId=i+1;tip.lpszText=text;
  SendMessageW(iniTooltipWindow,TTM_GETTEXTW,2048,LPARAM(&tip));
  if(!wcsstr(text,diagnostic::ini_tweaks::Fields[i].hint)){report+=L"Native tooltip text failed: "+std::to_wstring(i)+L"\r\n";return false;}
 }
 return true;
}
