#include "VrSettings.h"
#include "HudVisibility.h"
#include "LauncherFixedFeatures.h"
#include <cstdio>
#include <string>
int wmain(int argc, wchar_t** argv) {
    static_assert(diagnostic::VrNumberCount==101);
    unsigned failures=0;auto check=[&](bool ok,char const* name){printf("%s %s\n",ok?"PASS":"FAIL",name);failures+=!ok;};
    wchar_t directory[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,directory);GetTempFileNameW(directory,L"gvs",0,path);
    auto missingHandHud=diagnostic::LoadVrSettings(path);
    check(missingHandHud.numbers[85]==100&&diagnostic::VrSettings{}.numbers[85]==100,
        "New and missing-key configurations preserve native fog at 100 percent");
    for(int fog:{0,30,100}){auto value=diagnostic::VrSettings{};value.numbers[85]=fog;value.numbers[0]=-31;
        bool saved=diagnostic::SaveVrSettings(path,value);auto loaded=diagnostic::LoadVrSettings(path);
        check(saved&&loaded.numbers[85]==fog&&loaded.numbers[0]==-31,"Explicit fog preferences survive save/load without resetting camera calibration");}

    check(missingHandHud.fastPickup&&missingHandHud.firstPersonDialogue&&missingHandHud.firstPersonChest&&missingHandHud.firstPersonLectern,
        "Missing comfort keys preserve the enabled 1.0.6 through 1.0.9 behavior");
    for(unsigned mask=0;mask<32;++mask){diagnostic::VrSettings value;
        value.fastPickup=(mask&1)!=0;value.firstPersonDialogue=(mask&2)!=0;
        value.firstPersonChest=(mask&4)!=0;value.firstPersonLectern=(mask&8)!=0;value.thirdPersonOrbit=(mask&16)!=0;
        value.numbers[0]=-31;value.numbers[8]=7;diagnostic::ApplyLauncherFixedFeatures(value);
        bool saved=diagnostic::SaveVrSettings(path,value);auto loaded=diagnostic::LoadVrSettings(path);
        check(saved&&loaded.fastPickup==bool(mask&1)&&loaded.firstPersonDialogue==bool(mask&2)&&loaded.firstPersonChest==bool(mask&4)&&
            loaded.firstPersonLectern==bool(mask&8)&&loaded.thirdPersonOrbit==bool(mask&16)&&loaded.numbers[0]==-31&&loaded.numbers[8]==7,
            "All pickup and third-person choices survive launcher defaults enforcement and independent settings round-trip");
    }

    int expectedHandHud[]{-7,0,0,-90,180,-90,7,0,0,90,0,-90,50,50};
    bool correctDefaults=true;for(unsigned i=0;i<14;++i)correctDefaults&=missingHandHud.numbers[87+i]==expectedHandHud[i];
    check(correctDefaults,"Missing hand HUD keys use the calibrated transforms and 50 percent independent scales");
    check(diagnostic::BoundVrNumber(diagnostic::VrNumbers[99],1)==25&&diagnostic::BoundVrNumber(diagnostic::VrNumbers[100],999)==200,"Both scales clamp to the supported 25 to 200 percent range");
    check(diagnostic::LoadVrSettings(path).handHudEnabled,"Hand HUD defaults to enabled for existing configurations without the new key");
    check(diagnostic::LoadVrSettings(path).hideCompass&&!diagnostic::LoadVrSettings(path).showSlotBar&&!diagnostic::LoadVrSettings(path).showVitalsPanel,"All optional native HUD displays default to disabled when keys are missing");
    for(unsigned mask=0;mask<8;++mask){diagnostic::VrSettings value;value.hideCompass=!(mask&1);value.showSlotBar=(mask&2)!=0;value.showVitalsPanel=(mask&4)!=0;value.numbers[2]=120;
        diagnostic::ApplyLauncherFixedFeatures(value);bool saved=diagnostic::SaveVrSettings(path,value);auto loaded=diagnostic::LoadVrSettings(path);
        check(saved&&loaded.hideCompass==value.hideCompass&&loaded.showSlotBar==value.showSlotBar&&loaded.showVitalsPanel==value.showVitalsPanel&&loaded.numbers[2]==120,"Each HUD visibility combination persists independently of other HUD settings");
        diagnostic::HudVisibility show{!loaded.hideCompass,loaded.showSlotBar,loaded.showVitalsPanel};
        check(diagnostic::HiddenHudControl(20545,show)==loaded.hideCompass&&diagnostic::HiddenHudControl(20380,show)==!loaded.showSlotBar&&
              diagnostic::HiddenHudControl(20384,show)==!loaded.showVitalsPanel,"Native compass, quick slots and player vitals respect independent toggles");
    }
    for(bool enabled:{true,false}){diagnostic::VrSettings value;value.handHudEnabled=enabled;
        value.showSlotBar=false;value.showVitalsPanel=false;value.numbers[8]=7;
        for(unsigned i=87;i<101;++i)value.numbers[i]=i%2?diagnostic::VrNumbers[i].minimum:diagnostic::VrNumbers[i].maximum;
        bool saved=diagnostic::SaveVrSettings(path,value);auto loaded=diagnostic::LoadVrSettings(path);
        bool same=true;for(unsigned i=87;i<101;++i)same&=loaded.numbers[i]==value.numbers[i];
        check(saved&&same&&loaded.handHudEnabled==enabled&&!loaded.showSlotBar&&!loaded.showVitalsPanel&&loaded.numbers[8]==7,
            "Both hand HUD transforms, independent scales and visibility round-trip independently of native HUD and palm calibration");}
    struct Widget {unsigned id;Widget* parent;};Widget root{0,nullptr},bar{20379,&root},decoration{99,&bar},health{20387,&root},healthLabel{42,&health},focus{20386,&root},inventory{20394,&root};
    auto hidden=[](Widget* window,diagnostic::HudVisibility show={}){return diagnostic::HiddenHudWindow(window,show,[](Widget* w){return w->id;},[](Widget* w){return w->parent;});};
    check(hidden(&decoration)&&hidden(&healthLabel),"Hidden HUD panels suppress decorative and label descendants as well as bars");
    check(!hidden(&focus)&&!hidden(&inventory)&&!hidden(&root),"Target focus bar, inventory and unrelated HUD controls remain available");
    check(!hidden(&decoration,{false,true,false})&&!hidden(&healthLabel,{false,false,true}),"Enabling either HUD panel restores its own children independently");
    check(diagnostic::HudPaintMessage(15)&&diagnostic::HudPaintMessage(20)&&diagnostic::HudPaintMessage(0x85)&&!diagnostic::HudPaintMessage(0x100)&&!diagnostic::HudPaintMessage(2),"Visibility filters only paint messages, preserving key events and destruction");
    for(auto rva:{uintptr_t(0x2d1b44),uintptr_t(0x2d180c),uintptr_t(0x2d1924),uintptr_t(0x2d15d4)}){
        auto id=diagnostic::HudViewControlId(UINT32_MAX,rva);
        check(id!=UINT32_MAX&&diagnostic::HiddenHudControl(id,{}),"Anonymous native view root is classified so its frame and background hide with the bar");
        auto shown=diagnostic::EffectiveHudVisibility({},true);
        check(!diagnostic::HiddenHudControl(id,shown),"Inventory shows complete native slot and vital view roots despite disabled gameplay options");
    }
    auto outsideInventory=diagnostic::EffectiveHudVisibility({},false),insideInventory=diagnostic::EffectiveHudVisibility({},true);
    check(!outsideInventory.slotBar&&!outsideInventory.vitals&&insideInventory.slotBar&&insideInventory.vitals&&!insideInventory.compass,"Inventory override shows both blocks without enabling the compass or changing gameplay defaults");
    check(diagnostic::HudViewControlId(UINT32_MAX,0x12345)==UINT32_MAX,"Unrelated anonymous views are never suppressed");
    diagnostic::HudInventoryFrameCache inventoryCache;unsigned inventoryQueries=0;
    auto inventoryQuery=[&](){++inventoryQueries;return true;};inventoryCache.Get(1,inventoryQuery);inventoryCache.Get(1,inventoryQuery);
    check(inventoryQueries==1&&!inventoryCache.Get(2,[&](){++inventoryQueries;return false;})&&inventoryQueries==2,"Inventory state is queried once per frame and refreshed on the next frame");
    Widget cycle{7,nullptr};cycle.parent=&cycle;check(!hidden(&cycle),"Malformed parent cycle is bounded");
    for(bool hidden:{true,false}){diagnostic::VrSettings value;value.hideCompass=hidden;value.numbers[2]=120;value.bowReticle=false;
        diagnostic::ApplyLauncherFixedFeatures(value);
        bool saved=diagnostic::SaveVrSettings(path,value);auto loaded=diagnostic::LoadVrSettings(path);
        check(saved&&loaded.hideCompass==hidden&&loaded.numbers[2]==120&&!loaded.bowReticle,"Compass preference round-trips independently of HUD scale and aiming reticle");}
    check(!diagnostic::LoadVrSettings(path).handsOnly,"Hands only remains opt-in for existing and fresh configurations");
    check(diagnostic::LoadVrSettings(path).numbers[82]==40,"Missing 2H grip loss distance defaults to 40 cm");
    check(diagnostic::LoadVrSettings(path).numbers[83]==40&&diagnostic::LoadVrSettings(path).numbers[84]==20,"Release defaults: staff grip loss 40 cm and crossbow grip loss 20 cm");
    {diagnostic::VrSettings value;value.numbers[82]=25;value.numbers[83]=55;value.numbers[84]=80;value.numbers[8]=7;
        bool saved=diagnostic::SaveVrSettings(path,value);auto loaded=diagnostic::LoadVrSettings(path);
        check(saved&&loaded.numbers[82]==25&&loaded.numbers[83]==55&&loaded.numbers[84]==80&&loaded.numbers[8]==7,"Each weapon family retains its own independent grip loss distance and hand calibration");}
    for(int distance:{10,40,100}){diagnostic::VrSettings value;value.numbers[82]=distance;value.numbers[8]=7;
        bool saved=diagnostic::SaveVrSettings(path,value);auto loaded=diagnostic::LoadVrSettings(path);
        check(saved&&loaded.numbers[82]==distance&&loaded.numbers[8]==7,"2H grip loss distance round-trips independently of hand calibration");}
    check(diagnostic::BoundVrNumber(diagnostic::VrNumbers[82],-50)==10&&diagnostic::BoundVrNumber(diagnostic::VrNumbers[82],500)==100&&diagnostic::BoundVrNumber(diagnostic::VrNumbers[82],42)==40,"2H grip loss distance is bounded and snaps to 5 cm steps");
    check(diagnostic::LoadVrSettings(path).toggleSupportGrip,"Release creator preset defaults to Toggle support grip");
    for(bool enabled:{true,false}){diagnostic::VrSettings value;value.toggleSupportGrip=enabled;value.numbers[8]=7;value.numbers[14]=-3;
        diagnostic::ApplyLauncherFixedFeatures(value);
        bool saved=diagnostic::SaveVrSettings(path,value);auto loaded=diagnostic::LoadVrSettings(path);
        check(saved&&loaded.toggleSupportGrip==enabled&&loaded.numbers[8]==7&&loaded.numbers[14]==-3,"Toggle support preference round-trips without changing either hand calibration");}
    for(bool enabled:{false,true}){diagnostic::VrSettings value;value.handsOnly=enabled;value.numbers[0]=-31;value.numbers[8]=7;
        diagnostic::ApplyLauncherFixedFeatures(value);
        check(value.handsOnly==enabled,"Fixed camera/tracking features do not overwrite the Hands only preference");
        bool saved=diagnostic::SaveVrSettings(path,value);auto loaded=diagnostic::LoadVrSettings(path);
        check(saved&&loaded.handsOnly==enabled&&loaded.numbers[0]==-31&&loaded.numbers[8]==7&&loaded.physicalCrouching&&loaded.thirdPersonOrbit,
            "Hands only round-trip preserves camera, hand calibration, physical crouching and third-person orbit");}
    {diagnostic::VrSettings old;
        old.autoStart=old.keepFirstPerson=old.hideHead=old.roomScaleWalking=false;
        old.stableHorizon=old.smoothCamera=old.trackedArms=old.stableBody=false;
        old.thirdPersonOrbit=old.physicalCrouching=false;old.controllers=old.instantControllerTurn=false;old.numbers[0]=-32;old.numbers[7]=800;
        auto before=old;diagnostic::ApplyLauncherFixedFeatures(old);
        bool fixed=old.autoStart&&old.keepFirstPerson&&old.hideHead&&old.roomScaleWalking&&old.stableHorizon&&old.smoothCamera&&old.trackedArms&&old.stableBody&&old.physicalCrouching&&!old.thirdPersonOrbit;
        check(fixed&&diagnostic::SaveVrSettings(path,old),"Removed launcher switches retain the approved enabled behavior for old configurations");
        auto loaded=diagnostic::LoadVrSettings(path);
        check(loaded.autoStart&&loaded.physicalCrouching&&loaded.trackedArms&&loaded.controllers&&loaded.instantControllerTurn&&memcmp(loaded.numbers,before.numbers,sizeof(old.numbers))==0&&loaded.openXrRuntime==before.openXrRuntime&&loaded.upscaleMethod==before.upscaleMethod&&loaded.ffrMode==before.ffrMode,
            "Fixed features enable controller input and preserve calibration, runtime and graphics through Save settings");}
    for(bool orbit:{false,true})for(bool crouch:{false,true}){diagnostic::VrSettings value;value.thirdPersonOrbit=orbit;value.physicalCrouching=crouch;auto previous=value.numbers[0];
        bool saved=diagnostic::SaveVrSettings(path,value);auto loaded=diagnostic::LoadVrSettings(path);
        check(saved&&loaded.thirdPersonOrbit==orbit&&loaded.physicalCrouching==crouch&&loaded.numbers[0]==previous,"Orbit and visual crouch round-trip independently without changing camera calibration");}
    // Compare the same serialized public settings, including every scalar, from
    // the frozen installation INI, the Defaults button, and missing-file fallbacks.
    check(argc==2,"Frozen creator preset supplied to settings test");
    if(argc==2){
        std::wstring files[3]={std::wstring(path)+L".creator",std::wstring(path)+L".defaults",std::wstring(path)+L".missing"};
        auto creator=diagnostic::LoadVrSettings(argv[1]);
        auto missing=diagnostic::LoadVrSettings(files[2].c_str());
        diagnostic::VrSettings reset;
        bool parity=diagnostic::SaveVrSettings(files[0].c_str(),creator)&&diagnostic::SaveVrSettings(files[1].c_str(),reset)&&diagnostic::SaveVrSettings(files[2].c_str(),missing);
        for(auto section:{L"Stereo",L"Controllers",L"Locomotion",L"ControllerBindings"}){
            wchar_t values[3][16384]{};
            for(int i=0;i<3;++i)GetPrivateProfileSectionW(section,values[i],16384,files[i].c_str());
            parity&=memcmp(values[0],values[1],sizeof(values[0]))==0&&memcmp(values[0],values[2],sizeof(values[0]))==0;
        }
        check(parity,"Frozen installation preset, Defaults and missing-file fallbacks agree on all launcher settings");
        for(auto const& f:files){DeleteFileW(f.c_str());DeleteFileW((f+L".bak").c_str());}
    }
    constexpr char fixture[]="; User configuration must survive\r\n[Diagnostics]\r\nEnabled=1\r\nMaxLogMiB=8\r\n[Stereo]\r\nFirstPersonHeightOffsetCm=-25\r\nUnitsPerMeter=125\r\nHudVerticalOffsetCm=-999\r\nMenuScalePercent=bad\r\nAutoStart=0\r\n[CustomMod]\r\nUnknownSetting=keep-me\r\n";
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);DWORD bytes=0;WriteFile(file,fixture,sizeof(fixture)-1,&bytes,nullptr);CloseHandle(file);
    auto s=diagnostic::LoadVrSettings(path);
    check(s.particleQuality==2,"Release creator preset defaults to Performance VR fire/magic visuals");
    for(int quality:{0,1,2}){s.particleQuality=quality;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).particleQuality==quality,"Native/Balanced/Performance particle quality roundtrip preserves other settings");}
    WritePrivateProfileStringW(L"Stereo",L"ParticleEffectQuality",L"999",path);check(diagnostic::LoadVrSettings(path).particleQuality==2,"Particle quality is bounded");
    s.particleQuality=1;
    check(s.openXrRuntime==diagnostic::OpenXrRuntimeChoice::Vdxr,"Missing runtime uses the frozen creator preset");
    for(int choice=0;choice<4;++choice){s.openXrRuntime=diagnostic::OpenXrRuntimeChoice(choice);check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).openXrRuntime==s.openXrRuntime,"OpenXR runtime roundtrip preserves calibration");}
    s.openXrRuntime=diagnostic::OpenXrRuntimeChoice::Vdxr;
    check(s.fxaaEnabled,"Missing FXAA key uses the creator preset");
    for(bool enabled:{true,false}){s.fxaaEnabled=enabled;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).fxaaEnabled==enabled,"FXAA roundtrip preserves hand and camera settings");}
    check(!s.smaaEnabled,"Old configuration has no SMAA cost");
    s.smaaEnabled=s.fxaaEnabled=true;check(diagnostic::SaveVrSettings(path,s),"Save conflicting legacy AA flags");
    auto aa=diagnostic::LoadVrSettings(path);check(aa.smaaEnabled&&!aa.fxaaEnabled,"SMAA selection excludes FXAA");
    WritePrivateProfileStringW(L"Stereo",L"FxaaEnabled",L"1",path);aa=diagnostic::LoadVrSettings(path);check(aa.smaaEnabled&&!aa.fxaaEnabled,"Manual conflicting INI cannot stack spatial AA");
    s.smaaEnabled=s.fxaaEnabled=false;check(diagnostic::SaveVrSettings(path,s),"Both spatial filters can be disabled");
    check(s.smaaQuality==2,"Missing SMAA quality uses the creator preset");
    for(int quality=0;quality<3;++quality){s.smaaQuality=quality;s.smaaEnabled=true;s.fxaaEnabled=false;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).smaaQuality==quality,"Medium/High/Ultra settings roundtrip");}
    WritePrivateProfileStringW(L"Stereo",L"SmaaQuality",L"999",path);check(diagnostic::LoadVrSettings(path).smaaQuality==0,"Invalid SMAA quality falls back to Medium");
    s.smaaQuality=0;s.smaaEnabled=false;check(diagnostic::SaveVrSettings(path,s),"SMAA preset test restores disabled filter");
    check(!s.dlaaExperimental,"DLAA remains opt-in for existing configurations");
    for(bool enabled:{true,false}){s.dlaaExperimental=enabled;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).dlaaExperimental==enabled,"DLAA setting round-trips without changing calibration");}
    check(s.numbers[81]==20,"existing configurations gain small string-hand bow aim influence");
    s.numbers[81]=0;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).numbers[81]==0,"string-hand aim can be disabled independently");
    s.numbers[81]=999;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).numbers[81]==40,"string-hand aim influence remains bounded");s.numbers[81]=20;
    diagnostic::VrSettings defaults;bool consistent=true;for(unsigned i=0;i<diagnostic::VrNumberCount;++i)consistent&=defaults.numbers[i]==diagnostic::VrNumbers[i].initial;check(consistent,"Reset defaults match all numeric field defaults");
    check(s.numbers[73]==40&&s.numbers[74]==20&&s.numbers[75]==30&&s.numbers[76]==40&&s.numbers[77]==65&&s.numbers[78]==45&&s.numbers[79]==20&&s.numbers[80]==15,"existing INIs gain independent light weapon handling defaults");
    for(unsigned i=73;i<81;++i)s.numbers[i]=int(i-73)*10;
    check(diagnostic::SaveVrSettings(path,s),"all four handling profiles save atomically");auto handling=diagnostic::LoadVrSettings(path);
    bool handlingRoundtrip=true;for(unsigned i=73;i<81;++i)handlingRoundtrip&=handling.numbers[i]==s.numbers[i];
    check(handlingRoundtrip,"1H/supported2H/staff one/two-hand profiles remain independent");
    s.numbers[73]=999;s.numbers[74]=-5;s.numbers[79]=23;s.numbers[80]=999;
    check(diagnostic::SaveVrSettings(path,s),"handling limits save");handling=diagnostic::LoadVrSettings(path);
    check(handling.numbers[73]==200&&handling.numbers[74]==0&&handling.numbers[79]==25&&handling.numbers[80]==150,"weight 0..200 and stabilization 0..150 bound and snap to five-unit steps");
    for(unsigned i=73;i<81;++i)s.numbers[i]=handling.numbers[i];

    check(s.numbers[70]==100&&s.numbers[71]==50&&s.numbers[72]==50,"existing configurations get lower independent stab speed defaults");
    check(s.numbers[67]==25&&s.numbers[68]==15&&s.numbers[69]==15,"missing stab keys use separate short thrust defaults");
    check(s.numbers[64]==600&&s.numbers[65]==600&&s.numbers[66]==600,"existing configurations gain speed gates without changing saved distance or calibration");
    check(s.numbers[29]==9&&s.numbers[30]==2&&s.numbers[31]==0,"bow grip has separate defaults without changing existing hand calibration");
    check(s.numbers[58]==0&&s.numbers[61]==0&&s.numbers[62]==-10&&s.numbers[63]==0,"shield defaults retain native orientation and separate palm calibration");
    s.numbers[58]=25;s.numbers[61]=3;s.numbers[62]=-2;s.numbers[63]=1;
    check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).numbers[58]==25&&diagnostic::LoadVrSettings(path).numbers[61]==3,"shield settings round-trip independently of hand and bow calibration");
    check(s.bowReticle,"existing configurations keep bow reticle enabled by default");
    check(s.numbers[0]==-25&&s.numbers[1]==-150&&s.numbers[4]==100&&!s.autoStart,"preserve existing height, clamp bounds, reject malformed value, honor autostart off");
    check(s.instantControllerTurn,"instant turn remains independent of native locomotion");
    check(s.numbers[51]==65,"radial defaults smaller independently of HUD and menu scale");
    s.numbers[51]=55;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).numbers[51]==55,"radial size saves live without overwriting existing menu and camera values");
    s.numbers[51]=999;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).numbers[51]==120,"radial upper scale is bounded");s.numbers[51]=55;
    s.instantControllerTurn=false;
    s.numbers[0]=-32;s.numbers[1]=-45;s.numbers[2]=85;s.numbers[3]=12;s.numbers[4]=130;s.numbers[5]=250;s.autoStart=true;s.hideHead=false;s.renderWidth=3840;s.renderHeight=2160;
    check(diagnostic::SaveVrSettings(path,s),"atomic save succeeds");
    auto saved=diagnostic::LoadVrSettings(path);bool equal=saved.autoStart&&saved.hideHead==false;for(unsigned i=0;i<diagnostic::VrNumberCount;++i)equal&=saved.numbers[i]==s.numbers[i];
    check(!saved.instantControllerTurn,"native locomotion preserves the independent turn preference");
    check(equal,"signed camera and both panel layouts round-trip through real Windows INI APIs");
    check(saved.renderWidth==3840&&saved.renderHeight==2160,"requested native per-eye render resolution survives launcher restart");
    wchar_t unknown[64];GetPrivateProfileStringW(L"CustomMod",L"UnknownSetting",L"",unknown,64,path);
    check(!wcscmp(unknown,L"keep-me")&&GetPrivateProfileIntW(L"Stereo",L"UnitsPerMeter",0,path)==125&&GetPrivateProfileIntW(L"Diagnostics",L"MaxLogMiB",0,path)==8,"preserve unrelated sections, world scale and diagnostic settings");
    std::wstring backup=std::wstring(path)+L".bak";auto old=diagnostic::LoadVrSettings(backup.c_str());check(old.numbers[0]==-25&&!old.autoStart,"backup contains the complete previous configuration");
    auto hud=diagnostic::PanelLayout(saved,false),menu=diagnostic::PanelLayout(saved,true);
    check(hud.verticalMeters<0&&menu.verticalMeters>0&&hud.widthMeters<menu.widthMeters&&hud.distanceMeters==2.5f&&menu.distanceMeters==2.5f,"HUD and menu layouts are independent with shared finite distance");
    HANDLE locked=CreateFileW(path,GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr);
    check(!diagnostic::SaveVrSettings(path,s),"locked original refuses save rather than losing unrelated settings");CloseHandle(locked);
    check(diagnostic::LoadVrSettings(path).numbers[0]==-32,"failed save leaves the original intact");
    check(s.stableBody,"stable body is enabled for an existing configuration without the new key");
    check(s.roomScaleWalking,"existing installations enable physical body following without changing saved camera offsets");
    check(s.smoothCamera&&s.cameraSmoothingMs==40&&s.cameraVerticalSmoothingMs==100,"existing configurations enable native camera stabilization without changing head tracking");
    s.smoothCamera=false;s.cameraSmoothingMs=55;s.cameraVerticalSmoothingMs=130;
    check(diagnostic::SaveVrSettings(path,s)&&!diagnostic::LoadVrSettings(path).smoothCamera&&diagnostic::LoadVrSettings(path).cameraSmoothingMs==55&&diagnostic::LoadVrSettings(path).cameraVerticalSmoothingMs==130,"camera smoothing toggle and independent native time constants round-trip");
    s.cameraSmoothingMs=999;s.cameraVerticalSmoothingMs=-10;
    check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).cameraSmoothingMs==250&&diagnostic::LoadVrSettings(path).cameraVerticalSmoothingMs==0,"camera smoothing is bounded and zero allows an immediate native axis");
    check(s.pickupReachCm==180,"ground pickup defaults to 180 cm separately from NPC reach");
    check(s.interactionAimUpPercent==-25,"existing configurations raise world selection by the requested 15 percent");
    s.interactionAimUpPercent=20;
    check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).interactionAimUpPercent==20,"launcher saves preserve the separate interaction elevation calibration");
    s.interactionAimUpPercent=0;
    check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).interactionAimUpPercent==0,"zero calibration restores unadjusted controller selection");
    s.interactionAimUpPercent=-150;
    check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).interactionAimUpPercent==-150,"negative selection calibration survives real signed INI round-trip across the expanded launcher range");
    s.interactionAimUpPercent=999;
    check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).interactionAimUpPercent==200,"launcher calibration is bounded at the extended maximum");
    s.pickupReachCm=165;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).pickupReachCm==165,"ground pickup reach survives launcher saves");
    check(s.interactionReachCm==120,"existing installations use 120 cm hand-to-surface reach");
    s.interactionReachCm=90;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).interactionReachCm==90,"controller reach survives launcher saves alongside existing numeric settings");
    s.interactionReachCm=999;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).interactionReachCm==250,"interaction reach is bounded independently of bow aiming");
    s.numbers[23]=-25;s.numbers[24]=12;s.numbers[25]=-8;s.numbers[26]=65;s.numbers[27]=15;s.numbers[28]=120;
    check(diagnostic::SaveVrSettings(path,s),"bow calibration saves atomically");
    auto bow=diagnostic::LoadVrSettings(path);check(bow.numbers[23]==-25&&bow.numbers[24]==12&&bow.numbers[25]==-8&&bow.numbers[26]==65&&bow.numbers[27]==15&&bow.numbers[28]==120,"bow angles, full draw, string radius and aim stabilization round-trip alongside existing weapon settings");
    check(s.numbers[48]==4&&s.numbers[49]==2&&s.numbers[50]==0,"existing configurations get independent magic palm contacts");
    s.numbers[48]=6;s.numbers[49]=3;s.numbers[50]=-2;check(diagnostic::SaveVrSettings(path,s),"magic contact saves");auto magic=diagnostic::LoadVrSettings(path);
    check(magic.numbers[48]==6&&magic.numbers[49]==3&&magic.numbers[50]==-2&&magic.numbers[8]==s.numbers[8]&&magic.numbers[28]==120,"magic offsets preserve controller calibration and aim stabilization");
    s.numbers[29]=-4;s.numbers[30]=7;s.numbers[31]=-2;check(diagnostic::SaveVrSettings(path,s),"bow contact offsets save atomically");
    auto contact=diagnostic::LoadVrSettings(path);check(contact.numbers[29]==-4&&contact.numbers[30]==7&&contact.numbers[31]==-2&&contact.numbers[8]==s.numbers[8]&&contact.numbers[28]==120,"bow contact offsets round-trip independently of tracking and aim");
    for(bool enabled:{false,true}){s.bowReticle=enabled;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).bowReticle==enabled&&diagnostic::LoadVrSettings(path).numbers[28]==120,"bow reticle on/off round-trips without altering aim stabilization or hand calibration");}
    check(s.twoHandedMelee&&s.numbers[32]==-30&&s.numbers[35]==10,"existing configurations get separate two-hand defaults");
    check(s.heavyTwoHand,"heavy one-hand 2H handling defaults on for existing configurations");
    for(bool enabled:{false,true}){s.heavyTwoHand=enabled;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).heavyTwoHand==enabled,"heavy 2H option round-trips independently of support grip");}
    s.numbers[32]=-20;s.numbers[33]=10;s.numbers[34]=15;s.numbers[35]=24;
    for(bool enabled:{false,true}){s.twoHandedMelee=enabled;check(diagnostic::SaveVrSettings(path,s),"two-hand settings save atomically");auto two=diagnostic::LoadVrSettings(path);
        check(two.twoHandedMelee==enabled&&two.numbers[32]==-20&&two.numbers[33]==10&&two.numbers[34]==15&&two.numbers[35]==24&&two.numbers[29]==-4&&two.numbers[28]==120,"two-hand settings preserve bow contact and aiming");}
    check(s.numbers[36]==0&&s.numbers[39]==15&&s.numbers[40]==4&&s.numbers[41]==2,"staff profile defaults include independent palm contact");
    s.numbers[36]=15;s.numbers[37]=-10;s.numbers[38]=20;s.numbers[39]=18;s.numbers[40]=6;s.numbers[41]=3;
    check(diagnostic::SaveVrSettings(path,s),"staff contact saves atomically");auto staff=diagnostic::LoadVrSettings(path);
    check(staff.numbers[36]==15&&staff.numbers[37]==-10&&staff.numbers[38]==20&&staff.numbers[39]==18&&staff.numbers[40]==6&&staff.numbers[41]==3&&staff.numbers[32]==-20&&staff.numbers[35]==24&&staff.numbers[29]==-4,"staff contact preserves 2H and bow calibration");
    WritePrivateProfileStringW(L"Stereo",L"StaffReverseGrip",L"1",path);auto legacy=diagnostic::LoadVrSettings(path);
    check(legacy.numbers[40]==6&&legacy.numbers[36]==15,"obsolete reversal key does not override accepted staff profile");
    s.numbers[40]=100;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).numbers[40]==10,"staff contact is bounded");s.numbers[40]=6;
    s.controllers=false;s.stableHorizon=false;s.trackedArms=false;s.stableBody=false;s.roomScaleWalking=false;s.numbers[6]=24;s.numbers[7]=800;
    check(diagnostic::SaveVrSettings(path,s)&&!diagnostic::LoadVrSettings(path).controllers,"launcher can disable controller input and preserve the setting");
    auto comfort=diagnostic::LoadVrSettings(path);check(!comfort.stableHorizon&&comfort.numbers[6]==24&&comfort.numbers[7]==800,"forward camera offset, increased turn speed and stable horizon round-trip");
    check(!comfort.trackedArms,"experimental tracked-arm setting round-trips without changing other controller options");
    check(!comfort.stableBody,"stable body can be disabled and round-trips independently of the horizon");
    check(!comfort.roomScaleWalking,"physical body following can be disabled independently of optical 6DoF tracking");
    s.numbers[7]=9999;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).numbers[7]==2000,"out-of-range turn speed is bounded at the new maximum");
    check(s.numbers[8]==0&&s.numbers[14]==0,"existing installations move both hands 5 cm outward by default");
    int handValues[]={-7,3,8,25,-40,12,9,-2,-5,-18,32,-15};
    for(unsigned i=0;i<12;++i)s.numbers[8+i]=handValues[i];
    bool handSaved=diagnostic::SaveVrSettings(path,s);auto calibrated=diagnostic::LoadVrSettings(path);
    for(unsigned i=0;i<12;++i)handSaved&=calibrated.numbers[8+i]==handValues[i];
    check(handSaved,"independent signed hand position and orientation calibration round-trip without losing unrelated keys");
    s.numbers[8]=999;s.numbers[17]=-999;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).numbers[8]==20&&diagnostic::LoadVrSettings(path).numbers[17]==-90,"hand positions and angles are separately bounded");
    s.numbers[20]=-25;s.numbers[21]=15;s.numbers[22]=35;
    check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).numbers[20]==-25&&diagnostic::LoadVrSettings(path).numbers[21]==15&&diagnostic::LoadVrSettings(path).numbers[22]==35,"sword-only grip angles round-trip independently of device calibration");
    check(s.numbers[42]==-35&&s.numbers[45]==15&&s.numbers[46]==4,"crossbow gets separate palm defaults");
    s.numbers[42]=-15;s.numbers[43]=20;s.numbers[44]=-10;s.numbers[45]=7;s.numbers[46]=3;s.numbers[47]=-2;s.bowReticle=false;
    check(diagnostic::SaveVrSettings(path,s),"crossbow settings save atomically");auto crossbow=diagnostic::LoadVrSettings(path);
    bool crossbowEqual=true;for(unsigned i=42;i<diagnostic::VrNumberCount;++i)crossbowEqual&=crossbow.numbers[i]==s.numbers[i];
    check(crossbowEqual&&!crossbow.bowReticle&&crossbow.numbers[28]==120&&crossbow.numbers[29]==-4&&crossbow.numbers[40]==6,"crossbow calibration and shared reticle preserve bow, staff and tracking settings");
    WritePrivateProfileStringW(L"Stereo",L"CrossbowReloadMs",L"1",path);
    auto oldReload=diagnostic::LoadVrSettings(path);check(oldReload.numbers[47]==-2&&oldReload.numbers[48]==s.numbers[48]&&oldReload.numbers[49]==s.numbers[49]&&oldReload.numbers[50]==s.numbers[50],"obsolete reload interval cannot override native progression");
    for(unsigned i=52;i<58;++i){s.numbers[i]=int(i-50)*10;check(diagnostic::VrNumbers[i].step==10,"melee damage and stamina sliders use ten-centimetre steps");}
    check(diagnostic::SaveVrSettings(path,s),"all weapon-class thresholds save atomically");auto minimums=diagnostic::LoadVrSettings(path);
    for(unsigned i=52;i<58;++i)check(minimums.numbers[i]==s.numbers[i],"independent per-class damage/stamina threshold round-trip");
    s.numbers[52]=27;s.numbers[53]=999;check(diagnostic::SaveVrSettings(path,s)&&diagnostic::LoadVrSettings(path).numbers[52]==30&&diagnostic::LoadVrSettings(path).numbers[53]==200,"thresholds snap to ten-centimetre steps and bound extremes");
    s.numbers[64]=150;s.numbers[65]=230;s.numbers[66]=170;
    check(diagnostic::SaveVrSettings(path,s),"independent melee velocity thresholds save atomically");auto speeds=diagnostic::LoadVrSettings(path);
    check(speeds.numbers[64]==150&&speeds.numbers[65]==230&&speeds.numbers[66]==170&&speeds.numbers[52]==30&&speeds.numbers[53]==200,"per-class velocity thresholds preserve independent damage/stamina distance keys");
    s.numbers[64]=999;s.numbers[65]=41;s.numbers[66]=156;
    check(diagnostic::SaveVrSettings(path,s),"velocity calibration accepts bounded values");auto boundedSpeeds=diagnostic::LoadVrSettings(path);
    check(boundedSpeeds.numbers[64]==800&&boundedSpeeds.numbers[65]==50&&boundedSpeeds.numbers[66]==160,"velocity gates clamp to 50..800 cm/s and snap in ten-unit steps");
    for(auto key:{L"StableLocomotion",L"DirectVelocity"})WritePrivateProfileStringW(L"Locomotion",key,L"1",path);
    WritePrivateProfileStringW(L"Stereo",L"LocomotionResponseMs",L"150",path);
    auto migration=diagnostic::LoadVrSettings(path);check(diagnostic::SaveVrSettings(path,migration),"save migrates obsolete direct locomotion settings");
    wchar_t removed[64];bool absent=true;for(auto key:{L"StableLocomotion",L"DirectVelocity"}){GetPrivateProfileStringW(L"Locomotion",key,L"missing",removed,64,path);absent&=!wcscmp(removed,L"missing");}
    GetPrivateProfileStringW(L"Stereo",L"LocomotionResponseMs",L"missing",removed,64,path);absent&=!wcscmp(removed,L"missing");
    check(absent,"old enabled direct motor and acceleration keys are removed without affecting other preferences");
    auto migrated=diagnostic::LoadVrSettings(path);check(migrated.numbers[51]==s.numbers[51]&&migrated.numbers[52]==30&&migrated.numbers[53]==200,"radial and independent swing thresholds retain key identities after obsolete slider removal");
    s.numbers[67]=10;s.numbers[68]=25;s.numbers[69]=35;
    check(diagnostic::SaveVrSettings(path,s),"separate stab profiles save live");auto stabs=diagnostic::LoadVrSettings(path);
    check(stabs.numbers[67]==10&&stabs.numbers[68]==25&&stabs.numbers[69]==35&&stabs.numbers[52]==30&&stabs.numbers[53]==200,"stab calibration preserves swing distance keys");
    s.numbers[67]=1;s.numbers[68]=999;s.numbers[69]=13;
    check(diagnostic::SaveVrSettings(path,s),"bounded stab calibration saves");stabs=diagnostic::LoadVrSettings(path);
    check(stabs.numbers[67]==5&&stabs.numbers[68]==100&&stabs.numbers[69]==15,"stab profiles bound to5..100cm and snap to5cm steps");
    s.numbers[70]=40;s.numbers[71]=80;s.numbers[72]=130;
    check(diagnostic::SaveVrSettings(path,s),"independent stab velocity saves atomically");auto stabSpeeds=diagnostic::LoadVrSettings(path);
    check(stabSpeeds.numbers[70]==40&&stabSpeeds.numbers[71]==80&&stabSpeeds.numbers[72]==130&&stabSpeeds.numbers[64]==800&&stabSpeeds.numbers[65]==50&&stabSpeeds.numbers[66]==160,"stab speed profiles preserve higher swing velocity and all distance keys");
    s.numbers[70]=1;s.numbers[71]=999;s.numbers[72]=57;
    check(diagnostic::SaveVrSettings(path,s),"bounded stab velocity saves");stabSpeeds=diagnostic::LoadVrSettings(path);
    check(stabSpeeds.numbers[70]==40&&stabSpeeds.numbers[71]==800&&stabSpeeds.numbers[72]==60,"stab speed clamps40..800cm/s and snaps in ten-unit steps");
    DeleteFileW(path);DeleteFileW(backup.c_str());return failures?1:0;
}
