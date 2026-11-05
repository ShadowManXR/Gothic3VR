// Gothic 3 VR — project created and maintained by PeCOS2k5.
// SPDX-License-Identifier: GPL-3.0-or-later
// Original and third-party credits: AUTHORS.md and THIRD-PARTY-NOTICES.md.
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <cstdio>
#include <vector>
#include <gdiplus.h>
#include <uxtheme.h>
#include "LauncherVersion.h"
#include "VrSettings.h"
#include "LauncherCore.h"
#include "LauncherFixedFeatures.h"
#include "LauncherPages.h"
#include "GameResolution.h"
#include "EddDrawDistance.h"
#include "NativeIniTweaks.h"
#include "UnionProtection.h"
#include "OpenXrRuntimeSelection.h"
#include "ModCompatibilityData.h"
#include "ControllerMappingIpc.h"
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace {
HINSTANCE instance;HWND window,pathEdit,status,preview;
HWND tracks[diagnostic::VrNumberCount],edits[diagnostic::VrNumberCount],labels[diagnostic::VrNumberCount],units[diagnostic::VrNumberCount];
HWND title,subtitle,pathLabel,browse,save,reset,launch,hint;
HWND resolutionLabel,resolutionCombo,resolutionHint;
HWND ffrLabel,ffrCombo,ffrCenterLabel,ffrTrack,ffrEdit,ffrUnit,ffrHint;
HWND ffrTransitionLabel,ffrTransitionTrack,ffrTransitionEdit,ffrTransitionUnit;
HWND graphicsLabel,graphicsCombo,graphicsPreview,graphicsHint,graphicsLabels[2],graphicsTracks[2],graphicsEdits[2],graphicsUnits[2];
HWND aaLabel,aaCombo;
HWND effectsLabel,effectsCombo;
HWND runtimeLabel,runtimeCombo;
HWND handsOnlyCheck,toggleSupportCheck,compassCheck,slotBarCheck,vitalsPanelCheck,handHudCheck;
HWND fastPickupCheck,dialogueViewCheck,chestViewCheck,lecternViewCheck,thirdPersonOrbitCheck,thirdPersonHint;
HWND unionLabel,unionPath,unionBrowse,unionOpen,unionCheck,unionHint,unionNotes;
HWND unionShortcut;
HWND iniLabels[diagnostic::ini_tweaks::Count]{},iniTracks[diagnostic::ini_tweaks::Count]{},iniValues[diagnostic::ini_tweaks::Count]{},iniUnits[diagnostic::ini_tweaks::Count]{};
HWND iniHint{},iniHintRight{},iniReload{},iniBackups{},iniTooltipWindow{},gameIniHint{};
std::wstring iniTooltips[diagnostic::ini_tweaks::Count];
diagnostic::ini_tweaks::Model nativeIni;bool nativeIniDirty=false;
constexpr int IniTrackBase=600,IniValueBase=650,IniReload=690,IniBackups=691;
HWND settingsTabs;
HWND mappingLeftHanded,mappingJump,mappingLive,mappingActionLabel,mappingAction,mappingModeLabel,mappingMode,mappingButtonLabel,mappingButton,mappingModifierLabel,mappingModifier,mappingListen,mappingClear,mappingDefaults,mappingStatus,mappingHint;
void LayoutControllerMapper(int,int,int,bool);
HWND aimLabel,aimTrack,aimEdit,aimUnit,aimHint;
HWND handHint,swordHint,bowHint,bowReticleCheck,bowGripHint,twoHandCheck,heavyCheck,twoHandHint,staffHint,crossbowHint,magicHint,radialHint,shieldHint;
HFONT font,titleFont;HBRUSH background;UINT dpi=96;bool syncing=false,dirty=false;
std::wstring executable,launcherIni;diagnostic::VrSettings settings;
constexpr int Browse=10,Save=11,Reset=12,Launch=13,Resolution=17,Tabs=19,AimTrack=25,AimEdit=26,BowReticle=27,TwoHand=28,Heavy=29,GraphicsMethod=32,GraphicsTrack=33,GraphicsEdit=35,FfrMethod=38,FfrTrack=39,FfrEdit=40,TrackBase=1000,EditBase=200;
constexpr int UnionBrowse=42,UnionOpen=43,UnionCheck=44;
constexpr int UnionPage=45;
constexpr int AntiAliasing=46;
constexpr int RuntimeChoice=47;
constexpr int UnionNotes=48;
constexpr int EffectsQuality=49;
constexpr int FfrTransitionTrack=55,FfrTransitionEdit=56;
constexpr int HandsOnly=57,ToggleSupport=58,ShowCompass=59,ShowSlotBar=60,ShowVitalsPanel=61,HandHud=62;
constexpr int FastPickup=64,DialogueView=65,ChestView=66,LecternView=67,ThirdPersonOrbit=68;
int Scale(int n){return MulDiv(n,int(dpi),96);}
int NumberPage(unsigned i){
    if(i==diagnostic::FirstPersonNearClipIndex)return CameraPage;
    if(i>=87)return HandHudPage;
    if(i==86)return HudPage;
    if(i==85)return GameIniPage;
    if(i==84)return CrossbowPage;
    if(i==83)return StaffPage;
    if(i==82)return TwoHandPage;
    if(i==81)return BowPage;
    if(i>=73)return i<75?OneHandPage:i<77?TwoHandPage:StaffPage;
    if(i>=64)return (i-64)%3==0?OneHandPage:(i-64)%3==1?TwoHandPage:StaffPage;
    if(i>=58)return ShieldPage;if(i>=56)return StaffPage;if(i>=54)return TwoHandPage;if(i>=52)return OneHandPage;
    return i==51?RadialPage:i>=48?MagicPage:i>=42?CrossbowPage:i>=36?StaffPage:i>=32?TwoHandPage:i>=23?BowPage:i>=20?OneHandPage:i>=14?RightHandPage:i>=8?LeftHandPage:i==0||i==6||i==7?CameraPage:HudPage;
}
std::wstring Ini(){return diagnostic::ParentDirectory(executable)+L"\\scripts\\Script_Gothic3VRDiagnostics.ini";}
std::wstring OptionsIni(){wchar_t documents[MAX_PATH];if(FAILED(SHGetFolderPathW(nullptr,CSIDL_PERSONAL,nullptr,SHGFP_TYPE_CURRENT,documents)))return L"";
    return std::wstring(documents)+L"\\Gothic3\\UserOptions.ini";}
void Message(wchar_t const* text,bool error=false){SetWindowTextW(status,text);if(error)MessageBoxW(window,text,L"Gothic 3 VR",MB_OK|MB_ICONERROR);}
void StyleControl(HWND,wchar_t const*,DWORD);
HWND Control(wchar_t const* cls,wchar_t const* text,DWORD style,int id=0) {
    HWND h=CreateWindowExW(cls==std::wstring(L"EDIT")?WS_EX_CLIENTEDGE:0,cls,text,WS_CHILD|WS_VISIBLE|style|(!wcscmp(cls,L"STATIC")?SS_NOPREFIX:0),0,0,0,0,window,reinterpret_cast<HMENU>(INT_PTR(id)),instance,nullptr);
    SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);StyleControl(h,cls,style);return h;
}
void Position(HWND h,int x,int y,int width,int height){MoveWindow(h,Scale(x),Scale(y),Scale(width),Scale(height),FALSE);}
bool ModsBlocked(){try{auto game=diagnostic::ParentDirectory(executable);if(diagnostic::optional_mods::Active(game)){Message(L"Protected Union+ is running. Close Union+ before saving VR settings or launching the game.",true);return true;}
 if(!diagnostic::optional_mods::Pending(game).empty()){Message(L"A protected mod session needs verification. Open Optional mods and choose Verify / recover VR.",true);return true;}}
 catch(...){Message(L"Could not verify the Union+ protection state. Use Optional mods > Verify / recover VR.",true);return true;}return false;}
void ChooseUnion(){if(ModsBlocked())return;wchar_t path[MAX_PATH]{};GetWindowTextW(unionPath,path,MAX_PATH);OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;
 dialog.lpstrFilter=L"Union+ (UnionStarter.exe)\0UnionStarter.exe\0";dialog.lpstrFile=path;dialog.nMaxFile=MAX_PATH;dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 if(GetOpenFileNameW(&dialog)){if(!WritePrivateProfileStringW(L"Launcher",L"UnionExecutable",path,launcherIni.c_str())){Message(L"Could not save the Union+ path.",true);return;}SetWindowTextW(unionPath,path);}}
void OpenUnion(bool recover){if(diagnostic::GothicRunning()){Message(L"Close Gothic 3 before changing mods or verifying VR files.",true);return;}
 auto helper=diagnostic::ParentDirectory(launcherIni)+L"\\Gothic3VRUnionGuard.exe";wchar_t path[MAX_PATH]{};GetWindowTextW(unionPath,path,MAX_PATH);
 if(!recover&&!path[0]){ChooseUnion();GetWindowTextW(unionPath,path,MAX_PATH);if(!path[0])return;}
 std::wstring args=L"--game \""+executable+L"\""+(recover?L" --recover":std::wstring(L" --union \"")+path+L"\"");PROCESS_INFORMATION process{};
 if(!diagnostic::StartExecutable(helper,args,process)){Message(L"Could not start VR protection. Check Gothic3VRUnionGuard.exe beside this launcher.",true);return;}
 CloseHandle(process.hThread);CloseHandle(process.hProcess);Message(recover?L"Checking VR compatibility and interrupted-session recovery...":L"Opening Union+ with VR protection. Close Union+ before starting Gothic 3.");}
