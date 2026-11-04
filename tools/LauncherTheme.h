// Launcher-only presentation. No rendering resources or hooks are added to Gothic 3.
#pragma once

constexpr int NavBase=500;
wchar_t const* PageNames[PageCount]={L"Camera & controls",L"HUD & menus",L"Interaction",L"Left hand",L"Right hand",L"1H melee weapon",L"Bow",L"2H melee",L"Staff / Halberd",L"Crossbow",L"Magic",L"Radial menu",L"Shield",L"Graphics",L"Foveated rendering",L"Optional mods",L"INI tweaks",L"Controller mapping",L"Hand HUD"};
wchar_t const* PageDescriptions[PageCount]={
 L"Your view into Myrtana. Calibrate the camera and choose how you play.",
 L"Keep the interface comfortable, readable and within reach.",
 L"Align the interaction point with the way you hold your controller.",
 L"Fine-tune the left palm position and controller orientation.",
 L"Fine-tune the right palm position and controller orientation.",
 L"Balance alignment, handling and physical combat for one-handed weapons.",
 L"Tune bow alignment, physical draw, steady aiming and palm contact.",
 L"Set up two-handed swords and axes for a secure, balanced grip.",
 L"Calibrate staff contact, support grip and physical handling.",
 L"Align your crossbow and support hand. Reload progression stays native.",
 L"Bring spells, runes and scrolls into your hands.",
 L"Your weapons, quick slots and menus at a glance.",
 L"Position the shield for a natural guard.",
 L"Choose scene resolution and sharpening for your headset.",
 L"Keep the center sharp while reducing peripheral shading cost.",
 L"Manage optional Union+ mods with protection for your VR installation.",
 L"Tune native visibility, world simulation and graphics. Restart required.",
 L"Assign OpenXR controller buttons, double taps and two-button combinations.",
 L"Independent displays on the lower arms. Offsets use each display's local axes."
};
HWND navigation[PageCount]{};
HFONT smallFont{},navFont{},brandFont{},sectionFont{};
HBRUSH cardBrush{},editBrush{};
ULONG_PTR gdiplusToken{};
IStream* artworkStream{};
Gdiplus::Bitmap* artwork{};
Gdiplus::RectF artworkBounds;
HBITMAP canvas{};int canvasWidth{},canvasHeight{};
bool checkingIdlePaint=false;unsigned idlePaintCount=0;
struct Panel {int x,y,w,h;std::wstring heading;};
std::vector<Panel> panels;
constexpr COLORREF Ink=RGB(237,232,221),Muted=RGB(154,158,166),Gold=RGB(223,174,98),Card=RGB(23,25,30),Field=RGB(34,37,43);

