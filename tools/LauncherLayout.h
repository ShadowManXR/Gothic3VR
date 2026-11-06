#pragma once

void Layout(){
 RECT client;GetClientRect(window,&client);int w=MulDiv(client.right,96,int(dpi)),h=MulDiv(client.bottom,96,int(dpi));if(!title||w<700||h<500)return;
 // Do not expose the intermediate hide/move/show operations as individual frames.
 // Only suspend a window that was visible, so WM_CREATE stays hidden until ShowWindow.
 bool wasVisible=(GetWindowLongPtrW(window,GWL_STYLE)&WS_VISIBLE)!=0;
 if(wasVisible)SendMessageW(window,WM_SETREDRAW,FALSE,0);
 bool compact=h<840;int x=260,cw=w-x-30,top=compact?224:260,bottom=h-120,ch=bottom-top,col=(cw-18)/2;
 int page=TabCtrl_GetCurSel(settingsTabs);if(page<0)page=0;
 // The tab control remains the page model; the vertical buttons are its visible, keyboard-accessible navigation.
 Position(settingsTabs,0,0,1,1);ShowWindow(settingsTabs,SW_HIDE);ShowWindow(unionShortcut,SW_HIDE);
 Position(title,x,compact?24:43,cw-116,46);SetWindowTextW(title,PageNames[page]);Position(subtitle,x,compact?72:92,cw-116,25);SetWindowTextW(subtitle,PageDescriptions[page]);
 int navStep=std::min(compact?28:31,(h-250)/PageCount);
 for(int p=0;p<PageCount;++p){int y=140+p*navStep;Position(navigation[p],14,y,202,navStep-1);InvalidateRect(navigation[p],nullptr,FALSE);}
 Position(pathLabel,x+22,compact?112:139,210,23);Position(pathEdit,x+22,compact?137:164,cw-142,27);Position(browse,x+cw-109,compact?135:162,87,31);
 Position(resolutionLabel,x+22,compact?177:207,190,22);Position(resolutionCombo,x+213,compact?172:202,cw-235,280);ShowWindow(resolutionHint,SW_HIDE);
 panels.clear();panels.push_back({x,compact?103:126,cw,compact?107:115,L""});
 bool dual=page==OneHandPage||page==TwoHandPage||page==StaffPage;
 if(page==IniTweaksPage){panels.push_back({x,top,col,ch,L"STREAMING"});panels.push_back({x+col+18,top,col,ch,L"WORLD & GRAPHICS"});}
 else if(page==GameIniPage)panels.push_back({x,top,cw,ch,L"FOG & DRAW DISTANCE"});
 else if(page==HandHudPage){panels.push_back({x,top,col,ch,L"LEFT ARM - SLOT BAR"});panels.push_back({x+col+18,top,col,ch,L"RIGHT ARM - HEALTH / MANA / STAMINA"});}
 else if(page==ModsPage)panels.push_back({x,top,cw,ch,L"UNION+ 1.7.3"});
 else if(dual){panels.push_back({x,top,col,ch,L"ALIGNMENT & HANDLING"});panels.push_back({x+col+18,top,col,ch,L"COMBAT THRESHOLDS"});}
 else if(page==BowPage){panels.push_back({x,top,col,ch,L"ALIGNMENT & DRAW"});panels.push_back({x+col+18,top,col,ch,L"PALM GRIP"});}
 else if(page==CameraPage){panels.push_back({x,top,col,ch,L"CAMERA POSITION"});panels.push_back({x+col+18,top,col,ch,L"VR RUNTIME & CONTROLS"});}
 else panels.push_back({x,top,cw,ch,page==ModsPage?L"OPTIONAL MODS":page==BindingsPage?L"CONTROLLER BINDINGS":page==GraphicsPage||page==FoveatedPage?L"RENDERING":page==RadialPage?L"CONTROLS":page==HudPage?L"INTERFACE":L"CALIBRATION"});
 for(unsigned i=0;i<diagnostic::VrNumberCount;++i)for(HWND control:{labels[i],tracks[i],edits[i],units[i]})ShowWindow(control,SW_HIDE);
 for(HWND control:{fastPickupCheck,dialogueViewCheck,chestViewCheck,lecternViewCheck,thirdPersonOrbitCheck,thirdPersonHint,handHudCheck,compassCheck,slotBarCheck,vitalsPanelCheck,runtimeLabel,runtimeCombo,handsOnlyCheck,toggleSupportCheck,ffrLabel,ffrCombo,ffrCenterLabel,ffrTrack,ffrEdit,ffrUnit,ffrHint,ffrTransitionLabel,ffrTransitionTrack,ffrTransitionEdit,ffrTransitionUnit,graphicsLabel,graphicsCombo,graphicsPreview,graphicsHint,aaLabel,aaCombo,effectsLabel,effectsCombo,unionLabel,unionPath,unionBrowse,unionOpen,unionCheck,unionNotes,unionHint,handHint,swordHint,bowHint,bowGripHint,twoHandCheck,heavyCheck,twoHandHint,staffHint,radialHint,shieldHint,magicHint,crossbowHint,bowReticleCheck,magicReticleCheck,aimLabel,aimTrack,aimEdit,aimUnit,aimHint,hint})ShowWindow(control,SW_HIDE);
 for(unsigned i=0;i<diagnostic::ini_tweaks::Count;++i)for(HWND control:{iniLabels[i],iniTracks[i],iniValues[i],iniUnits[i]})if(control)ShowWindow(control,SW_HIDE);
 for(HWND control:{gameIniHint,iniHint,iniHintRight,iniReload,iniBackups})if(control)ShowWindow(control,SW_HIDE);
 for(int i=0;i<2;++i)for(HWND control:{graphicsLabels[i],graphicsTracks[i],graphicsEdits[i],graphicsUnits[i]})ShowWindow(control,SW_HIDE);
 for(HWND control:{mappingLeftHanded,mappingJump,mappingLive,mappingActionLabel,mappingAction,mappingModeLabel,mappingMode,mappingButtonLabel,mappingButton,mappingModifierLabel,mappingModifier,mappingListen,mappingClear,mappingDefaults,mappingStatus,mappingHint})if(control)ShowWindow(control,SW_HIDE);
 auto show=[](HWND control,int a,int b,int c,int d){Position(control,a,b,c,d);ShowWindow(control,SW_SHOW);};
 auto field=[&](unsigned i,int fx,int fy,int width){
  show(labels[i],fx,fy,width-110,21);show(edits[i],fx+width-99,fy-2,58,27);show(units[i],fx+width-35,fy+1,40,23);show(tracks[i],fx,fy+23,width-110,18);
 };
 int fx=x+22,fw=cw-44,fy=top+47;
 if(dual){
  std::vector<unsigned> left,right;
  if(page==OneHandPage){left={20,21,22,73,74};right={52,53,64,67,70};}
  if(page==TwoHandPage){left={32,33,34,35,82,75,76};right={54,55,65,68,71};}
  if(page==StaffPage){left={36,37,38,39,83,40,41,77,78};right={56,57,66,69,72,79,80};}
  int spacing=page==StaffPage?(compact?42:47):page==TwoHandPage&&compact?47:53;
  for(unsigned row=0;row<left.size();++row)field(left[row],fx,fy+row*spacing,col-44);
  for(unsigned row=0;row<right.size();++row)field(right[row],x+col+40,fy+row*spacing,col-44);
  if(page==OneHandPage)show(swordHint,fx,fy+left.size()*spacing+12,col-44,86);
  if(page==TwoHandPage){show(twoHandCheck,fx,fy+left.size()*spacing+8,col-44,29);show(heavyCheck,x+col+40,fy+right.size()*spacing+18,col-44,45);show(twoHandHint,x+col+40,fy+right.size()*spacing+74,col-44,45);}
  if(page==StaffPage){show(twoHandCheck,x+col+40,fy+right.size()*spacing+8,col-44,29);show(staffHint,x+col+40,fy+right.size()*spacing+40,col-44,44);}
 }else if(page==HandHudPage){
  int spacing=compact?45:50;
  for(unsigned row=0;row<7;++row){field(row<6?87+row:99,fx,fy+row*spacing,col-44);field(row<6?93+row:100,x+col+40,fy+row*spacing,col-44);}
  show(handHudCheck,fx,fy+7*spacing+8,col-44,29);
 }else if(page==CameraPage){
  unsigned cameraFields[]{0,6,7,diagnostic::FirstPersonNearClipIndex};
  for(unsigned row=0;row<4;++row)field(cameraFields[row],fx,fy+row*72,col-44);
  int rx=x+col+40;
  show(runtimeLabel,rx,fy,col-44,22);show(runtimeCombo,rx,fy+26,col-44,180);
  show(handsOnlyCheck,rx,fy+75,col-44,29);
  show(toggleSupportCheck,rx,fy+109,col-44,29);
  show(hint,rx,fy+158,col-44,compact?180:210);
 }else if(page==GameIniPage){
  field(85,fx,fy,fw);
  int spacing=compact?55:62;
  for(unsigned i=0;i<3;++i){int y=fy+65+i*spacing;
   show(iniLabels[i],fx,y,fw-120,22);show(iniValues[i],fx+fw-116,y-2,72,27);
   show(iniUnits[i],fx+fw-39,y,39,22);show(iniTracks[i],fx,y+25,fw-120,19);
  }
  show(gameIniHint,fx,fy+65+3*spacing+12,fw,compact?82:115);
  show(iniReload,fx,bottom-43,145,32);show(iniBackups,fx+165,bottom-43,145,32);
 }else if(page==ThirdPersonPage){
  show(dialogueViewCheck,fx,fy,fw,29);
  show(chestViewCheck,fx,fy+42,fw,29);
  show(lecternViewCheck,fx,fy+84,fw,29);
  show(thirdPersonOrbitCheck,fx,fy+140,fw,29);
  show(thirdPersonHint,fx,fy+190,fw,150);
 }else if(page==BowPage){
  int spacing=compact?45:49,rx=x+col+40;
  unsigned aimFields[]{23,24,25,26,27,28,81},gripFields[]{29,30,31};
  for(unsigned row=0;row<7;++row)field(aimFields[row],fx,fy+row*spacing,col-44);
  for(unsigned row=0;row<3;++row)field(gripFields[row],rx,fy+row*spacing,col-44);
  show(bowGripHint,rx,fy+3*spacing+10,col-44,150);
  show(bowHint,rx,fy+3*spacing+174,col-44,70);
 }else if(page!=InteractionPage&&page!=GraphicsPage&&page!=FoveatedPage&&page!=ModsPage&&page!=IniTweaksPage&&page!=BindingsPage){
  std::vector<unsigned> fields;for(unsigned i=0;i<diagnostic::VrNumberCount;++i)if(NumberPage(i)==page||page==MagicPage&&i==28)fields.push_back(i);
  int spacing=compact?(page==CrossbowPage?42:45):page==BowPage?49:55;for(unsigned row=0;row<fields.size();++row)field(fields[row],fx,fy+row*spacing,fw);
  int after=fy+int(fields.size())*spacing+10;
  if(page==HudPage){show(compassCheck,fx,after,fw,29);show(slotBarCheck,fx,after+32,fw,29);show(vitalsPanelCheck,fx,after+64,fw,29);show(bowReticleCheck,fx,after+96,fw,29);}
  if(page==LeftHandPage||page==RightHandPage)show(handHint,fx,after,fw,70);
  if(page==CrossbowPage)show(crossbowHint,fx,after,fw,44);
  if(page==MagicPage){show(magicReticleCheck,fx,after,fw,29);show(magicHint,fx,after+40,fw,70);}
  if(page==RadialPage)show(radialHint,fx,after+4,fw,190);
  if(page==ShieldPage)show(shieldHint,fx,after,fw,70);
 }else if(page==InteractionPage){
  show(aimLabel,fx,fy,fw-110,22);show(aimTrack,fx,fy+26,fw,25);show(aimEdit,fx+fw-99,fy-2,58,27);show(aimUnit,fx+fw-35,fy,40,22);show(aimHint,fx,fy+83,fw,130);
  show(fastPickupCheck,fx,fy+230,fw,29);
 }else if(page==BindingsPage){
  LayoutControllerMapper(fx,fy,fw,compact);
 }else if(page==GraphicsPage){
  show(graphicsLabel,fx,fy,180,24);show(graphicsCombo,fx+185,fy-4,fw-185,180);
  int gap=compact?54:64;for(int i=0;i<2;++i){int y=fy+gap+i*gap;show(graphicsLabels[i],fx,y,fw-110,22);show(graphicsTracks[i],fx,y+25,fw,24);show(graphicsEdits[i],fx+fw-99,y-2,58,27);show(graphicsUnits[i],fx+fw-35,y,40,23);}
  int aaY=fy+(compact?213:258),effectsY=fy+(compact?258:311);
  show(graphicsPreview,fx,fy+(compact?173:210),fw,28);show(aaLabel,fx,aaY,180,24);show(aaCombo,fx+185,aaY-4,fw-185,180);
  show(effectsLabel,fx,effectsY,180,24);show(effectsCombo,fx+185,effectsY-4,fw-185,180);
  show(graphicsHint,fx,fy+(compact?302:357),fw,40);
 }else if(page==FoveatedPage){
  show(ffrLabel,fx,fy,180,24);show(ffrCombo,fx+185,fy-4,fw-185,180);show(ffrCenterLabel,fx,fy+72,fw-110,24);show(ffrTrack,fx,fy+99,fw,25);show(ffrEdit,fx+fw-99,fy+70,58,27);show(ffrUnit,fx+fw-35,fy+72,40,24);show(ffrTransitionLabel,fx,fy+148,fw-110,24);show(ffrTransitionTrack,fx,fy+175,fw-110,25);show(ffrTransitionEdit,fx+fw-99,fy+146,58,27);show(ffrTransitionUnit,fx+fw-35,fy+148,40,24);show(ffrHint,fx,fy+220,fw,compact?151:190);
 }else if(page==IniTweaksPage){
  int spacing=compact?53:60,iw=col-44,rx=x+col+40;
  auto number=[&](unsigned i,int atX,int atY){show(iniLabels[i],atX,atY,iw-120,22);show(iniValues[i],atX+iw-116,atY-2,72,27);show(iniUnits[i],atX+iw-39,atY,39,22);show(iniTracks[i],atX,atY+25,iw-120,19);};
  for(unsigned i=3;i<5;++i)number(i,fx,fy+(i-3)*spacing);
  number(5,rx,fy);number(6,rx,fy+spacing);
  int choicesY=fy+2*spacing+7,gap=compact?33:40;
  for(unsigned i=7;i<=9;++i){int y=choicesY+(i-7)*gap;show(iniLabels[i],rx,y,iw-178,23);show(iniValues[i],rx+iw-172,y-3,172,180);}
  int toggleY=choicesY+3*gap+3;
  for(unsigned i=10;i<diagnostic::ini_tweaks::Count;++i)show(iniValues[i],rx,toggleY+(i-10)*(compact?25:29),iw,26);
  show(iniHint,fx,fy+2*spacing+12,iw,compact?75:90);show(iniHintRight,rx,toggleY+3*(compact?25:29)+6,iw,compact?40:48);
  show(iniReload,fx,bottom-43,145,32);show(iniBackups,rx,bottom-43,145,32);
 }else if(page==ModsPage){
  int iw=col-44,rx=x+col+40;
  show(unionLabel,fx,fy,fw,24);show(unionPath,fx,fy+30,fw-87,29);show(unionBrowse,fx+fw-80,fy+28,80,33);
  show(unionOpen,fx,fy+88,iw,35);show(unionCheck,fx,fy+131,iw,35);show(unionNotes,fx,fy+174,iw,35);
  show(unionHint,rx,fy+88,iw,ch-150);
 }
 SetWindowTextW(reset,page==IniTweaksPage||page==GameIniPage?L"CP defaults":L"Defaults");
 Position(status,x,h-89,cw,23);Position(reset,x,h-53,112,35);Position(save,w-393,h-55,152,39);Position(launch,w-224,h-55,194,39);
 UpdateNativeTooltipRects();
 ClearCanvas();HDC dc=GetDC(window);PaintCanvas(dc,false);ReleaseDC(window,dc);
 if(wasVisible){SendMessageW(window,WM_SETREDRAW,TRUE,0);RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_FRAME|RDW_NOERASE);}
 else RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_NOERASE);
}