void GraphicsPreview(){
    int selected=int(SendMessageW(resolutionCombo,CB_GETCURSEL,0,0));if(selected<0||selected>=int(diagnostic::VrResolutionCount))return;
    int method=int(SendMessageW(graphicsCombo,CB_GETCURSEL,0,0));BOOL valid=FALSE;int scale=int(GetDlgItemInt(window,GraphicsEdit,&valid,FALSE));if(!valid)scale=75;
    auto r=diagnostic::VrResolutions[selected];wchar_t text[256];
    if(!selected)swprintf_s(text,L"Use game settings: sharpening only; native resolution is preserved.");
    else{auto p=diagnostic::MakeUpscalePlan(r.width,r.height,diagnostic::ValidUpscaleMethod(method),scale,25);
        swprintf_s(text,L"Scene: %u × %u  →  Headset per eye: %u × %u  (%.0f%% scene pixels)",p.inputWidth,p.inputHeight,p.outputWidth,p.outputHeight,100.0*double(p.inputWidth)*p.inputHeight/(double(p.outputWidth)*p.outputHeight));}
    SetWindowTextW(graphicsPreview,text);
    bool enabled=method>0;EnableWindow(graphicsTracks[0],enabled&&selected!=0);EnableWindow(graphicsEdits[0],enabled&&selected!=0);
    EnableWindow(graphicsTracks[1],enabled);EnableWindow(graphicsEdits[1],enabled);
}
#include "LauncherTheme.h"
#include "ModCompatibilityView.h"
void UpdateNativeTooltipRects();
#include "LauncherLayout.h"
#include "LauncherIniTweaks.h"
#include "LauncherMapper.h"

