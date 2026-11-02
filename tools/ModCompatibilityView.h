// Launcher documentation only: no game hooks, installation or setting writes.
#pragma once
HWND modNotesWindow{},modNotesSearch{},modNotesFilter{},modNotesText{},modNotesSummary{},modNotesClose{},modNotesTip{};
constexpr int ModNotesSearch=901,ModNotesFilter=902;

std::wstring FoldModText(std::wstring text){if(!text.empty())CharLowerBuffW(text.data(),DWORD(text.size()));return text;}
void RefreshModNotes(){
 if(!modNotesText)return;
 wchar_t query[256]{};GetWindowTextW(modNotesSearch,query,256);auto needle=FoldModText(query);
 int filter=int(SendMessageW(modNotesFilter,CB_GETCURSEL,0,0));std::wstring body;unsigned shown=0;
 for(auto const& entry:diagnostic::mod_guidance::Entries){
  std::wstring state=entry.status;
  bool accepted=filter==0?(state==L"Avoid"||state==L"Baseline"):filter==1?state==L"Check":filter==2?state==L"Performance":filter==3?state==L"Incomplete":true;
  auto text=std::wstring(entry.name)+L"\r\n"+entry.folder+L"\r\n["+entry.status+L"] "+entry.reason+L"\r\n\r\n";
  if(accepted&&(needle.empty()||FoldModText(text).find(needle)!=std::wstring::npos)){body+=text;++shown;}
 }
 if(body.empty())body=L"No matching entries. Clear Search or choose another category.";
 SendMessageW(modNotesText,WM_SETREDRAW,FALSE,0);SetWindowTextW(modNotesText,body.c_str());SendMessageW(modNotesText,EM_SETSEL,0,0);SendMessageW(modNotesText,EM_SCROLLCARET,0,0);SendMessageW(modNotesText,WM_SETREDRAW,TRUE,0);InvalidateRect(modNotesText,nullptr,TRUE);
 auto summary=std::to_wstring(shown)+L" entries shown / 192 reviewed. Static review; individual runtime failures are not established.\r\nAvoid = camera/time overlap. Baseline = unvalidated player/combat overlap. Other categories are cautions.";
 SetWindowTextW(modNotesSummary,summary.c_str());
}
void LayoutModNotes(HWND h){
 RECT r{};GetClientRect(h,&r);int m=Scale(18),w=r.right-2*m;
 MoveWindow(GetDlgItem(h,903),m,m,w,Scale(78),TRUE);
 MoveWindow(modNotesFilter,m,Scale(108),w/2-Scale(8),Scale(240),TRUE);
 MoveWindow(modNotesSearch,m+w/2+Scale(8),Scale(108),w/2-Scale(8),Scale(29),TRUE);
 MoveWindow(modNotesSummary,m,Scale(151),w,Scale(55),TRUE);
 MoveWindow(modNotesText,m,Scale(217),w,std::max(1,int(r.bottom)-Scale(285)),TRUE);
 MoveWindow(modNotesClose,r.right-m-Scale(112),r.bottom-Scale(52),Scale(112),Scale(34),TRUE);
}
LRESULT CALLBACK ModNotesProcedure(HWND h,UINT message,WPARAM w,LPARAM l){
 switch(message){
 case WM_CREATE:{
  auto make=[&](wchar_t const* cls,wchar_t const* text,DWORD flags,int id){auto child=CreateWindowExW(!wcscmp(cls,L"EDIT")?WS_EX_CLIENTEDGE:0,cls,text,WS_CHILD|WS_VISIBLE|flags,0,0,0,0,h,reinterpret_cast<HMENU>(INT_PTR(id)),instance,nullptr);SendMessageW(child,WM_SETFONT,WPARAM(font),FALSE);return child;};
  make(L"STATIC",L"VR mod compatibility - Union+ 1.7.3\r\nLocal descriptions, dependencies and payload/configuration inventory reviewed (2026-10-25).\r\nKeep mandatory dependencies and patches with their parent mods. These notes do not change installed mods.",0,903);
  modNotesFilter=make(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL,ModNotesFilter);
  for(auto text:{L"Not recommended for VR baseline",L"Targeted compatibility checks",L"Performance cautions",L"Incomplete / documentation only",L"All 192 folders"})SendMessageW(modNotesFilter,CB_ADDSTRING,0,LPARAM(text));
  SendMessageW(modNotesFilter,CB_SETCURSEL,0,0);
  modNotesSearch=make(L"EDIT",L"",ES_AUTOHSCROLL|WS_TABSTOP,ModNotesSearch);SendMessageW(modNotesSearch,EM_SETCUEBANNER,TRUE,LPARAM(L"Search mod name, folder or reason"));SendMessageW(modNotesSearch,EM_SETLIMITTEXT,255,0);
  modNotesSummary=make(L"STATIC",L"",0,904);
  modNotesText=make(L"EDIT",L"",ES_READONLY|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL|WS_TABSTOP,905);SendMessageW(modNotesText,EM_SETLIMITTEXT,500000,0);
  modNotesClose=make(L"BUTTON",L"Close",BS_DEFPUSHBUTTON|WS_TABSTOP,IDCANCEL);
  LayoutModNotes(h);RefreshModNotes();return 0;}
 case WM_SIZE:LayoutModNotes(h);return 0;
 case WM_GETMINMAXINFO:reinterpret_cast<MINMAXINFO*>(l)->ptMinTrackSize={Scale(720),Scale(550)};return 0;
 case WM_COMMAND:
  if(LOWORD(w)==IDCANCEL){DestroyWindow(h);return 0;}
  if((LOWORD(w)==ModNotesSearch&&HIWORD(w)==EN_CHANGE)||(LOWORD(w)==ModNotesFilter&&HIWORD(w)==CBN_SELCHANGE)){RefreshModNotes();return 0;}break;
 case WM_CTLCOLOREDIT:case WM_CTLCOLORLISTBOX:SetTextColor(HDC(w),Ink);SetBkColor(HDC(w),Field);return LRESULT(editBrush);
 case WM_CTLCOLORSTATIC:SetTextColor(HDC(w),Ink);SetBkColor(HDC(w),HWND(l)==modNotesText?Field:Card);return LRESULT(HWND(l)==modNotesText?editBrush:cardBrush);
 case WM_CLOSE:DestroyWindow(h);return 0;
 case WM_DESTROY:modNotesWindow=nullptr;modNotesText=nullptr;SetFocus(unionNotes);return 0;
 }
 return DefWindowProcW(h,message,w,l);
}
void OpenModNotes(){
 if(IsWindow(modNotesWindow)){ShowWindow(modNotesWindow,SW_RESTORE);SetForegroundWindow(modNotesWindow);return;}
 WNDCLASSW cls{};cls.hInstance=instance;cls.lpfnWndProc=ModNotesProcedure;cls.lpszClassName=L"Gothic3VRModNotes";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=cardBrush;cls.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(101));RegisterClassW(&cls);
 RECT owner{},area{};GetWindowRect(window,&owner);MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor);area=monitor.rcWork;
 int width=std::min(Scale(1000),int(area.right-area.left)),height=std::min(Scale(830),int(area.bottom-area.top));
 int x=std::max(int(area.left),std::min(int(owner.left)+(int(owner.right-owner.left)-width)/2,int(area.right)-width)),y=std::max(int(area.top),std::min(int(owner.top)+(int(owner.bottom-owner.top)-height)/2,int(area.bottom)-height));
 modNotesWindow=CreateWindowExW(0,cls.lpszClassName,L"Gothic 3 VR - Mod compatibility",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,x,y,width,height,window,nullptr,instance,nullptr);
 if(modNotesWindow){ShowWindow(modNotesWindow,SW_SHOW);SetFocus(modNotesSearch);}
}
void CreateModNotesTooltip(){
 modNotesTip=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,window,nullptr,instance,nullptr);
 if(!modNotesTip)return;
 TOOLINFOW tool{sizeof(tool)};tool.uFlags=TTF_IDISHWND|TTF_SUBCLASS;tool.hwnd=window;tool.uId=UINT_PTR(unionNotes);tool.lpszText=const_cast<wchar_t*>(diagnostic::mod_guidance::Tooltip);SendMessageW(modNotesTip,TTM_ADDTOOLW,0,LPARAM(&tool));
 SendMessageW(modNotesTip,TTM_SETMAXTIPWIDTH,0,Scale(560));SendMessageW(modNotesTip,TTM_SETDELAYTIME,TTDT_AUTOPOP,20000);
}