void ClearCanvas(){if(canvas){DeleteObject(canvas);canvas=nullptr;}}
void UiText(HDC dc,std::wstring const& text,int x,int y,int w,int h,HFONT f,COLORREF color,UINT flags=DT_LEFT|DT_SINGLELINE|DT_VCENTER){
 RECT r{Scale(x),Scale(y),Scale(x+w),Scale(y+h)};auto old=SelectObject(dc,f);SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);DrawTextW(dc,text.c_str(),-1,&r,flags|DT_NOPREFIX);SelectObject(dc,old);
}
void Rounded(Gdiplus::Graphics& g,Gdiplus::RectF r,float radius,Gdiplus::Color color,Gdiplus::Color border=Gdiplus::Color(0,0,0,0)){
 Gdiplus::GraphicsPath p;float d=radius*2;p.AddArc(r.X,r.Y,d,d,180,90);p.AddArc(r.GetRight()-d,r.Y,d,d,270,90);p.AddArc(r.GetRight()-d,r.GetBottom()-d,d,d,0,90);p.AddArc(r.X,r.GetBottom()-d,d,d,90,90);p.CloseFigure();Gdiplus::SolidBrush b(color);g.FillPath(&b,&p);if(border.GetA()){Gdiplus::Pen pen(border);g.DrawPath(&pen,&p);}
}
void PaintCanvas(HDC dc,bool present=true){
 RECT c;GetClientRect(window,&c);int w=MulDiv(c.right,96,int(dpi)),h=MulDiv(c.bottom,96,int(dpi));
 if(!canvas||canvasWidth!=c.right||canvasHeight!=c.bottom){
  ClearCanvas();canvasWidth=c.right;canvasHeight=c.bottom;canvas=CreateCompatibleBitmap(dc,c.right,c.bottom);auto mem=CreateCompatibleDC(dc);auto old=SelectObject(mem,canvas);
  {Gdiplus::Graphics g(mem);g.ScaleTransform(float(dpi)/96,float(dpi)/96);g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);g.Clear(Gdiplus::Color(255,12,14,18));
   if(artwork&&artworkBounds.Width>0&&artworkBounds.Height>0){float factor=std::max(float(w)/artworkBounds.Width,float(h)/artworkBounds.Height);float iw=artworkBounds.Width*factor,ih=artworkBounds.Height*factor;g.DrawImage(artwork,Gdiplus::RectF((w-iw)*.5f,(h-ih)*.5f,iw,ih),artworkBounds.X,artworkBounds.Y,artworkBounds.Width,artworkBounds.Height,Gdiplus::UnitPixel);}
   Gdiplus::SolidBrush veil(Gdiplus::Color(64,8,10,15));g.FillRectangle(&veil,0,0,w,h);
   Gdiplus::LinearGradientBrush fade(Gdiplus::Point(230,90),Gdiplus::Point(w,250),Gdiplus::Color(144,9,12,18),Gdiplus::Color(96,12,13,18));g.FillRectangle(&fade,230,0,w-230,250);
   // 230/255 is approximately 90% opacity, revealing a little of the art.
   Gdiplus::SolidBrush side(Gdiplus::Color(230,12,15,20));g.FillRectangle(&side,0,0,230,h);
   Gdiplus::Pen line(Gdiplus::Color(255,46,43,38));g.DrawLine(&line,230,0,230,h);
   for(auto const& p:panels)Rounded(g,Gdiplus::RectF(float(p.x),float(p.y),float(p.w),float(p.h)),10,Gdiplus::Color(230,23,25,30),Gdiplus::Color(255,55,53,49));
   Gdiplus::SolidBrush bottom(Gdiplus::Color(230,14,17,22));g.FillRectangle(&bottom,230,h-103,w-230,103);g.DrawLine(&line,230,h-103,w,h-103);
  }
  // One larger G3 VR emblem in the top-right header; no duplicate wordmark.
  auto icon=LoadImageW(instance,MAKEINTRESOURCEW(101),IMAGE_ICON,Scale(88),Scale(88),LR_SHARED);DrawIconEx(mem,Scale(w-118),Scale(8),HICON(icon),Scale(88),Scale(88),0,nullptr,DI_NORMAL);
  UiText(mem,L"SETTINGS",25,111,170,18,smallFont,Muted);
  UiText(mem,L"PC VR · OPENXR · v1.0.7",15,40,200,20,smallFont,Gold,DT_CENTER|DT_SINGLELINE|DT_VCENTER);
  for(auto const& p:panels)if(!p.heading.empty())UiText(mem,p.heading,p.x+22,p.y+15,p.w-44,22,sectionFont,Gold);
  UiText(mem,L"Controller mapping",25,h-83,180,24,navFont,Ink);UiText(mem,L"Customize buttons in Settings",25,h-57,180,22,smallFont,Muted);
  SelectObject(mem,old);DeleteDC(mem);
 }
 if(present){HDC mem=CreateCompatibleDC(dc);auto old=SelectObject(mem,canvas);BitBlt(dc,0,0,c.right,c.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteDC(mem);}
}
void BackgroundAt(HDC dc,HWND child){
 RECT r;GetClientRect(child,&r);POINT at{};MapWindowPoints(child,window,&at,1);if(canvas){auto mem=CreateCompatibleDC(dc);auto old=SelectObject(mem,canvas);BitBlt(dc,0,0,r.right,r.bottom,mem,at.x,at.y,SRCCOPY);SelectObject(mem,old);DeleteDC(mem);}else FillRect(dc,&r,cardBrush);
}
LRESULT CALLBACK ButtonProcedure(HWND h,UINT msg,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){
 if(msg==WM_ERASEBKGND){BackgroundAt(HDC(w),h);return 1;}
 if(msg==WM_MOUSEMOVE&&!GetPropW(h,L"Hot")){SetPropW(h,L"Hot",HANDLE(1));TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,h,0};TrackMouseEvent(&track);InvalidateRect(h,nullptr,FALSE);}
 if(msg==WM_MOUSELEAVE){RemovePropW(h,L"Hot");InvalidateRect(h,nullptr,FALSE);}
 if(msg==WM_SETCURSOR){SetCursor(LoadCursorW(nullptr,IDC_HAND));return TRUE;}
 if(msg==WM_NCDESTROY){RemovePropW(h,L"Hot");RemovePropW(h,L"Toggle");RemoveWindowSubclass(h,ButtonProcedure,2);}
 return DefSubclassProc(h,msg,w,l);
}
LRESULT CALLBACK CheckProcedure(HWND h,UINT msg,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){
 if(msg==BM_SETCHECK){SetPropW(h,L"Checked",HANDLE(INT_PTR(w)));InvalidateRect(h,nullptr,FALSE);return 0;}
 if(msg==BM_GETCHECK)return LRESULT(GetPropW(h,L"Checked"));
 if(msg==WM_NCDESTROY){RemovePropW(h,L"Checked");RemoveWindowSubclass(h,CheckProcedure,1);}
 return DefSubclassProc(h,msg,w,l);
}
LRESULT CALLBACK LabelProcedure(HWND h,UINT msg,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){
 if(msg==WM_ERASEBKGND){BackgroundAt(HDC(w),h);return 1;}
 if(msg==WM_PAINT||msg==WM_PRINTCLIENT){PAINTSTRUCT p{};HDC dc=msg==WM_PAINT?BeginPaint(h,&p):HDC(w);BackgroundAt(dc,h);RECT r;GetClientRect(h,&r);int length=GetWindowTextLengthW(h);std::wstring text(size_t(length)+1,L'\0');GetWindowTextW(h,text.data(),length+1);auto old=SelectObject(dc,HFONT(SendMessageW(h,WM_GETFONT,0,0)));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,h==subtitle||h==status||h==hint||h==handHint||h==swordHint||h==staffHint||h==bowHint||h==bowGripHint||h==magicHint||h==radialHint||h==shieldHint||h==crossbowHint||h==graphicsHint||h==ffrHint||h==unionHint||h==aimHint||h==twoHandHint||h==iniHint||h==iniHintRight?Muted:Ink);DrawTextW(dc,text.c_str(),length,&r,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);if(msg==WM_PAINT)EndPaint(h,&p);return 0;}
 // The native static procedure paints immediately in its default colors on SETTEXT.
 // Store the text without that paint; our subclass draws it in the composed frame.
 if(msg==WM_SETTEXT){auto result=DefWindowProcW(h,msg,w,l);InvalidateRect(h,nullptr,FALSE);return result;}
 if(msg==WM_NCDESTROY)RemoveWindowSubclass(h,LabelProcedure,1);
 return DefSubclassProc(h,msg,w,l);
}
LRESULT CALLBACK FieldProcedure(HWND h,UINT msg,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){
 if(msg==WM_ERASEBKGND){RECT r;GetClientRect(h,&r);FillRect(HDC(w),&r,editBrush);return 1;}
 if(msg==WM_NCDESTROY)RemoveWindowSubclass(h,FieldProcedure,3);
 return DefSubclassProc(h,msg,w,l);
}
void PaintTrack(HWND,HDC);
LRESULT CALLBACK SliderProcedure(HWND h,UINT msg,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){
 if(msg==WM_ERASEBKGND){BackgroundAt(HDC(w),h);return 1;}
 // Let the native control finish its own paint lifecycle. Invalidating inside
 // WM_PAINT keeps the composed window dirty and causes an endless redraw loop.
 if(msg==WM_PAINT&&checkingIdlePaint)++idlePaintCount;
 if(msg==WM_PRINTCLIENT){PaintTrack(h,HDC(w));return 0;}
 if(msg==WM_NCDESTROY)RemoveWindowSubclass(h,SliderProcedure,4);
 auto result=DefSubclassProc(h,msg,w,l);
 if((msg==TBM_SETPOS&&w)||msg==WM_LBUTTONDOWN||msg==WM_LBUTTONUP||(msg==WM_MOUSEMOVE&&GetCapture()==h)||msg==WM_KEYDOWN||msg==WM_MOUSEWHEEL||msg==WM_ENABLE||msg==WM_SETFOCUS||msg==WM_KILLFOCUS)InvalidateRect(h,nullptr,FALSE);
 return result;
}
void StyleControl(HWND h,wchar_t const* cls,DWORD style){
 if(!wcscmp(cls,L"EDIT")||!wcscmp(cls,L"COMBOBOX"))SetWindowSubclass(h,FieldProcedure,3,0);
 if(!wcscmp(cls,TRACKBAR_CLASSW))SetWindowSubclass(h,SliderProcedure,4,0);
 if(!wcscmp(cls,L"STATIC"))SetWindowSubclass(h,LabelProcedure,1,0);
 if(!wcscmp(cls,L"BUTTON")){
  SetWindowSubclass(h,ButtonProcedure,2,0);
  if((style&BS_TYPEMASK)==BS_AUTOCHECKBOX){SetPropW(h,L"Toggle",HANDLE(1));SetWindowSubclass(h,CheckProcedure,1,0);}
  SetWindowLongPtrW(h,GWL_STYLE,(GetWindowLongPtrW(h,GWL_STYLE)&~BS_TYPEMASK)|BS_OWNERDRAW);
 }
 if(!wcscmp(cls,L"EDIT")){SetWindowLongPtrW(h,GWL_EXSTYLE,GetWindowLongPtrW(h,GWL_EXSTYLE)&~WS_EX_CLIENTEDGE);SetWindowLongPtrW(h,GWL_STYLE,GetWindowLongPtrW(h,GWL_STYLE)|WS_BORDER);SetWindowTheme(h,L"",L"");}
 if(!wcscmp(cls,TRACKBAR_CLASSW))SetWindowTheme(h,L"",L"");
 if(!wcscmp(cls,L"COMBOBOX")){SetWindowTheme(h,L"DarkMode_CFD",L"");COMBOBOXINFO combo{sizeof(combo)};if(GetComboBoxInfo(h,&combo)&&combo.hwndList)SetWindowSubclass(combo.hwndList,FieldProcedure,3,0);}
}
void DrawButton(DRAWITEMSTRUCT const& d){
 HDC dc=d.hDC;BackgroundAt(dc,d.hwndItem);RECT r=d.rcItem;bool down=(d.itemState&ODS_SELECTED)!=0,focus=(d.itemState&ODS_FOCUS)!=0,disabled=(d.itemState&ODS_DISABLED)!=0;
 wchar_t text[256];GetWindowTextW(d.hwndItem,text,256);int id=GetDlgCtrlID(d.hwndItem);bool nav=id>=NavBase&&id<NavBase+PageCount;bool selected=nav&&TabCtrl_GetCurSel(settingsTabs)==id-NavBase;bool check=GetPropW(d.hwndItem,L"Toggle")!=nullptr;
 auto oldFont=SelectObject(dc,nav?navFont:font);SetBkMode(dc,TRANSPARENT);
 if(check){
  bool checked=SendMessageW(d.hwndItem,BM_GETCHECK,0,0)==BST_CHECKED;RECT box{r.left+Scale(2),r.top+(r.bottom-r.top-Scale(17))/2,r.left+Scale(19),r.top+(r.bottom-r.top+Scale(17))/2};auto b=CreateSolidBrush(checked?Gold:Field);FillRect(dc,&box,b);DeleteObject(b);auto pen=CreatePen(PS_SOLID,1,checked?Gold:RGB(98,101,109));auto oldPen=SelectObject(dc,pen);auto oldB=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,box.left,box.top,box.right,box.bottom);SelectObject(dc,oldB);SelectObject(dc,oldPen);DeleteObject(pen);
  if(checked){pen=CreatePen(PS_SOLID,Scale(2),RGB(25,23,20));oldPen=SelectObject(dc,pen);MoveToEx(dc,box.left+Scale(4),box.top+Scale(8),nullptr);LineTo(dc,box.left+Scale(7),box.top+Scale(11));LineTo(dc,box.left+Scale(13),box.top+Scale(5));SelectObject(dc,oldPen);DeleteObject(pen);}r.left+=Scale(30);
 }else{
  Gdiplus::Graphics g(dc);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);bool primary=id==Launch,support=id==Donate,hot=GetPropW(d.hwndItem,L"Hot")!=nullptr;Gdiplus::Color fill=primary?Gdiplus::Color(255,down?185:220,down?132:169,89):selected?Gdiplus::Color(255,49,42,33):Gdiplus::Color(255,down?48:30,down?47:33,down?47:40);
  if(support)fill=down?Gdiplus::Color(255,65,42,27):hot?Gdiplus::Color(255,62,43,31):Gdiplus::Color(255,43,31,25);
  if(nav&&!selected)fill=GetPropW(d.hwndItem,L"Hot")?Gdiplus::Color(180,29,31,36):Gdiplus::Color(0,12,15,20);
  Rounded(g,Gdiplus::RectF(float(r.left+1),float(r.top+1),float(r.right-r.left-2),float(r.bottom-r.top-2)),float(Scale(6)),fill,support?Gdiplus::Color(255,hot?235:190,hot?188:141,hot?119:77):selected?Gdiplus::Color(255,105,80,46):nav?Gdiplus::Color(0,0,0,0):Gdiplus::Color(255,71,67,59));
  if(support){
   float x=float(r.left+Scale(40)),y=float((r.top+r.bottom)/2-Scale(7)),s=float(Scale(16));
   Gdiplus::GraphicsPath heart;heart.StartFigure();heart.AddBezier(x+s*.5f,y+s*.22f,x-s*.05f,y-s*.25f,x-s*.25f,y+s*.4f,x+s*.5f,y+s);
   heart.AddBezier(x+s*.5f,y+s,x+s*1.25f,y+s*.4f,x+s*1.05f,y-s*.25f,x+s*.5f,y+s*.22f);heart.CloseFigure();
   Gdiplus::SolidBrush accent(Gdiplus::Color(255,hot?243:221,hot?151:119,hot?124:92));g.FillPath(&accent,&heart);r.left+=Scale(24);
  }
  if(nav){wchar_t number[8];swprintf_s(number,L"%02d",id-NavBase+1);auto f=SelectObject(dc,smallFont);SetTextColor(dc,selected?Gold:RGB(96,103,112));RECT n=r;n.left+=Scale(13);n.right=n.left+Scale(26);DrawTextW(dc,number,-1,&n,DT_LEFT|DT_SINGLELINE|DT_VCENTER);SelectObject(dc,f);r.left+=Scale(46);}
 }
 SetTextColor(dc,disabled?RGB(99,103,111):id==Launch?RGB(25,22,18):id==Donate?RGB(248,220,178):selected?Gold:Ink);
 DrawTextW(dc,text,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX|(nav||check?DT_LEFT:DT_CENTER));
 if(focus){RECT f=d.rcItem;InflateRect(&f,-Scale(4),-Scale(4));DrawFocusRect(dc,&f);}SelectObject(dc,oldFont);
}
void PaintTrack(HWND h,HDC target){
 RECT channel{},thumb{},client{};GetClientRect(h,&client);if(client.right<=0||client.bottom<=0)return;
 HDC dc=CreateCompatibleDC(target);HBITMAP bitmap=CreateCompatibleBitmap(target,client.right,client.bottom);auto old=SelectObject(dc,bitmap);
 BackgroundAt(dc,h);SendMessageW(h,TBM_GETCHANNELRECT,0,LPARAM(&channel));SendMessageW(h,TBM_GETTHUMBRECT,0,LPARAM(&thumb));
 int mid=(client.top+client.bottom)/2;auto b=CreateSolidBrush(RGB(68,69,73));RECT bar{channel.left,mid-Scale(1),channel.right,mid+Scale(2)};FillRect(dc,&bar,b);DeleteObject(b);
 bool enabled=IsWindowEnabled(h)!=FALSE;bar.right=(thumb.left+thumb.right)/2;b=CreateSolidBrush(enabled?Gold:Muted);FillRect(dc,&bar,b);DeleteObject(b);
 {Gdiplus::Graphics g(dc);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);Gdiplus::SolidBrush dot(enabled?Gdiplus::Color(255,223,174,98):Gdiplus::Color(255,100,104,112));float size=float(Scale(11));g.FillEllipse(&dot,float(thumb.left+thumb.right)*.5f-size*.5f,float(mid)-size*.5f,size,size);}
 BitBlt(target,0,0,client.right,client.bottom,dc,0,0,SRCCOPY);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
}
LRESULT DrawTrack(NMCUSTOMDRAW const& d){
 // Tooltips also send NM_CUSTOMDRAW. Only sliders use our custom painter;
 // suppressing the default tooltip painter makes its explanation disappear.
 wchar_t cls[64]{};GetClassNameW(d.hdr.hwndFrom,cls,64);
 if(_wcsicmp(cls,TRACKBAR_CLASSW))return CDRF_DODEFAULT;
 if(d.dwDrawStage!=CDDS_PREPAINT)return CDRF_DODEFAULT;
 PaintTrack(d.hdr.hwndFrom,d.hdc);return CDRF_SKIPDEFAULT;
}
void InitializeArtwork(){
 Gdiplus::GdiplusStartupInput input;Gdiplus::GdiplusStartup(&gdiplusToken,&input,nullptr);cardBrush=CreateSolidBrush(Card);editBrush=CreateSolidBrush(Field);
 auto resource=FindResourceW(instance,MAKEINTRESOURCEW(102),RT_RCDATA);if(!resource)return;auto size=SizeofResource(instance,resource);auto data=LockResource(LoadResource(instance,resource));auto global=GlobalAlloc(GMEM_MOVEABLE,size);if(!global)return;auto p=GlobalLock(global);memcpy(p,data,size);GlobalUnlock(global);
 if(SUCCEEDED(CreateStreamOnHGlobal(global,TRUE,&artworkStream))){artwork=Gdiplus::Bitmap::FromStream(artworkStream);if(artwork->GetLastStatus()!=Gdiplus::Ok){delete artwork;artwork=nullptr;}}else GlobalFree(global);
 if(artwork){
  int width=int(artwork->GetWidth()),height=int(artwork->GetHeight());artworkBounds=Gdiplus::RectF(0,0,float(width),float(height));
  // The supplied image has transparent atlas padding beneath its 4:3 art.
  // Ignore only that padding for aspect-preserving cover; retain the PNG
  // byte-for-byte and do this bounded alpha scan once at launcher startup.
  Gdiplus::Rect area(0,0,width,height);Gdiplus::BitmapData pixels{};
  if(artwork->LockBits(&area,Gdiplus::ImageLockModeRead,PixelFormat32bppARGB,&pixels)==Gdiplus::Ok){
   int left=width,top=height,right=0,bottom=0;
   for(int y=0;y<height;++y){auto* row=static_cast<unsigned char*>(pixels.Scan0)+y*pixels.Stride;
    for(int x=0;x<width;++x)if(row[x*4+3]>=4){left=std::min(left,x);top=std::min(top,y);right=std::max(right,x+1);bottom=std::max(bottom,y+1);}}
   artwork->UnlockBits(&pixels);if(right>left&&bottom>top)artworkBounds=Gdiplus::RectF(float(left),float(top),float(right-left),float(bottom-top));
  }
 }
}
void ReleaseArtwork(){ClearCanvas();delete artwork;artwork=nullptr;if(artworkStream)artworkStream->Release();DeleteObject(cardBrush);DeleteObject(editBrush);if(gdiplusToken)Gdiplus::GdiplusShutdown(gdiplusToken);}