void Fill() {
    syncing=true;diagnostic::ApplyLauncherFixedFeatures(settings);
    FillControllerMapper();
    SendMessageW(ffrCombo,CB_SETCURSEL,settings.ffrMode,0);SendMessageW(ffrTrack,TBM_SETRANGE,FALSE,MAKELPARAM(40,95));SendMessageW(ffrTrack,TBM_SETLINESIZE,0,5);SendMessageW(ffrTrack,TBM_SETPAGESIZE,0,5);SendMessageW(ffrTrack,TBM_SETPOS,TRUE,settings.ffrDiameterPercent);SetDlgItemInt(window,FfrEdit,settings.ffrDiameterPercent,FALSE);
    SendMessageW(ffrTransitionTrack,TBM_SETRANGE,FALSE,MAKELPARAM(0,30));SendMessageW(ffrTransitionTrack,TBM_SETLINESIZE,0,1);SendMessageW(ffrTransitionTrack,TBM_SETPAGESIZE,0,5);SendMessageW(ffrTransitionTrack,TBM_SETPOS,TRUE,settings.ffrTransitionPercent);SetDlgItemInt(window,FfrTransitionEdit,settings.ffrTransitionPercent,FALSE);
    SendMessageW(graphicsCombo,CB_SETCURSEL,int(settings.upscaleMethod),0);
    settings.dlaaExperimental=false; // Retired experiment: never re-enable from an old configuration.
    SendMessageW(aaCombo,CB_SETCURSEL,settings.smaaEnabled?2+diagnostic::ValidSmaaQuality(settings.smaaQuality):settings.fxaaEnabled?1:0,0);
    SendMessageW(effectsCombo,CB_SETCURSEL,diagnostic::particles::Quality(settings.particleQuality),0);
    SendMessageW(runtimeCombo,CB_SETCURSEL,int(settings.openXrRuntime),0);
    for(int i=0;i<2;++i){SendMessageW(graphicsTracks[i],TBM_SETRANGE,FALSE,MAKELPARAM(i?0:50,100));SendMessageW(graphicsTracks[i],TBM_SETPAGESIZE,0,5);SendMessageW(graphicsTracks[i],TBM_SETLINESIZE,0,5);
        int value=i?settings.upscaleSharpness:settings.upscaleScalePercent;SendMessageW(graphicsTracks[i],TBM_SETPOS,TRUE,value);SetDlgItemInt(window,GraphicsEdit+i,value,FALSE);}
    for(unsigned i=0;i<diagnostic::VrNumberCount;++i){auto const& f=diagnostic::VrNumbers[i];
        SendMessageW(tracks[i],TBM_SETRANGEMIN,FALSE,f.minimum);SendMessageW(tracks[i],TBM_SETRANGEMAX,FALSE,f.maximum);
        SendMessageW(tracks[i],TBM_SETLINESIZE,0,f.step);SendMessageW(tracks[i],TBM_SETPAGESIZE,0,f.step);
        SendMessageW(tracks[i],TBM_SETPOS,TRUE,settings.numbers[i]);SetDlgItemInt(window,EditBase+i,UINT(settings.numbers[i]),TRUE);}
    SendMessageW(aimTrack,TBM_SETRANGEMIN,FALSE,diagnostic::InteractionAimMinimum);SendMessageW(aimTrack,TBM_SETRANGEMAX,FALSE,diagnostic::InteractionAimMaximum);
    SendMessageW(aimTrack,TBM_SETPOS,TRUE,settings.interactionAimUpPercent);SetDlgItemInt(window,AimEdit,UINT(settings.interactionAimUpPercent),TRUE);
    SendMessageW(fastPickupCheck,BM_SETCHECK,settings.fastPickup?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(dialogueViewCheck,BM_SETCHECK,settings.firstPersonDialogue?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(chestViewCheck,BM_SETCHECK,settings.firstPersonChest?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(lecternViewCheck,BM_SETCHECK,settings.firstPersonLectern?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(thirdPersonOrbitCheck,BM_SETCHECK,settings.thirdPersonOrbit?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(compassCheck,BM_SETCHECK,settings.hideCompass?BST_UNCHECKED:BST_CHECKED,0);
    SendMessageW(slotBarCheck,BM_SETCHECK,settings.showSlotBar?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(handHudCheck,BM_SETCHECK,settings.handHudEnabled?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(vitalsPanelCheck,BM_SETCHECK,settings.showVitalsPanel?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(bowReticleCheck,BM_SETCHECK,settings.bowReticle?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(heavyCheck,BM_SETCHECK,settings.heavyTwoHand?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(twoHandCheck,BM_SETCHECK,settings.twoHandedMelee?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(handsOnlyCheck,BM_SETCHECK,settings.handsOnly?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(toggleSupportCheck,BM_SETCHECK,settings.toggleSupportGrip?BST_CHECKED:BST_UNCHECKED,0);
    SetWindowTextW(pathEdit,executable.c_str());syncing=false;dirty=false;
    SendMessageW(resolutionCombo,CB_RESETCONTENT,0,0);
    wchar_t current[96];auto path=OptionsIni();if(GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES){
        auto slash=path.find_last_of(L"\\/");auto recovery=path.substr(0,slash+1)+L"__UserOptions.ini";
        path=GetFileAttributesW(recovery.c_str())!=INVALID_FILE_ATTRIBUTES?recovery:path+L".bak";}
    int width=GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,path.c_str());int height=GetPrivateProfileIntW(L"Options.Video",L"Resolution.Height",0,path.c_str());
    swprintf_s(current,L"Use game settings (%d × %d)",width,height);
    for(unsigned i=0;i<diagnostic::VrResolutionCount;++i)SendMessageW(resolutionCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(i?diagnostic::VrResolutions[i].label:current));
    SendMessageW(resolutionCombo,CB_SETCURSEL,diagnostic::ResolutionIndex(settings.renderWidth,settings.renderHeight),0);GraphicsPreview();
}
bool Collect() {
    diagnostic::ApplyLauncherFixedFeatures(settings);
    wchar_t ffrText[64];GetWindowTextW(ffrEdit,ffrText,64);wchar_t* ffrEnd=nullptr;long ffrValue=wcstol(ffrText,&ffrEnd,10);
    if(ffrEnd==ffrText||*ffrEnd||ffrValue<40||ffrValue>95){Message(L"Full-detail diameter: enter 40 to 95 %.",true);TabCtrl_SetCurSel(settingsTabs,FoveatedPage);Layout();SetFocus(ffrEdit);return false;}
    settings.ffrMode=diagnostic::FfrMode(int(SendMessageW(ffrCombo,CB_GETCURSEL,0,0)));settings.ffrDiameterPercent=int(ffrValue);
    wchar_t transitionText[64];GetWindowTextW(ffrTransitionEdit,transitionText,64);wchar_t* transitionEnd=nullptr;long transition=wcstol(transitionText,&transitionEnd,10);
    if(transitionEnd==transitionText||*transitionEnd||transition<0||transition>30){Message(L"Transition width: enter 0 to 30 %.",true);TabCtrl_SetCurSel(settingsTabs,FoveatedPage);Layout();SetFocus(ffrTransitionEdit);return false;}
    settings.ffrTransitionPercent=int(transition);
    int graphics[2]{};for(int i=0;i<2;++i){wchar_t text[64];GetWindowTextW(graphicsEdits[i],text,64);wchar_t* end=nullptr;long value=wcstol(text,&end,10);
        if(end==text||*end||value<(i?0:50)||value>100){Message(i?L"Sharpness: enter 0 to 100 %.":L"Internal render scale: enter 50 to 100 %.",true);TabCtrl_SetCurSel(settingsTabs,GraphicsPage);Layout();SetFocus(graphicsEdits[i]);return false;}graphics[i]=int(value);}
    settings.upscaleMethod=diagnostic::ValidUpscaleMethod(int(SendMessageW(graphicsCombo,CB_GETCURSEL,0,0)));settings.upscaleScalePercent=graphics[0];settings.upscaleSharpness=graphics[1];
    settings.dlaaExperimental=false;
    int aa=int(SendMessageW(aaCombo,CB_GETCURSEL,0,0));
    settings.fxaaEnabled=aa==1;settings.smaaEnabled=aa>=2&&aa<=4;if(settings.smaaEnabled)settings.smaaQuality=aa-2;
    settings.particleQuality=diagnostic::particles::Quality(int(SendMessageW(effectsCombo,CB_GETCURSEL,0,0)));
    settings.openXrRuntime=diagnostic::ValidOpenXrRuntime(int(SendMessageW(runtimeCombo,CB_GETCURSEL,0,0)));
    for(unsigned i=0;i<diagnostic::VrNumberCount;++i){wchar_t text[64];GetWindowTextW(edits[i],text,64);wchar_t* end=nullptr;long n=wcstol(text,&end,10);
        auto const& f=diagnostic::VrNumbers[i];if(end==text||*end||n<f.minimum||n>f.maximum||(n-f.minimum)%f.step!=0){
            wchar_t error[256];swprintf_s(error,L"%s: enter a value between %d and %d %s (step %d).",f.label,f.minimum,f.maximum,f.unit,f.step);Message(error,true);
            TabCtrl_SetCurSel(settingsTabs,NumberPage(i));Layout();SetFocus(edits[i]);return false;}
        settings.numbers[i]=int(n);}
    wchar_t aimText[64];GetWindowTextW(aimEdit,aimText,64);wchar_t* aimEnd=nullptr;long aimValue=wcstol(aimText,&aimEnd,10);
    if(aimEnd==aimText||*aimEnd||aimValue<diagnostic::InteractionAimMinimum||aimValue>diagnostic::InteractionAimMaximum){
        Message(L"Selection vertical aim: enter a value between -200 and 200 %.",true);TabCtrl_SetCurSel(settingsTabs,InteractionPage);Layout();SetFocus(aimEdit);return false;}
    settings.interactionAimUpPercent=int(aimValue);
    settings.fastPickup=SendMessageW(fastPickupCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    settings.firstPersonDialogue=SendMessageW(dialogueViewCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    settings.firstPersonChest=SendMessageW(chestViewCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    settings.firstPersonLectern=SendMessageW(lecternViewCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    settings.thirdPersonOrbit=SendMessageW(thirdPersonOrbitCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    settings.hideCompass=SendMessageW(compassCheck,BM_GETCHECK,0,0)!=BST_CHECKED;
    settings.showSlotBar=SendMessageW(slotBarCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    settings.handHudEnabled=SendMessageW(handHudCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    settings.showVitalsPanel=SendMessageW(vitalsPanelCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    settings.bowReticle=SendMessageW(bowReticleCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    settings.twoHandedMelee=SendMessageW(twoHandCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    settings.handsOnly=SendMessageW(handsOnlyCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    settings.toggleSupportGrip=SendMessageW(toggleSupportCheck,BM_GETCHECK,0,0)==BST_CHECKED;
    settings.heavyTwoHand=SendMessageW(heavyCheck,BM_GETCHECK,0,0)==BST_CHECKED;return true;
}
bool Installed() {
    auto root=diagnostic::ParentDirectory(executable);
    if(GetFileAttributesW(executable.c_str())==INVALID_FILE_ATTRIBUTES||
       GetFileAttributesW((root+L"\\scripts\\Script_Gothic3VRDiagnostics.dll").c_str())==INVALID_FILE_ATTRIBUTES||
       GetFileAttributesW(Ini().c_str())==INVALID_FILE_ATTRIBUTES){Message(L"Select Gothic3.exe from the folder where the VR mod is installed.",true);return false;}return true;
}
bool SaveSettings() {
    if(ModsBlocked())return false;
    if(!Installed()||!Collect()||!CollectNativeIni())return false;
    if(nativeIniDirty&&nativeIni.Changed()&&diagnostic::GothicRunning()){Message(L"Close Gothic 3 before saving INI tweaks. VR-only settings can still be saved while playing.",true);return false;}
    int selected=int(SendMessageW(resolutionCombo,CB_GETCURSEL,0,0));if(selected<0||selected>=int(diagnostic::VrResolutionCount))return false;
    settings.renderWidth=diagnostic::VrResolutions[selected].width;settings.renderHeight=diagnostic::VrResolutions[selected].height;
    if(!diagnostic::SaveVrSettings(Ini().c_str(),settings)){Message(L"Could not save settings. Check the game folder permissions.",true);return false;}
    WritePrivateProfileStringW(L"Launcher",L"GameExecutable",executable.c_str(),launcherIni.c_str());
    bool nativeChanged=false;
    if(nativeIniDirty){auto applied=diagnostic::ini_tweaks::Save(nativeIni);if(!applied.ok){Message((L"VR settings saved; native INIs were not applied. "+applied.error).c_str(),true);return false;}nativeChanged=applied.changed;LoadNativeIni();}
    Fill();Message(nativeChanged?L"INI tweaks saved with external backups. Restart Gothic 3 to apply them.":L"Settings saved. Controller mapping and label size apply live; runtime / scene graphics changes may need a restart.");return true;
}
void ChooseGame() {
    wchar_t path[MAX_PATH];wcscpy_s(path,executable.c_str());OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;
    dialog.lpstrFilter=L"Gothic 3 (Gothic3.exe)\0Gothic3.exe\0";dialog.lpstrFile=path;dialog.nMaxFile=MAX_PATH;dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    if(GetOpenFileNameW(&dialog)){executable=path;settings=diagnostic::LoadVrSettings(Ini().c_str());Fill();LoadNativeIni();Message(L"Settings loaded from the selected folder.");}
}
void LaunchGame() {
    if(diagnostic::GothicRunning()){Message(L"Gothic 3 is already running. You can save camera and VR panel settings.");return;}
    if(!SaveSettings())return;
    if(settings.autoStart){try{auto manifest=diagnostic::xr_runtime::Select(settings.openXrRuntime);
        if(settings.openXrRuntime==diagnostic::OpenXrRuntimeChoice::MetaLink||(settings.openXrRuntime==diagnostic::OpenXrRuntimeChoice::System&&!_wcsicmp(manifest.filename().c_str(),L"oculus_openxr_32.json"))){
            diagnostic::xr_runtime::ValidateManifest(manifest.parent_path()/L"oculus_openxr_64.json",IMAGE_FILE_MACHINE_AMD64);
            auto folder=std::filesystem::path(diagnostic::ParentDirectory(executable))/L"scripts/Gothic3VRDiagnostics/meta64";
            if(!std::filesystem::is_regular_file(folder/L"Gothic3VRMetaHost64.exe")||!std::filesystem::is_regular_file(folder/L"openxr_loader64.dll"))throw std::runtime_error("Meta 64-bit host is missing. Reinstall the VR runtime files.");
        }
    }catch(std::exception const& error){
        int n=MultiByteToWideChar(CP_UTF8,0,error.what(),-1,nullptr,0);std::wstring text(n,L'\0');MultiByteToWideChar(CP_UTF8,0,error.what(),-1,text.data(),n);Message(text.c_str(),true);return;}}
    try{diagnostic::optional_mods::VerifyCore(diagnostic::ParentDirectory(executable));}catch(...){Message(L"The native engine or VR startup files changed. Use Optional mods > Verify / recover VR before launching.",true);return;}
    if(settings.renderWidth&&GetFileAttributesW((diagnostic::ParentDirectory(executable)+L"\\patches\\Patch_Gothic3VRResolution.dll").c_str())==INVALID_FILE_ATTRIBUTES){
        Message(L"The VR resolution startup extension is missing. Reinstall the VR mod before using a custom resolution.",true);return;}
    DWORD videoError=0;auto options=OptionsIni();
    if(!diagnostic::ApplyGameResolution(options.c_str(),settings,&videoError)){
        wchar_t message[768];swprintf_s(message,L"Could not apply resolution (Windows error %lu).\n%s\nPrevious settings were preserved.",videoError,options.c_str());Message(message,true);return;}
    Fill();
    DWORD distanceError=0;
    if(!diagnostic::edd::Apply(diagnostic::ParentDirectory(executable),distanceError)){
        wchar_t message[256];swprintf_s(message,L"Could not import EDD draw distances (Windows error %lu). Check G3_edd.ini and Ini\\ge3.ini. Supported ranges: SH 1000-50000, SL 10000-300000, VR 1000-30000.",distanceError);Message(message,true);return;
    }
    PROCESS_INFORMATION process{};if(!diagnostic::StartExecutable(executable,L"",process)){
        wchar_t message[200];swprintf_s(message,L"Game launch failed (Windows error %lu).",GetLastError());Message(message,true);return;}
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    Message(settings.autoStart?L"Game launched. VR starts through the selected OpenXR runtime.":L"Game launched. Enable VR with Ctrl+Shift+F8.");
}
void Fonts() {
    for(HFONT f:{font,titleFont,smallFont,navFont,brandFont,sectionFont})if(f)DeleteObject(f);
    auto make=[](int size,int weight,wchar_t const* family=L"Segoe UI"){return CreateFontW(-Scale(size),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,family);};
    font=make(15,FW_NORMAL);titleFont=make(30,FW_SEMIBOLD);smallFont=make(12,FW_NORMAL);navFont=make(14,FW_NORMAL);brandFont=make(22,FW_BOLD,L"Georgia");sectionFont=make(12,FW_SEMIBOLD);ClearCanvas();
}
LRESULT CALLBACK Procedure(HWND h,UINT msg,WPARAM w,LPARAM l) {
    switch(msg){
    case WM_CREATE:
        window=h;Fonts();
        for(int p=0;p<PageCount;++p)navigation[p]=Control(L"BUTTON",PageNames[p],WS_TABSTOP,NavBase+p);title=Control(L"STATIC",L"Gothic 3 VR",0);SendMessageW(title,WM_SETFONT,reinterpret_cast<WPARAM>(titleFont),TRUE);
        subtitle=Control(L"STATIC",L"Quest / Touch · OpenXR",0);pathLabel=Control(L"STATIC",L"Game executable",0);
        runtimeLabel=Control(L"STATIC",L"OpenXR runtime (restart required)",0);
        handsOnlyCheck=Control(L"BUTTON",L"Hands only (hide body)",WS_TABSTOP|BS_AUTOCHECKBOX,HandsOnly);
        toggleSupportCheck=Control(L"BUTTON",L"Toggle support grip (2H / staff / crossbow)",WS_TABSTOP|BS_AUTOCHECKBOX,ToggleSupport);
        runtimeCombo=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,RuntimeChoice);
        for(int i=0;i<4;++i)SendMessageW(runtimeCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(diagnostic::OpenXrRuntimeLabel(diagnostic::OpenXrRuntimeChoice(i))));
        unionShortcut=Control(L"BUTTON",L"Optional mods...",WS_TABSTOP,UnionPage);
        pathEdit=Control(L"EDIT",L"",ES_READONLY|ES_AUTOHSCROLL);browse=Control(L"BUTTON",L"Browse…",WS_TABSTOP,Browse);
        resolutionLabel=Control(L"STATIC",L"Output resolution per eye",0);
        resolutionCombo=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,Resolution);
        settingsTabs=Control(WC_TABCONTROLW,L"",WS_TABSTOP,Tabs);
        for(int page=0;page<PageCount;++page){TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(PageNames[page]);TabCtrl_InsertItem(settingsTabs,page,&item);}
        resolutionHint=Control(L"STATIC",L"Applied at launch. Graphics tab: NIS / CAS can render the scene at a lower resolution.",0);
        for(unsigned i=0;i<diagnostic::VrNumberCount;++i){labels[i]=Control(L"STATIC",diagnostic::VrNumbers[i].label,0);
            tracks[i]=Control(TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,TrackBase+i);
            edits[i]=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,EditBase+i);units[i]=Control(L"STATIC",diagnostic::VrNumbers[i].unit,0);SendMessageW(edits[i],EM_SETLIMITTEXT,6,0);}
        aimLabel=Control(L"STATIC",L"Selection vertical aim",0);aimTrack=Control(TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,AimTrack);
        aimEdit=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,AimEdit);aimUnit=Control(L"STATIC",L"%",0);SendMessageW(aimEdit,EM_SETLIMITTEXT,6,0);
        aimHint=Control(L"STATIC",L"Positive values tilt selection up; negative values tilt it down.\n0 = the controller's original direction. Range: -200 to +200 %.\nIf you must bend your wrist down, try -15 or -30 %.\nPrecise hits favor small targets over beds and shrines.\nUse Save settings to apply while playing.",0);
        unionLabel=Control(L"STATIC",L"Union+ 1.7.3 (optional)",0);unionPath=Control(L"EDIT",L"",ES_AUTOHSCROLL|ES_READONLY);
        {wchar_t path[MAX_PATH]{};GetPrivateProfileStringW(L"Launcher",L"UnionExecutable",L"",path,MAX_PATH,launcherIni.c_str());SetWindowTextW(unionPath,path);}
        unionBrowse=Control(L"BUTTON",L"Browse...",WS_TABSTOP,UnionBrowse);unionOpen=Control(L"BUTTON",L"Open protected Union+",WS_TABSTOP,UnionOpen);unionCheck=Control(L"BUTTON",L"Verify / recover VR",WS_TABSTOP,UnionCheck);
        unionNotes=Control(L"BUTTON",L"VR compatibility notes...",WS_TABSTOP,UnionNotes);CreateModNotesTooltip();
        unionHint=Control(L"STATIC",L"Supported version: Union+ 1.7.3\n\nUse your own Nexus Mods download.\n\nOpen Union+ here to protect your VR files.\nClose it before playing or saving settings.\n\nUnion+ manages mod selection and dependencies.\nGameplay or animation mods may need a new game.",0);
        graphicsLabel=Control(L"STATIC",L"Upscaler",0);graphicsCombo=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,GraphicsMethod);
        aaLabel=Control(L"STATIC",L"Anti-aliasing",0);aaCombo=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,AntiAliasing);
        for(auto text:{L"Off",L"FXAA - Low cost",L"SMAA 1x - Medium",L"SMAA 1x - High",L"SMAA 1x - Ultra"})SendMessageW(aaCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
        effectsLabel=Control(L"STATIC",L"Fire & magic effects",0);effectsCombo=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,EffectsQuality);
        for(auto text:{L"Original — all native particles",L"Balanced VR — 65% particles, no heat haze",L"Performance VR — 40% particles, no heat haze"})SendMessageW(effectsCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
        for(auto text:{L"Off — native resolution",L"NVIDIA Image Scaling (NIS)",L"AMD Contrast Adaptive Sharpening (CAS)"})SendMessageW(graphicsCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
        for(int i=0;i<2;++i){graphicsLabels[i]=Control(L"STATIC",i?L"Sharpness":L"Internal render scale",0);graphicsTracks[i]=Control(TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,GraphicsTrack+i);graphicsEdits[i]=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,GraphicsEdit+i);graphicsUnits[i]=Control(L"STATIC",L"%",0);SendMessageW(graphicsEdits[i],EM_SETLIMITTEXT,3,0);}
        ffrLabel=Control(L"STATIC",L"Hardware FFR",0);ffrCombo=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,FfrMethod);
        for(auto text:{L"Off",L"Quality — large native center, 2×2 edges",L"Balanced — medium native center, 2×2 edges",L"Performance — smaller center, up to 4×4 corners"})SendMessageW(ffrCombo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
        ffrCenterLabel=Control(L"STATIC",L"Full-detail diameter",0);ffrTrack=Control(TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,FfrTrack);ffrEdit=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,FfrEdit);ffrUnit=Control(L"STATIC",L"%",0);SendMessageW(ffrEdit,EM_SETLIMITTEXT,2,0);
        ffrTransitionLabel=Control(L"STATIC",L"Transition width",0);ffrTransitionTrack=Control(TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,FfrTransitionTrack);
        ffrTransitionEdit=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,FfrTransitionEdit);ffrTransitionUnit=Control(L"STATIC",L"%",0);
        ffrHint=Control(L"STATIC",L"Requires the Gothic 3 VR DXVK FFR backend and NVIDIA shading-rate support.\nThe full-detail center stays a landscape oval (3:2).\nTransition width blends a pixel blur around the detail boundary.\n0 = original boundary, no extra pass. Blur runs after scene AA/upscaling.\nMenus and HUD stay at full quality. Save settings applies FFR live.",0);
        graphicsPreview=Control(L"STATIC",L"",0);
        graphicsHint=Control(L"STATIC",L"Fog: 100% = current world fog; 0% = clear world-fog ramp. Sky colors and draw distances stay unchanged.\nSave settings applies fog / effects live. AA / upscaler changes need a restart.",0);
        shieldHint=Control(L"STATIC",L"Raise shield / weapon to chest level, then hold right grip to block.\nNative timing, stamina and skills apply. Save settings applies live.",0);
        radialHint=Control(L"STATIC",L"Radial size changes independently of native menus and HUD.\n80% = smaller; 100% = the previous size. Save settings applies live.\nMenu: main menu / back. L3 tap: Sprint until stop. Double-tap left trigger: quick heal.\nRight stick up: jump. Down: toggle Sneak; eye icon and vibration follow native state.\nRadial: R3 opens, right stick selects, A confirms, X cancels, B changes page.\nTrading: right grip = one item; right trigger = drag a stack.",0);
        handHint=Control(L"STATIC",L"Position offsets place the palm relative to the controller grip; rotations pivot at the palm.\nNegative outward moves inward. Negative up moves down. Save settings applies live.",0);
        swordHint=Control(L"STATIC",L"Weight adds inertia; stabilization reduces jitter. Both at 0 = direct tracking.\nSave settings applies live to both 1H hands, including dual wield.",0);
        fastPickupCheck=Control(L"BUTTON",L"Skip item pickup delay and animation",WS_TABSTOP|BS_AUTOCHECKBOX,FastPickup);
        dialogueViewCheck=Control(L"BUTTON",L"Keep first-person view during dialogue",WS_TABSTOP|BS_AUTOCHECKBOX,DialogueView);
        chestViewCheck=Control(L"BUTTON",L"Keep first-person view and suppress chest animation",WS_TABSTOP|BS_AUTOCHECKBOX,ChestView);
        lecternViewCheck=Control(L"BUTTON",L"Keep first-person view when reading at lecterns",WS_TABSTOP|BS_AUTOCHECKBOX,LecternView);
        thirdPersonOrbitCheck=Control(L"BUTTON",L"Allow camera orbit in third-person view",WS_TABSTOP|BS_AUTOCHECKBOX,ThirdPersonOrbit);
        thirdPersonHint=Control(L"STATIC",L"Enabled view options retain the VR viewpoint. Turn one off to restore the native interaction camera.\nThe chest option also suppresses the displayed hero animation and back-mounted weapons. Turn it off to restore both.\nCamera orbit applies to native third-person activities.\nSave settings applies these choices live.",0);
        compassCheck=Control(L"BUTTON",L"Show compass",WS_TABSTOP|BS_AUTOCHECKBOX,ShowCompass);
        slotBarCheck=Control(L"BUTTON",L"Show slot bar",WS_TABSTOP|BS_AUTOCHECKBOX,ShowSlotBar);
        handHudCheck=Control(L"BUTTON",L"Show hand HUD (both arms)",WS_TABSTOP|BS_AUTOCHECKBOX,HandHud);
        vitalsPanelCheck=Control(L"BUTTON",L"Show health / mana / stamina panel",WS_TABSTOP|BS_AUTOCHECKBOX,ShowVitalsPanel);
        bowReticleCheck=Control(L"BUTTON",L"Aiming reticle (bow / crossbow / magic)",WS_TABSTOP|BS_AUTOCHECKBOX,BowReticle);
        bowHint=Control(L"STATIC",L"Right trigger: draw and release. String-hand influence: 0 = left-hand aim only.",0);
        bowGripHint=Control(L"STATIC",L"Grip offsets move only the bow, not your hand.\nToward fingers: away from the wrist.\nPalm depth: toward the palm surface.\nAcross palm: sideways along the grip.\nThe arrow stays above the grip. Save applies live.",0);
        twoHandCheck=Control(L"BUTTON",L"Two-hand grip (left grip button)",WS_TABSTOP|BS_AUTOCHECKBOX,TwoHand);
        heavyCheck=Control(L"BUTTON",L"Heavy one-hand handling",WS_TABSTOP|BS_AUTOCHECKBOX,Heavy);
        twoHandHint=Control(L"STATIC",L"Two-hand weight and stabilization apply with left grip. Save settings applies live.",0);
        magicHint=Control(L"STATIC",L"Right trigger: cast. Grip offsets move the item without moving the hand.\nAnimal form: hold X + Y for 0.5 s to return human and recenter.",0);
        crossbowHint=Control(L"STATIC",L"Right trigger: fire. Left grip: stable aim. One-hand aim sways. Reload stays native.",0);
        staffHint=Control(L"STATIC",L"0 weight / stabilization = direct tracking.\nSave settings applies live.",0);
        hint=Control(L"STATIC",L"Default controls (edit in Controller mapping):\nR3: radial; right stick: select; A: confirm; X: cancel; B: page. X: inventory; B: weapon.\nL3: sprint until stop. Double left trigger: heal. Right stick up: jump. Down: sneak. Menu: back. L3+R3 (0.5 s): recenter.\nNear clipping distance applies only to first-person VR. Save settings to apply live.",0);
        status=Control(L"STATIC",L"Connect your headset through the selected OpenXR runtime before launching.",0);
        reset=Control(L"BUTTON",L"Defaults",WS_TABSTOP,Reset);save=Control(L"BUTTON",L"Save settings",WS_TABSTOP,Save);
        launch=Control(L"BUTTON",L"Save and launch",WS_TABSTOP|BS_DEFPUSHBUTTON,Launch);CreateNativeIniControls();CreateControllerMapper();SetTimer(h,MapperTimer,100,nullptr);Fill();Layout();return 0;
    case WM_TIMER:if(w==MapperTimer){TickControllerMapper();return 0;}break;
    case WM_SIZE:Layout();return 0;
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT p;HDC dc=BeginPaint(h,&p);PaintCanvas(dc);EndPaint(h,&p);return 0;}
    case WM_PRINTCLIENT:PaintCanvas(HDC(w));return 0;
    case WM_DRAWITEM:if(w){DrawButton(*reinterpret_cast<DRAWITEMSTRUCT*>(l));return TRUE;}break;
    case WM_NOTIFY:if(reinterpret_cast<NMHDR*>(l)->code==NM_CUSTOMDRAW)return DrawTrack(*reinterpret_cast<NMCUSTOMDRAW*>(l));if(reinterpret_cast<NMHDR*>(l)->idFrom==Tabs&&reinterpret_cast<NMHDR*>(l)->code==TCN_SELCHANGE){Layout();return 0;}break;
    case WM_GETMINMAXINFO:{auto* info=reinterpret_cast<MINMAXINFO*>(l);info->ptMinTrackSize={Scale(1180),Scale(810)};return 0;}
    case WM_DPICHANGED:{dpi=HIWORD(w);Fonts();EnumChildWindows(h,[](HWND child,LPARAM)->BOOL{SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(child==title?titleFont:font),TRUE);return TRUE;},0);
        auto* r=reinterpret_cast<RECT*>(l);SetWindowPos(h,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);Layout();return 0;}
    case WM_HSCROLL:
        for(unsigned i=0;i<diagnostic::ini_tweaks::Count;++i)if(iniTracks[i]&&HWND(l)==iniTracks[i]){syncing=true;int value=diagnostic::ini_tweaks::Snap(i,int(SendMessageW(iniTracks[i],TBM_GETPOS,0,0)));SendMessageW(iniTracks[i],TBM_SETPOS,TRUE,value);SetDlgItemInt(h,IniValueBase+i,value,FALSE);nativeIni.values[i]=value;syncing=false;nativeIniDirty=true;Message(L"INI tweaks changed. Close Gothic 3, then Save settings; restart required.");return 0;}
        if(HWND(l)==ffrTrack){syncing=true;int value=(int(SendMessageW(ffrTrack,TBM_GETPOS,0,0))+2)/5*5;SendMessageW(ffrTrack,TBM_SETPOS,TRUE,value);SetDlgItemInt(h,FfrEdit,value,FALSE);syncing=false;dirty=true;Message(L"Save settings to apply the FFR diameter live (DXVK FFR backend required).");return 0;}
        if(HWND(l)==ffrTransitionTrack){syncing=true;SetDlgItemInt(h,FfrTransitionEdit,UINT(SendMessageW(ffrTransitionTrack,TBM_GETPOS,0,0)),FALSE);syncing=false;dirty=true;Message(L"Save settings to apply the FFR blur transition live.");return 0;}
        for(int i=0;i<2;++i)if(HWND(l)==graphicsTracks[i]){syncing=true;int value=(int(SendMessageW(graphicsTracks[i],TBM_GETPOS,0,0))+2)/5*5;SendMessageW(graphicsTracks[i],TBM_SETPOS,TRUE,value);SetDlgItemInt(h,GraphicsEdit+i,value,FALSE);syncing=false;dirty=true;GraphicsPreview();Message(L"Graphics changes apply after restarting the game.");return 0;}
        if(HWND(l)==aimTrack){syncing=true;SetDlgItemInt(h,AimEdit,UINT(SendMessageW(aimTrack,TBM_GETPOS,0,0)),TRUE);syncing=false;dirty=true;Message(L"Selection changed. Save settings to apply in-game.");return 0;}
        for(unsigned i=0;i<diagnostic::VrNumberCount;++i)if(HWND(l)==tracks[i]){syncing=true;int snapped=diagnostic::BoundVrNumber(diagnostic::VrNumbers[i],int(SendMessageW(tracks[i],TBM_GETPOS,0,0)));SendMessageW(tracks[i],TBM_SETPOS,TRUE,snapped);SetDlgItemInt(h,EditBase+i,UINT(snapped),TRUE);syncing=false;dirty=true;Message(L"Settings changed. Save to apply.");}return 0;
    case WM_COMMAND:
        if(LOWORD(w)>=NavBase&&LOWORD(w)<NavBase+PageCount&&HIWORD(w)==BN_CLICKED){int page=LOWORD(w)-NavBase;if(TabCtrl_GetCurSel(settingsTabs)!=page){TabCtrl_SetCurSel(settingsTabs,page);Layout();}return 0;}
        if(HIWORD(w)==BN_CLICKED&&l&&GetPropW(HWND(l),L"Toggle")){SendMessageW(HWND(l),BM_SETCHECK,SendMessageW(HWND(l),BM_GETCHECK,0,0)==BST_CHECKED?BST_UNCHECKED:BST_CHECKED,0);}
        if(ControllerMapperCommand(LOWORD(w),HIWORD(w)))return 0;
        if(HIWORD(w)==EN_CHANGE&&!syncing&&LOWORD(w)==FfrEdit){BOOL valid=FALSE;int value=int(GetDlgItemInt(h,FfrEdit,&valid,FALSE));if(valid)SendMessageW(ffrTrack,TBM_SETPOS,TRUE,value);dirty=true;return 0;}
        if(HIWORD(w)==EN_CHANGE&&!syncing&&LOWORD(w)==FfrTransitionEdit){BOOL valid=FALSE;int value=int(GetDlgItemInt(h,FfrTransitionEdit,&valid,FALSE));if(valid)SendMessageW(ffrTransitionTrack,TBM_SETPOS,TRUE,value);dirty=true;return 0;}
        if(HIWORD(w)==EN_CHANGE&&!syncing&&LOWORD(w)>=GraphicsEdit&&LOWORD(w)<GraphicsEdit+2){int i=LOWORD(w)-GraphicsEdit;BOOL valid=FALSE;int value=int(GetDlgItemInt(h,GraphicsEdit+i,&valid,FALSE));if(valid)SendMessageW(graphicsTracks[i],TBM_SETPOS,TRUE,value);dirty=true;GraphicsPreview();return 0;}
        if(HIWORD(w)==EN_CHANGE&&!syncing&&LOWORD(w)==AimEdit){BOOL valid=FALSE;int value=int(GetDlgItemInt(h,AimEdit,&valid,TRUE));if(valid)SendMessageW(aimTrack,TBM_SETPOS,TRUE,value);dirty=true;return 0;}
        if(HIWORD(w)==EN_CHANGE&&!syncing&&LOWORD(w)>=EditBase&&LOWORD(w)<EditBase+diagnostic::VrNumberCount){unsigned i=LOWORD(w)-EditBase;BOOL valid=FALSE;int n=int(GetDlgItemInt(h,EditBase+i,&valid,TRUE));if(valid)SendMessageW(tracks[i],TBM_SETPOS,TRUE,n);dirty=true;return 0;}
        if(LOWORD(w)>=IniValueBase&&LOWORD(w)<IniValueBase+diagnostic::ini_tweaks::Count&&!syncing){unsigned i=LOWORD(w)-IniValueBase;auto kind=diagnostic::ini_tweaks::Fields[i].kind;
            if((kind==diagnostic::ini_tweaks::Kind::Number&&HIWORD(w)==EN_CHANGE)||(kind==diagnostic::ini_tweaks::Kind::Choice&&HIWORD(w)==CBN_SELCHANGE)||(kind==diagnostic::ini_tweaks::Kind::Toggle&&HIWORD(w)==BN_CLICKED)){
                if(kind==diagnostic::ini_tweaks::Kind::Number){BOOL valid=FALSE;int value=int(GetDlgItemInt(h,IniValueBase+i,&valid,TRUE));if(valid){nativeIni.values[i]=value;SendMessageW(iniTracks[i],TBM_SETPOS,TRUE,value);}}
                nativeIniDirty=true;Message(L"INI tweaks changed. Close Gothic 3, then Save settings; restart required.");return 0;}}
        switch(LOWORD(w)){case Browse:ChooseGame();break;case Save:SaveSettings();break;case IDOK:case Launch:LaunchGame();break;case IDCANCEL:SendMessageW(h,WM_CLOSE,0,0);break;
        case UnionBrowse:ChooseUnion();break;case UnionOpen:OpenUnion(false);break;case UnionCheck:OpenUnion(true);break;
        case UnionNotes:OpenModNotes();break;
        case UnionPage:TabCtrl_SetCurSel(settingsTabs,ModsPage);Layout();SetFocus(unionOpen);break;
        case FfrMethod:if(HIWORD(w)==CBN_SELCHANGE&&!syncing){int mode=int(SendMessageW(ffrCombo,CB_GETCURSEL,0,0));if(mode>0){syncing=true;int value=mode==1?75:mode==2?60:45;SendMessageW(ffrTrack,TBM_SETPOS,TRUE,value);SetDlgItemInt(h,FfrEdit,value,FALSE);SendMessageW(graphicsCombo,CB_SETCURSEL,0,0);syncing=false;GraphicsPreview();}dirty=true;Message(L"FFR preset selected. Adjust diameter as needed, then Save settings to apply live (backend required).");}break;
        case GraphicsMethod:case Resolution:if(HIWORD(w)==CBN_SELCHANGE&&!syncing){GraphicsPreview();dirty=true;Message(L"Resolution will apply at the next game launch.");}break;
        case IniReload:LoadNativeIni();Message(L"Native INIs reloaded. Unsaved INI edits were discarded; VR calibration is unchanged.");break;
        case IniBackups:OpenIniBackups();break;
        case Reset:if(TabCtrl_GetCurSel(settingsTabs)==GameIniPage){
            auto defaults=nativeIni;diagnostic::ini_tweaks::Defaults(defaults);
            for(unsigned i=0;i<3;++i)if(nativeIni.available[i])nativeIni.values[i]=defaults.values[i];
            nativeIni.forceNativeDistances=nativeIni.forceNativeDistances||defaults.forceNativeDistances;
            FillNativeIni();nativeIniDirty=nativeIni.Changed();settings.numbers[85]=100;
            SendMessageW(tracks[85],TBM_SETPOS,TRUE,100);SetDlgItemInt(h,EditBase+85,100,FALSE);dirty=true;
            Message(L"Fog 100% and native distance defaults staged. Save settings to apply.");break;}
        if(TabCtrl_GetCurSel(settingsTabs)==IniTweaksPage){diagnostic::ini_tweaks::Defaults(nativeIni);FillNativeIni();nativeIniDirty=nativeIni.Changed();Message(L"INI tweak defaults staged. Save settings to apply.");break;}settings=diagnostic::VrSettings{};Fill();dirty=true;Message(L"Defaults restored. Save to apply.");break;
        case AntiAliasing:if(HIWORD(w)==CBN_SELCHANGE&&!syncing){dirty=true;Message(L"Anti-aliasing applies after restarting the game. HUD and menus remain unfiltered.");}break;
        case EffectsQuality:if(HIWORD(w)==CBN_SELCHANGE&&!syncing){dirty=true;Message(L"Fire & magic effect quality applies live with Save settings. Original restores native visuals.");}break;
        case RuntimeChoice:if(HIWORD(w)==CBN_SELCHANGE&&!syncing){dirty=true;Message(L"Runtime changes apply after restarting the game. The Windows default runtime is unchanged.");}break;
        case FastPickup:case DialogueView:case ChestView:case LecternView:case ThirdPersonOrbit:case HandHud:case ShowCompass:case ShowSlotBar:case ShowVitalsPanel:case BowReticle:case TwoHand:case Heavy:case HandsOnly:case ToggleSupport:dirty=true;break;}return 0;
    case WM_CTLCOLOREDIT:case WM_CTLCOLORLISTBOX:SetBkColor(HDC(w),Field);SetTextColor(HDC(w),Ink);return reinterpret_cast<LRESULT>(editBrush);
    case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:{
        wchar_t cls[64];GetClassNameW(HWND(l),cls,64);bool edit=!wcscmp(cls,L"Edit");SetBkMode(HDC(w),edit?OPAQUE:TRANSPARENT);SetBkColor(HDC(w),edit?Field:Card);SetTextColor(HDC(w),HWND(l)==subtitle||HWND(l)==status||HWND(l)==hint?Muted:Ink);
        if(edit)return reinterpret_cast<LRESULT>(editBrush);
        if(!wcscmp(cls,TRACKBAR_CLASSW))return reinterpret_cast<LRESULT>(cardBrush);
        return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));}

    case WM_CLOSE:
        StopControllerCapture();
        if(dirty||nativeIniDirty){int choice=MessageBoxW(h,L"Save changed settings before closing?",L"Gothic 3 VR",MB_YESNOCANCEL|MB_ICONQUESTION);
            if(choice==IDCANCEL||(choice==IDYES&&!SaveSettings()))return 0;}DestroyWindow(h);return 0;
    case WM_DESTROY:StopControllerCapture();KillTimer(h,MapperTimer);PostQuitMessage(0);return 0;
    }return DefWindowProcW(h,msg,w,l);
}
int SelfTest() {
    wchar_t path[MAX_PATH];GetModuleFileNameW(nullptr,path,MAX_PATH);PROCESS_INFORMATION process{};
    if(!diagnostic::StartExecutable(path,L"--launch-fixture",process))return 1;
    DWORD wait=WaitForSingleObject(process.hProcess,10000),code=1;GetExitCodeProcess(process.hProcess,&code);
    CloseHandle(process.hThread);CloseHandle(process.hProcess);return wait==WAIT_OBJECT_0&&code==42?0:1;
}
bool UiCheck(wchar_t const* path) {
    RECT client;GetClientRect(window,&client);bool fits=true;
    EnumChildWindows(window,[](HWND child,LPARAM context)->BOOL{
        RECT r,c;GetWindowRect(child,&r);MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&r),2);GetClientRect(window,&c);
        if(IsWindowVisible(child)&&(r.left<0||r.top<0||r.right>c.right||r.bottom>c.bottom))*reinterpret_cast<bool*>(context)=false;return TRUE;
    },reinterpret_cast<LPARAM>(&fits));
    std::wstring report;
    for(HWND child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){
        if(!IsWindowVisible(child))continue;
        // Exercise the intermediate erase stage, not just the final styled snapshot.
        {RECT r;GetClientRect(child,&r);HDC dc=GetDC(child),memory=CreateCompatibleDC(dc);auto bitmap=CreateCompatibleBitmap(dc,r.right,r.bottom);auto old=SelectObject(memory,bitmap);PatBlt(memory,0,0,r.right,r.bottom,WHITENESS);SendMessageW(child,WM_ERASEBKGND,WPARAM(memory),0);auto pixel=GetPixel(memory,r.right/2,r.bottom/2);if(pixel==RGB(255,255,255)){fits=false;report+=L"Unpainted erase stage: "+std::to_wstring(GetDlgCtrlID(child))+L"\r\n";}SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(child,dc);}
        wchar_t cls[64];GetClassNameW(child,cls,64);
        if(!_wcsicmp(cls,TRACKBAR_CLASSW)){
            int original=int(SendMessageW(child,TBM_GETPOS,0,0)),minimum=int(SendMessageW(child,TBM_GETRANGEMIN,0,0)),maximum=int(SendMessageW(child,TBM_GETRANGEMAX,0,0));RECT client;GetClientRect(child,&client);
            // Exercise movement in both directions without a full page repaint.
            // Each step must invalidate the whole custom thumb, including its edges.
            for(int value:{minimum,maximum,original}){ValidateRect(child,nullptr);SendMessageW(child,TBM_SETPOS,TRUE,value);RECT update{};if(!GetUpdateRect(child,&update,FALSE)||!EqualRect(&update,&client)){fits=false;report+=L"Partial slider update: "+std::to_wstring(GetDlgCtrlID(child))+L"\r\n";}UpdateWindow(child);}
        }
        if(wcscmp(cls,L"Static"))continue;
        RECT r;GetClientRect(child,&r);int available=r.bottom;HDC measure=GetDC(child);auto old=SelectObject(measure,HFONT(SendMessageW(child,WM_GETFONT,0,0)));wchar_t text[2048];GetWindowTextW(child,text,2048);DrawTextW(measure,text,-1,&r,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);SelectObject(measure,old);ReleaseDC(child,measure);
        if(r.bottom>available){fits=false;report+=L"Clipped text: "+std::wstring(text)+L"\r\n";}
    }
    {auto log=CreateFileW((std::wstring(path)+L".txt").c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(log!=INVALID_HANDLE_VALUE){DWORD written;std::wstring message=report.empty()?L"PASS: visible controls fit; text is not clipped.\r\n":report;WriteFile(log,message.data(),DWORD(message.size()*sizeof(wchar_t)),&written,nullptr);CloseHandle(log);}}
    HDC dc=GetDC(window),memory=CreateCompatibleDC(dc);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=client.right;info.bmiHeader.biHeight=-client.bottom;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);auto old=SelectObject(memory,bitmap);
    bool rendered=PrintWindow(window,memory,PW_CLIENTONLY)!=FALSE;
    DWORD size=client.right*client.bottom*4;BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);header.bfSize=header.bfOffBits+size;
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);DWORD written;
    bool saved=file!=INVALID_HANDLE_VALUE&&WriteFile(file,&header,sizeof(header),&written,nullptr)&&WriteFile(file,&info.bmiHeader,sizeof(info.bmiHeader),&written,nullptr)&&WriteFile(file,pixels,size,&written,nullptr);
    if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window,dc);return fits&&rendered&&saved;
}
bool UiInteractionCheck(wchar_t const* path){
    std::wstring report;
    auto original=settings;bool ok=artwork&&gdiplusToken;
    TabCtrl_SetCurSel(settingsTabs,FoveatedPage);Layout();
    for(int width:{0,15,30}){SendMessageW(ffrTransitionTrack,TBM_SETPOS,TRUE,width);SendMessageW(window,WM_HSCROLL,TB_THUMBPOSITION,LPARAM(ffrTransitionTrack));ok=Collect()&&ok;ok=ok&&settings.ffrTransitionPercent==width;}
    settings=original;Fill();
    for(HWND check:{handHudCheck,compassCheck,slotBarCheck,vitalsPanelCheck,bowReticleCheck,heavyCheck,twoHandCheck,handsOnlyCheck,toggleSupportCheck}){
        TabCtrl_SetCurSel(settingsTabs,check==handHudCheck?HandHudPage:check==compassCheck||check==slotBarCheck||check==vitalsPanelCheck?HudPage:check==bowReticleCheck?BowPage:check==heavyCheck||check==twoHandCheck?TwoHandPage:CameraPage);Layout();
        auto before=SendMessageW(check,BM_GETCHECK,0,0);SendMessageW(check,BM_CLICK,0,0);if(SendMessageW(check,BM_GETCHECK,0,0)==before){report+=L"Toggle failed: "+std::to_wstring(GetDlgCtrlID(check))+L"\r\n";ok=false;}
        if(check==handHudCheck){ok=Collect()&&ok;ok=ok&&settings.handHudEnabled==(before!=BST_CHECKED);}
        if(check==compassCheck){ok=Collect()&&ok;ok=ok&&settings.hideCompass==(before==BST_CHECKED);}
        if(check==slotBarCheck){ok=Collect()&&ok;ok=ok&&settings.showSlotBar==(before!=BST_CHECKED);}
        if(check==vitalsPanelCheck){ok=Collect()&&ok;ok=ok&&settings.showVitalsPanel==(before!=BST_CHECKED);}
        if(check==handsOnlyCheck){ok=Collect()&&ok;ok=ok&&settings.handsOnly==(before!=BST_CHECKED);}
        if(check==toggleSupportCheck){ok=Collect()&&ok;ok=ok&&settings.toggleSupportGrip==(before!=BST_CHECKED);}
        SendMessageW(check,BM_CLICK,0,0);ok=ok&&SendMessageW(check,BM_GETCHECK,0,0)==before;
    }
    for(auto control:{fastPickupCheck,dialogueViewCheck,chestViewCheck,lecternViewCheck,thirdPersonOrbitCheck}){
        TabCtrl_SetCurSel(settingsTabs,control==fastPickupCheck?InteractionPage:ThirdPersonPage);Layout();
        bool original=SendMessageW(control,BM_GETCHECK,0,0)==BST_CHECKED;
        for(unsigned click=0;click<2;++click){SendMessageW(control,BM_CLICK,0,0);ok=Collect()&&ok;
            bool value=control==fastPickupCheck?settings.fastPickup:control==dialogueViewCheck?settings.firstPersonDialogue:
                control==chestViewCheck?settings.firstPersonChest:control==lecternViewCheck?settings.firstPersonLectern:settings.thirdPersonOrbit;
            ok=ok&&value==(click?original:!original);
        }
    }
    settings.dlaaExperimental=true; // A stale retired flag must not disable active graphics options.
    ok=Collect()&&ok;
    ok=ok&&!settings.dlaaExperimental&&settings.ffrMode==original.ffrMode&&settings.upscaleMethod==original.upscaleMethod;
    ok=ok&&settings.fxaaEnabled==original.fxaaEnabled&&settings.smaaEnabled==original.smaaEnabled;
    for(int effects=0;effects<3;++effects){SendMessageW(effectsCombo,CB_SETCURSEL,effects,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(EffectsQuality,CBN_SELCHANGE),LPARAM(effectsCombo));ok=Collect()&&ok;ok=ok&&settings.particleQuality==effects;}
    SendMessageW(effectsCombo,CB_SETCURSEL,original.particleQuality,0);
    for(int aa=0;aa<5;++aa){SendMessageW(aaCombo,CB_SETCURSEL,aa,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(AntiAliasing,CBN_SELCHANGE),LPARAM(aaCombo));ok=Collect()&&ok;ok=ok&&settings.fxaaEnabled==(aa==1)&&settings.smaaEnabled==(aa>=2)&&(!settings.smaaEnabled||settings.smaaQuality==aa-2);}
    SendMessageW(aaCombo,CB_SETCURSEL,original.smaaEnabled?2+diagnostic::ValidSmaaQuality(original.smaaQuality):original.fxaaEnabled?1:0,0);
    for(int choice=0;choice<4;++choice){SendMessageW(runtimeCombo,CB_SETCURSEL,choice,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(RuntimeChoice,CBN_SELCHANGE),LPARAM(runtimeCombo));ok=Collect()&&ok;ok=ok&&int(settings.openXrRuntime)==choice;}
    SendMessageW(runtimeCombo,CB_SETCURSEL,int(original.openXrRuntime),0);
    SendMessageW(ffrCombo,CB_SETCURSEL,2,0);SendMessageW(graphicsCombo,CB_SETCURSEL,0,0);settings.dlaaExperimental=true;ok=Collect()&&ok;ok=ok&&!settings.dlaaExperimental&&settings.ffrMode==2&&settings.upscaleMethod==diagnostic::UpscaleMethod::Off;
    SendMessageW(ffrCombo,CB_SETCURSEL,0,0);SendMessageW(graphicsCombo,CB_SETCURSEL,1,0);settings.dlaaExperimental=true;ok=Collect()&&ok;ok=ok&&!settings.dlaaExperimental&&settings.ffrMode==0&&int(settings.upscaleMethod)==1;
    settings=original;Fill();ok=Collect()&&ok;
    for(unsigned i=0;i<diagnostic::VrNumberCount;++i)if(settings.numbers[i]!=original.numbers[i]){report+=L"Value mismatch: "+std::to_wstring(i)+L"\r\n";ok=false;}
    ok=ok&&settings.autoStart==original.autoStart&&settings.trackedArms==original.trackedArms&&settings.controllers==original.controllers&&settings.twoHandedMelee==original.twoHandedMelee&&settings.bowReticle==original.bowReticle&&settings.hideCompass==original.hideCompass&&settings.showSlotBar==original.showSlotBar&&settings.showVitalsPanel==original.showVitalsPanel&&settings.handsOnly==original.handsOnly&&settings.toggleSupportGrip==original.toggleSupportGrip;
    settings=original;Fill();dirty=false;
    TabCtrl_SetCurSel(settingsTabs,CameraPage);Layout();auto before=settings.numbers[0];int next=before<diagnostic::VrNumbers[0].maximum?before+1:before-1;SendMessageW(tracks[0],TBM_SETPOS,TRUE,next);SendMessageW(window,WM_HSCROLL,TB_THUMBPOSITION,LPARAM(tracks[0]));BOOL valid=FALSE;ok=ok&&int(GetDlgItemInt(window,EditBase,&valid,TRUE))==next&&valid;settings=original;Fill();dirty=false;
    if(nativeIni.ready&&nativeIni.available[5]){
        auto nativeOriginal=nativeIni;bool nativeWasDirty=nativeIniDirty;
        auto gameBefore=diagnostic::ini_tweaks::Bytes(nativeIni.game),userBefore=diagnostic::ini_tweaks::Bytes(nativeIni.user);
        SendMessageW(navigation[IniTweaksPage],BM_CLICK,0,0);ok=NativeTooltipCheck(report)&&ok;
        int next=diagnostic::ini_tweaks::Snap(5,nativeIni.values[5]+250);SendMessageW(iniTracks[5],TBM_SETPOS,TRUE,next);SendMessageW(window,WM_HSCROLL,TB_THUMBPOSITION,LPARAM(iniTracks[5]));
        BOOL valid=FALSE;ok=ok&&nativeIniDirty&&int(GetDlgItemInt(window,IniValueBase+5,&valid,FALSE))==next&&valid;
        bool previous=SendMessageW(iniValues[10],BM_GETCHECK,0,0)==BST_CHECKED;SendMessageW(iniValues[10],BM_CLICK,0,0);
        ok=ok&&(SendMessageW(iniValues[10],BM_GETCHECK,0,0)==BST_CHECKED)!=previous;
        ok=CollectNativeIni()&&ok;
        SendMessageW(reset,BM_CLICK,0,0);for(unsigned i=0;i<diagnostic::VrNumberCount;++i)ok=ok&&settings.numbers[i]==original.numbers[i];
        SendMessageW(iniReload,BM_CLICK,0,0);ok=ok&&!nativeIniDirty&&nativeIni.values==nativeOriginal.values;
        ok=ok&&diagnostic::ini_tweaks::Bytes(nativeIni.game)==gameBefore&&diagnostic::ini_tweaks::Bytes(nativeIni.user)==userBefore;
        SendMessageW(navigation[GameIniPage],BM_CLICK,0,0);ok=NativeTooltipCheck(report)&&ok;
        for(unsigned i=0;i<3;++i)if(nativeIni.available[i]){
            int next=diagnostic::ini_tweaks::Snap(i,nativeIni.values[i]+diagnostic::ini_tweaks::Fields[i].step);
            SendMessageW(iniTracks[i],TBM_SETPOS,TRUE,next);SendMessageW(window,WM_HSCROLL,TB_THUMBPOSITION,LPARAM(iniTracks[i]));
            ok=ok&&int(GetDlgItemInt(window,IniValueBase+i,nullptr,TRUE))==next;
        }
        SetDlgItemInt(window,EditBase+85,30,FALSE);ok=Collect()&&ok;ok=ok&&settings.numbers[85]==30;
        auto staged=nativeIni;SendMessageW(reset,BM_CLICK,0,0);ok=ok&&settings.numbers[85]==100;
        auto expected=nativeOriginal;diagnostic::ini_tweaks::Defaults(expected);
        for(unsigned i=0;i<3;++i)if(nativeIni.available[i])ok=ok&&nativeIni.values[i]==expected.values[i];
        for(unsigned i=3;i<diagnostic::ini_tweaks::Count;++i)ok=ok&&nativeIni.values[i]==staged.values[i];
        ok=ok&&diagnostic::ini_tweaks::Bytes(nativeIni.game)==gameBefore&&diagnostic::ini_tweaks::Bytes(nativeIni.user)==userBefore;
        settings=original;Fill();dirty=false;
        nativeIni=nativeOriginal;nativeIniDirty=nativeWasDirty;FillNativeIni();
    }
    {SendMessageW(navigation[ModsPage],BM_CLICK,0,0);ok=ok&&IsWindowVisible(unionHint)&&IsWindowVisible(unionOpen)&&!GetDlgItem(window,710)&&!GetDlgItem(window,711)&&!GetDlgItem(window,714);}
    {auto saved=settings.controllerBindings;bool wasDirty=dirty;
      SendMessageW(navigation[BindingsPage],BM_CLICK,0,0);
      ok=ok&&IsWindowVisible(mappingAction)&&IsWindowVisible(mappingListen);
      SendMessageW(mappingJump,BM_CLICK,0,0);
      ok=ok&&mappingSelected==diagnostic::mapping::Jump&&IsWindowVisible(mappingLeftHanded);
      SendMessageW(mappingLeftHanded,BM_CLICK,0,0);ok=ok&&settings.controllerBindings.leftHanded!=saved.leftHanded;
      SendMessageW(mappingLeftHanded,BM_CLICK,0,0);ok=ok&&settings.controllerBindings.leftHanded==saved.leftHanded;
      SendMessageW(mappingAction,CB_SETCURSEL,diagnostic::mapping::Heal,0);FillControllerMapper();
      diagnostic::mapping::Link source;bool opened=source.Open(Ini().c_str(),true);
      diagnostic::ControllerFrame raw;raw.focused=true;raw.timeMs=GetTickCount64();raw.availableButtons=0xfff;
      source.Publish(raw,0);ControllerMapperCommand(MapperListen,BN_CLICKED);TickControllerMapper();
      source.Publish(raw,diagnostic::mapping::Bit(diagnostic::mapping::LeftGrip)|diagnostic::mapping::Bit(diagnostic::mapping::RightPrimary));TickControllerMapper();
      source.Publish(raw,0);TickControllerMapper();
      auto binding=settings.controllerBindings.bindings[diagnostic::mapping::Heal];
      ok=ok&&opened&&!mappingListening&&diagnostic::mapping::Mask(binding)==(diagnostic::mapping::Bit(diagnostic::mapping::LeftGrip)|diagnostic::mapping::Bit(diagnostic::mapping::RightPrimary))&&binding.gesture==diagnostic::mapping::DoubleTap&&!source.Capturing(GetTickCount64());
      StopControllerCapture();mappingLink.Close();source.Close();settings.controllerBindings=saved;dirty=wasDirty;FillControllerMapper();
    }
    ok=ok&&PageCount==21&&TabCtrl_GetItemCount(settingsTabs)==PageCount;
    for(int page=0;page<PageCount;++page){SendMessageW(navigation[page],BM_CLICK,0,0);ok=ok&&TabCtrl_GetCurSel(settingsTabs)==page;
        ok=ok&&(IsWindowVisible(fastPickupCheck)!=FALSE)==(page==InteractionPage);
        for(unsigned i=0;i<diagnostic::ini_tweaks::Count;++i)
            ok=ok&&(IsWindowVisible(iniValues[i])!=FALSE)==(page==(i<3?GameIniPage:IniTweaksPage));
        for(auto control:{dialogueViewCheck,chestViewCheck,lecternViewCheck,thirdPersonOrbitCheck})
            ok=ok&&(IsWindowVisible(control)!=FALSE)==(page==ThirdPersonPage);
        ok=ok&&PageNames[page]&&*PageNames[page]&&PageDescriptions[page]&&*PageDescriptions[page];
        ok=ok&&(IsWindowVisible(handsOnlyCheck)!=FALSE)==(page==CameraPage);
        ok=ok&&(IsWindowVisible(toggleSupportCheck)!=FALSE)==(page==CameraPage);
        for(unsigned i=0;i<diagnostic::VrNumberCount;++i){bool expected=NumberPage(i)==page||(page==MagicPage&&i==28);
            if(bool(IsWindowVisible(edits[i]))!=expected||bool(IsWindowVisible(tracks[i]))!=expected){ok=false;report+=L"Incorrect field page: "+std::to_wstring(i)+L" on "+std::to_wstring(page)+L"\r\n";}}
    }
    TabCtrl_SetCurSel(settingsTabs,CameraPage);Layout();auto log=CreateFileW((std::wstring(path)+L".interaction.txt").c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(log!=INVALID_HANDLE_VALUE){DWORD written;if(report.empty())report=ok?L"PASS":L"FAILED";WriteFile(log,report.data(),DWORD(report.size()*sizeof(wchar_t)),&written,nullptr);CloseHandle(log);}return ok;
}
bool UiIdlePaintCheck(wchar_t const* path){
    RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_NOERASE);
    checkingIdlePaint=true;idlePaintCount=0;auto start=GetTickCount64();MSG message{};
    while(GetTickCount64()-start<300){if(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}else MsgWaitForMultipleObjectsEx(0,nullptr,10,QS_ALLINPUT,MWMO_INPUTAVAILABLE);}
    checkingIdlePaint=false;bool ok=idlePaintCount<=3;
    auto log=CreateFileW((std::wstring(path)+L".idle.txt").c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(log!=INVALID_HANDLE_VALUE){DWORD written;auto text=std::wstring(ok?L"PASS":L"FAIL")+L": idle slider paints in 300 ms: "+std::to_wstring(idlePaintCount)+L"\r\n";WriteFile(log,text.data(),DWORD(text.size()*sizeof(wchar_t)),&written,nullptr);CloseHandle(log);}return ok;
}
}
int WINAPI wWinMain(HINSTANCE module,HINSTANCE,LPWSTR,int show) {
    int count=0;auto args=CommandLineToArgvW(GetCommandLineW(),&count);
    if(count>1&&!wcscmp(args[1],L"--launch-fixture")){wchar_t self[MAX_PATH],cwd[MAX_PATH];GetModuleFileNameW(nullptr,self,MAX_PATH);GetCurrentDirectoryW(MAX_PATH,cwd);
        bool correct=!_wcsicmp(diagnostic::ParentDirectory(self).c_str(),cwd);LocalFree(args);return correct?42:1;}
    if(count>1&&!wcscmp(args[1],L"--self-test")){LocalFree(args);return SelfTest();}
    std::wstring uiCheckPath;int checkWidth=1320,checkHeight=940;UINT checkDpi=0;if(count>2&&!wcscmp(args[1],L"--ui-check")){uiCheckPath=args[2];if(count>4){checkWidth=_wtoi(args[3]);checkHeight=_wtoi(args[4]);}if(count>5)checkDpi=UINT(_wtoi(args[5]));}LocalFree(args);
    instance=module;SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    dpi=checkDpi?checkDpi:GetDpiForSystem();RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);if(uiCheckPath.empty())dpi=std::min(dpi,UINT(MulDiv(work.bottom-work.top-24,96,810)));INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_BAR_CLASSES|ICC_TAB_CLASSES};InitCommonControlsEx(&common);
    wchar_t self[MAX_PATH],game[MAX_PATH];GetModuleFileNameW(nullptr,self,MAX_PATH);launcherIni=diagnostic::ParentDirectory(self)+L"\\Gothic3VRLauncher.ini";
    auto adjacentGame=diagnostic::ParentDirectory(self)+L"\\Gothic3.exe";GetPrivateProfileStringW(L"Launcher",L"GameExecutable",adjacentGame.c_str(),game,MAX_PATH,launcherIni.c_str());executable=game;
    settings=diagnostic::LoadVrSettings(Ini().c_str());background=CreateSolidBrush(RGB(12,15,20));InitializeArtwork();
    WNDCLASSW cls{};cls.hInstance=module;cls.lpfnWndProc=Procedure;cls.lpszClassName=L"Gothic3VRLauncher";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=background;cls.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(101));RegisterClassW(&cls);
    int width=Scale(checkWidth),height=Scale(checkHeight);if(uiCheckPath.empty()){width=std::min(width,int(work.right-work.left-24));height=std::min(height,int(work.bottom-work.top-24));}
    window=CreateWindowExW(WS_EX_COMPOSITED,cls.lpszClassName,L"Gothic 3 VR — Settings and launcher",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,work.left+std::max(0,int(work.right-work.left-width)/2),work.top+std::max(0,int(work.bottom-work.top-height)/2),width,height,nullptr,nullptr,module,nullptr);
    if(window&&!uiCheckPath.empty()){ShowWindow(window,SW_SHOWNOACTIVATE);UpdateWindow(window);bool ok=UiInteractionCheck(uiCheckPath.c_str());ok=UiCheck(uiCheckPath.c_str())&&ok;
        wchar_t const* suffix[]={L"",L"-hud.bmp",L"-interaction.bmp",L"-left-hand.bmp",L"-right-hand.bmp",L"-sword.bmp",L"-bow.bmp",L"-two-hand.bmp",L"-staff.bmp",L"-crossbow.bmp",L"-magic.bmp",L"-radial.bmp",L"-shield.bmp",L"-graphics.bmp",L"-foveated.bmp",L"-mods.bmp",L"-ini-tweaks.bmp",L"-controller-mapping.bmp",L"-hand-hud.bmp",L"-third-person.bmp",L"-game-ini.bmp"};
        for(int page=1;page<int(sizeof(suffix)/sizeof(suffix[0]));++page){TabCtrl_SetCurSel(settingsTabs,page);Layout();UpdateWindow(window);ok=UiCheck((uiCheckPath+suffix[page]).c_str())&&ok;}
        TabCtrl_SetCurSel(settingsTabs,CameraPage);Layout();ok=UiIdlePaintCheck(uiCheckPath.c_str())&&ok;
        DestroyWindow(window);for(HFONT f:{font,titleFont,smallFont,navFont,brandFont,sectionFont})DeleteObject(f);DeleteObject(background);ReleaseArtwork();return ok?0:1;}
    if(!window)return 1;ShowWindow(window,show);UpdateWindow(window);MSG message{};
    while(GetMessageW(&message,nullptr,0,0)>0){RelayNativeTooltip(message);if(!(IsWindow(modNotesWindow)&&IsDialogMessageW(modNotesWindow,&message))&&!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
    for(HFONT f:{font,titleFont,smallFont,navFont,brandFont,sectionFont})DeleteObject(f);DeleteObject(background);ReleaseArtwork();return int(message.wParam);
}


