#pragma once
#include "UpscalePlan.h"
#include "OpenXrRuntimeChoice.h"
#include "FoveationPattern.h"
#include "ParticlePerformance.h"
#include "ControllerMappingSettings.h"
#include <windows.h>
#include <cwchar>
#include <algorithm>
#include <cmath>

namespace diagnostic {
// Shared, bounded settings for the launcher and the game. Add future controls here.
struct VrSetting { wchar_t const* key; wchar_t const* label; int initial, minimum, maximum; wchar_t const* unit; int step=1; };
inline constexpr VrSetting VrNumbers[] = {
    {L"FirstPersonHeightOffsetCm", L"Camera height", -15, -50, 50, L"cm"},
    {L"HudVerticalOffsetCm", L"HUD vertical position", -15, -150, 100, L"cm"},
    {L"HudScalePercent", L"HUD size", 89, 40, 180, L"%"},
    {L"MenuVerticalOffsetCm", L"Menu vertical position", 0, -150, 100, L"cm"},
    {L"MenuScalePercent", L"Menu size", 100, 40, 180, L"%"},
    {L"UiDistanceCm", L"Panel distance", 210, 100, 500, L"cm"},
    {L"FirstPersonForwardOffsetCm", L"Camera forward / back", 20, -100, 100, L"cm"},
    {L"ControllerTurnSpeedPercent", L"Thumbstick turn speed", 60, 10, 2000, L"%"},
    {L"LeftHandOutwardCm", L"Hand outward / inward", 0, -20, 20, L"cm"},
    {L"LeftHandUpCm", L"Hand up / down", 0, -20, 20, L"cm"},
    {L"LeftHandForwardCm", L"Hand forward / back", 0, -20, 20, L"cm"},
    {L"LeftHandPitchDeg", L"Hand pitch", 0, -90, 90, L"deg"},
    {L"LeftHandYawDeg", L"Hand yaw", 0, -90, 90, L"deg"},
    {L"LeftHandRollDeg", L"Hand roll", 0, -90, 90, L"deg"},
    {L"RightHandOutwardCm", L"Hand outward / inward", 0, -20, 20, L"cm"},
    {L"RightHandUpCm", L"Hand up / down", 0, -20, 20, L"cm"},
    {L"RightHandForwardCm", L"Hand forward / back", 0, -20, 20, L"cm"},
    {L"RightHandPitchDeg", L"Hand pitch", 0, -90, 90, L"deg"},
    {L"RightHandYawDeg", L"Hand yaw", 0, -90, 90, L"deg"},
    {L"RightHandRollDeg", L"Hand roll", 0, -90, 90, L"deg"},
    {L"SwordPitchDeg", L"Weapon pitch", -30, -90, 90, L"deg"},
    {L"SwordYawDeg", L"Weapon yaw", 0, -90, 90, L"deg"},
    {L"SwordRollDeg", L"Weapon roll", 0, -90, 90, L"deg"},
    {L"BowPitchDeg", L"Bow pitch", -34, -90, 90, L"deg"},
    {L"BowYawDeg", L"Bow yaw", 0, -90, 90, L"deg"},
    {L"BowRollDeg", L"Bow roll", 0, -90, 90, L"deg"},
    {L"BowFullDrawCm", L"Full draw length", 50, 20, 80, L"cm"},
    {L"BowGrabRadiusCm", L"String grab radius", 15, 5, 30, L"cm"},
    {L"BowAimSmoothingMs", L"Aim stabilization", 100, 0, 250, L"ms"},
    {L"BowGripForwardCm", L"Grip toward fingers", 9, -15, 15, L"cm"},
    {L"BowGripDepthCm", L"Grip palm depth", 2, -15, 15, L"cm"},
    {L"BowGripSideCm", L"Grip across palm", 0, -15, 15, L"cm"},
    {L"TwoHandPitchDeg", L"Weapon pitch", -30, -90, 90, L"deg"},
    {L"TwoHandYawDeg", L"Weapon yaw", 0, -90, 90, L"deg"},
    {L"TwoHandRollDeg", L"Weapon roll", 0, -90, 90, L"deg"},
    {L"TwoHandGripRadiusCm", L"Support grip radius", 10, 5, 25, L"cm"},
    {L"StaffPitchDeg", L"Weapon pitch", 0, -90, 90, L"deg"},
    {L"StaffYawDeg", L"Weapon yaw", 0, -90, 90, L"deg"},
    {L"StaffRollDeg", L"Weapon roll", 0, -90, 90, L"deg"},
    {L"StaffGripRadiusCm", L"Support grip radius", 15, 5, 25, L"cm"},
    {L"StaffGripForwardCm", L"Grip toward fingers", 4, -10, 10, L"cm"},
    {L"StaffGripDepthCm", L"Grip palm depth", 2, -10, 10, L"cm"},
    {L"CrossbowPitchDeg", L"Weapon pitch", -35, -90, 90, L"deg"},
    {L"CrossbowYawDeg", L"Weapon yaw", 0, -90, 90, L"deg"},
    {L"CrossbowRollDeg", L"Weapon roll", 0, -90, 90, L"deg"},
    {L"CrossbowGripForwardCm", L"Grip toward fingers", 15, -15, 15, L"cm"},
    {L"CrossbowGripDepthCm", L"Grip palm depth", 4, -15, 15, L"cm"},
    {L"CrossbowGripSideCm", L"Grip across palm", 4, -15, 15, L"cm"},
    {L"MagicGripForwardCm", L"Grip toward fingers", 4, -15, 15, L"cm"},
    {L"MagicGripDepthCm", L"Grip palm depth", 2, -15, 15, L"cm"},
    {L"MagicGripSideCm", L"Grip across palm", 0, -15, 15, L"cm"},
    {L"RadialScalePercent", L"Radial menu size", 65, 40, 120, L"%"},
    {L"OneHandDamageSwingCm", L"Damage swing minimum", 200, 10, 200, L"cm", 10},
    {L"OneHandStaminaSwingCm", L"Stamina swing minimum", 200, 10, 200, L"cm", 10},
    {L"TwoHandDamageSwingCm", L"Damage swing minimum", 200, 10, 200, L"cm", 10},
    {L"TwoHandStaminaSwingCm", L"Stamina swing minimum", 200, 10, 200, L"cm", 10},
    {L"StaffDamageSwingCm", L"Damage swing minimum", 200, 10, 200, L"cm", 10},
    {L"StaffStaminaSwingCm", L"Stamina swing minimum", 200, 10, 200, L"cm", 10},
    {L"ShieldPitchDeg", L"Shield pitch", 0, -90, 90, L"deg"},
    {L"ShieldYawDeg", L"Shield yaw", 0, -90, 90, L"deg"},
    {L"ShieldRollDeg", L"Shield roll", 0, -90, 90, L"deg"},
    {L"ShieldGripForwardCm", L"Grip toward fingers", 0, -15, 15, L"cm"},
    {L"ShieldGripDepthCm", L"Grip palm depth", -10, -15, 15, L"cm"},
    {L"ShieldGripSideCm", L"Grip across palm", 0, -15, 15, L"cm"},
    {L"OneHandSwingSpeedCmPerSec", L"Minimum swing speed", 600, 50, 800, L"cm/s", 10},
    {L"TwoHandSwingSpeedCmPerSec", L"Minimum swing speed", 600, 50, 800, L"cm/s", 10},
    {L"StaffSwingSpeedCmPerSec", L"Minimum swing speed", 600, 50, 800, L"cm/s", 10},
    {L"OneHandStabDistanceCm", L"Stab distance minimum", 25, 5, 100, L"cm", 5},
    {L"TwoHandStabDistanceCm", L"Stab distance minimum", 15, 5, 100, L"cm", 5},
    {L"StaffStabDistanceCm", L"Stab distance minimum", 15, 5, 100, L"cm", 5},
    {L"OneHandStabSpeedCmPerSec", L"Minimum stab speed", 100, 40, 800, L"cm/s", 10},
    {L"TwoHandStabSpeedCmPerSec", L"Minimum stab speed", 50, 40, 800, L"cm/s", 10},
    {L"StaffStabSpeedCmPerSec", L"Minimum stab speed", 50, 40, 800, L"cm/s", 10},
    {L"OneHandWeightPercent", L"Weight", 40, 0, 200, L"%", 5},
    {L"OneHandStabilizationMs", L"Stabilization", 20, 0, 150, L"ms", 5},
    {L"TwoHandSupportWeightPercent", L"Two-hand weight", 30, 0, 200, L"%", 5},
    {L"TwoHandSupportStabilizationMs", L"Two-hand stabilization", 40, 0, 150, L"ms", 5},
    {L"StaffOneHandWeightPercent", L"One-hand weight", 65, 0, 200, L"%", 5},
    {L"StaffOneHandStabilizationMs", L"One-hand stabilization", 45, 0, 150, L"ms", 5},
    {L"StaffTwoHandWeightPercent", L"Two-hand weight", 20, 0, 200, L"%", 5},
    {L"StaffTwoHandStabilizationMs", L"Two-hand stabilization", 15, 0, 150, L"ms", 5},
    {L"BowStringAimInfluencePercent", L"String-hand aim influence", 20, 0, 40, L"%", 5},
    {L"TwoHandGripLossDistanceCm", L"Grip loss distance", 40, 10, 100, L"cm", 5},
    {L"StaffGripLossDistanceCm", L"Grip loss distance", 40, 10, 100, L"cm", 5},
    {L"CrossbowGripLossDistanceCm", L"Grip loss distance", 20, 10, 100, L"cm", 5},
    {L"FogIntensityPercent", L"Fog intensity", 100, 0, 100, L"%", 5},
    {L"WorldLabelScalePercent", L"World label size", 50, 25, 150, L"%", 5},
    {L"LeftHandHudXcm", L"Position X", -7, -50, 50, L"cm"},
    {L"LeftHandHudYcm", L"Position Y", 0, -50, 50, L"cm"},
    {L"LeftHandHudZcm", L"Position Z", 0, -50, 50, L"cm"},
    {L"LeftHandHudPitchDeg", L"Pitch", -90, -180, 180, L"deg"},
    {L"LeftHandHudYawDeg", L"Yaw", 180, -180, 180, L"deg"},
    {L"LeftHandHudRollDeg", L"Roll", -90, -180, 180, L"deg"},
    {L"RightHandHudXcm", L"Position X", 7, -50, 50, L"cm"},
    {L"RightHandHudYcm", L"Position Y", 0, -50, 50, L"cm"},
    {L"RightHandHudZcm", L"Position Z", 0, -50, 50, L"cm"},
    {L"RightHandHudPitchDeg", L"Pitch", 90, -180, 180, L"deg"},
    {L"RightHandHudYawDeg", L"Yaw", 0, -180, 180, L"deg"},
    {L"RightHandHudRollDeg", L"Roll", -90, -180, 180, L"deg"},
    {L"LeftHandHudScalePercent", L"Scale", 50, 25, 200, L"%", 5},
    {L"RightHandHudScalePercent", L"Scale", 50, 25, 200, L"%", 5},
};
inline constexpr unsigned VrNumberCount = sizeof(VrNumbers)/sizeof(VrNumbers[0]);
inline constexpr int InteractionAimMinimum=-200,InteractionAimMaximum=200;
struct VrSettings {
    mapping::Config controllerBindings;
    int numbers[VrNumberCount]{};
    VrSettings(){for(unsigned i=0;i<VrNumberCount;++i)numbers[i]=VrNumbers[i].initial;}
    bool autoStart=true, hideHead=true, keepFirstPerson=true, controllers=true, stableHorizon=true, smoothCamera=true, trackedArms=true, stableBody=true, roomScaleWalking=true;
    bool bowReticle=true,instantControllerTurn=true;
    bool handHudEnabled=true;
    bool fastPickup=true,firstPersonDialogue=true,firstPersonChest=true,firstPersonLectern=true;
    bool hideCompass=true,showSlotBar=false,showVitalsPanel=false;
    bool thirdPersonOrbit=true,physicalCrouching=true;
    bool handsOnly=false;
    bool toggleSupportGrip=true;
    bool twoHandedMelee=true,heavyTwoHand=true;
    int cameraSmoothingMs=40, cameraVerticalSmoothingMs=100; // Native translation only; XR poses stay immediate.
    int renderWidth=2560,renderHeight=2560; // Desired XR output; 0/0 uses native video settings.
    int ffrMode=2,ffrDiameterPercent=60,ffrTransitionPercent=3;
    bool dlaaExperimental=false;
    bool fxaaEnabled=true,smaaEnabled=false;
    int smaaQuality=2;
    int particleQuality=2; // Native, Balanced VR, Performance VR; visual effects only.
    OpenXrRuntimeChoice openXrRuntime=OpenXrRuntimeChoice::Vdxr;
    UpscaleMethod upscaleMethod=UpscaleMethod::Off;int upscaleScalePercent=100,upscaleSharpness=10;
    int interactionReachCm=120; // Controller-to-surface reach; ranged aiming stays native.
    int pickupReachCm=180; // Reach thin ground loot from a standing hand.
    int interactionAimUpPercent=-25; // World-elevation slope bias; does not change hands, menu pointer or ranged aim.
};
struct RenderResolution {int width,height;wchar_t const* label;};
inline constexpr RenderResolution VrResolutions[] = {
    {0,0,L"Use game settings"},
    {1920,1080,L"1920 × 1080 — performance"},
    {2560,1440,L"2560 × 1440 — QHD"},
    {3200,1800,L"3200 × 1800"},
    {3840,2160,L"3840 × 2160 — 4K"},
    {5120,2880,L"5120 × 2880 — 5K"},
    {2560,2560,L"2560 × 2560 — square VR"},
    {2880,2880,L"2880 × 2880 — square VR"},
    {3200,3200,L"3200 × 3200 — square VR"}
};
inline constexpr unsigned VrResolutionCount=sizeof(VrResolutions)/sizeof(VrResolutions[0]);
inline int ResolutionIndex(int width,int height) {
    for(unsigned i=0;i<VrResolutionCount;++i)if(VrResolutions[i].width==width&&VrResolutions[i].height==height)return int(i);
    return 0;
}
inline int BoundVrNumber(VrSetting const& field,int value){int clamped=std::clamp(value,field.minimum,field.maximum);return field.minimum+int(std::round(float(clamped-field.minimum)/float(field.step)))*field.step;}
inline int ReadVrNumber(wchar_t const* path, VrSetting const& field) {
    wchar_t text[64],fallback[32];swprintf_s(fallback,L"%d",field.initial);
    GetPrivateProfileStringW(L"Stereo",field.key,fallback,text,64,path);
    wchar_t* end=nullptr;long value=wcstol(text,&end,10);
    if(end==text||*end!=L'\0')return field.initial;
    return BoundVrNumber(field,int(std::clamp(value,long(field.minimum),long(field.maximum))));
}
inline VrSettings LoadVrSettings(wchar_t const* path) {
    VrSettings s;s.controllerBindings=mapping::Load(path);for(unsigned i=0;i<VrNumberCount;++i)s.numbers[i]=ReadVrNumber(path,VrNumbers[i]);
    s.autoStart=GetPrivateProfileIntW(L"Stereo",L"AutoStart",1,path)!=0;
    s.instantControllerTurn=GetPrivateProfileIntW(L"Locomotion",L"InstantControllerTurn",1,path)!=0;
    s.thirdPersonOrbit=GetPrivateProfileIntW(L"Controllers",L"ThirdPersonOrbit",1,path)!=0;
    s.physicalCrouching=GetPrivateProfileIntW(L"Controllers",L"PhysicalCrouching",1,path)!=0;
    s.handsOnly=GetPrivateProfileIntW(L"Controllers",L"HandsOnly",0,path)!=0;
    s.toggleSupportGrip=GetPrivateProfileIntW(L"Controllers",L"ToggleSupportGrip",1,path)!=0;
    s.bowReticle=GetPrivateProfileIntW(L"Stereo",L"BowReticle",1,path)!=0;
    s.hideCompass=GetPrivateProfileIntW(L"Stereo",L"HideCompass",1,path)!=0;
    s.showSlotBar=GetPrivateProfileIntW(L"Stereo",L"ShowSlotBar",0,path)!=0;
    s.handHudEnabled=GetPrivateProfileIntW(L"Stereo",L"HandHudEnabled",1,path)!=0;
    s.fastPickup=GetPrivateProfileIntW(L"Stereo",L"FastPickup",1,path)!=0;
    s.firstPersonDialogue=GetPrivateProfileIntW(L"Stereo",L"FirstPersonDialogue",1,path)!=0;
    s.firstPersonChest=GetPrivateProfileIntW(L"Stereo",L"FirstPersonChest",1,path)!=0;
    s.firstPersonLectern=GetPrivateProfileIntW(L"Stereo",L"FirstPersonLectern",1,path)!=0;
    s.showVitalsPanel=GetPrivateProfileIntW(L"Stereo",L"ShowVitalsPanel",0,path)!=0;
    s.twoHandedMelee=GetPrivateProfileIntW(L"Stereo",L"TwoHandedMelee",1,path)!=0;
    s.heavyTwoHand=GetPrivateProfileIntW(L"Stereo",L"HeavyTwoHand",1,path)!=0;
    s.hideHead=GetPrivateProfileIntW(L"Stereo",L"HideFirstPersonHead",1,path)!=0;
    s.keepFirstPerson=GetPrivateProfileIntW(L"Stereo",L"KeepFirstPerson",1,path)!=0;
    s.controllers=GetPrivateProfileIntW(L"Controllers",L"Enabled",1,path)!=0;
    s.stableHorizon=GetPrivateProfileIntW(L"Stereo",L"StableHorizon",1,path)!=0;
    s.smoothCamera=GetPrivateProfileIntW(L"Stereo",L"SmoothCamera",1,path)!=0;
    s.cameraSmoothingMs=std::clamp(int(GetPrivateProfileIntW(L"Stereo",L"CameraSmoothingMs",40,path)),0,250);
    s.cameraVerticalSmoothingMs=std::clamp(int(GetPrivateProfileIntW(L"Stereo",L"CameraVerticalSmoothingMs",100,path)),0,250);
    s.trackedArms=GetPrivateProfileIntW(L"Controllers",L"TrackedArms",1,path)!=0;
    s.stableBody=GetPrivateProfileIntW(L"Controllers",L"StableBody",1,path)!=0;
    s.roomScaleWalking=GetPrivateProfileIntW(L"Locomotion",L"RoomScaleWalking",1,path)!=0;
    s.interactionReachCm=std::clamp(int(GetPrivateProfileIntW(L"Controllers",L"InteractionReachCm",120,path)),30,250);
    s.pickupReachCm=std::clamp(int(GetPrivateProfileIntW(L"Controllers",L"PickupReachCm",180,path)),30,250);
    s.interactionAimUpPercent=std::clamp(int(GetPrivateProfileIntW(L"Controllers",L"InteractionAimUpPercent",-25,path)),InteractionAimMinimum,InteractionAimMaximum);
    int width=GetPrivateProfileIntW(L"Stereo",L"RenderWidth",2560,path),height=GetPrivateProfileIntW(L"Stereo",L"RenderHeight",2560,path);
    auto const& resolution=VrResolutions[ResolutionIndex(width,height)];s.renderWidth=resolution.width;s.renderHeight=resolution.height;
    s.ffrMode=FfrMode(int(GetPrivateProfileIntW(L"Stereo",L"FfrMode",2,path)));
    s.ffrDiameterPercent=std::clamp(int(GetPrivateProfileIntW(L"Stereo",L"FfrDiameterPercent",60,path)),40,95);
    s.ffrTransitionPercent=FfrTransition(int(GetPrivateProfileIntW(L"Stereo",L"FfrTransitionPercent",3,path)));
    s.upscaleMethod=ValidUpscaleMethod(int(GetPrivateProfileIntW(L"Stereo",L"UpscaleMethod",0,path)));
    s.dlaaExperimental=GetPrivateProfileIntW(L"Stereo",L"DlaaExperimental",0,path)!=0;
    s.smaaQuality=ValidSmaaQuality(int(GetPrivateProfileIntW(L"Stereo",L"SmaaQuality",2,path)));
    s.particleQuality=particles::Quality(int(GetPrivateProfileIntW(L"Stereo",L"ParticleEffectQuality",2,path)));
    s.smaaEnabled=GetPrivateProfileIntW(L"Stereo",L"SmaaEnabled",0,path)!=0;
    s.fxaaEnabled=!s.smaaEnabled&&GetPrivateProfileIntW(L"Stereo",L"FxaaEnabled",1,path)!=0;
    s.openXrRuntime=ValidOpenXrRuntime(int(GetPrivateProfileIntW(L"Stereo",L"OpenXrRuntime",0,path)));
    s.upscaleScalePercent=std::clamp(int(GetPrivateProfileIntW(L"Stereo",L"UpscaleScalePercent",100,path)),50,100);
    s.upscaleSharpness=std::clamp(int(GetPrivateProfileIntW(L"Stereo",L"UpscaleSharpness",10,path)),0,100);
    return s;
}
// Use a same-directory temporary copy: preserve unrelated keys/comments and
// replace only after every write succeeds. The previous complete INI is .bak.
template<class Writer> inline bool SaveProfileAtomic(wchar_t const* path,Writer writeProfile) {
    wchar_t directory[MAX_PATH],temporary[MAX_PATH],backup[MAX_PATH];
    if(wcslen(path)>=MAX_PATH-5)return false;
    wcscpy_s(directory,path);auto slash=wcsrchr(directory,L'\\');if(!slash)return false;*slash=0;
    if(!GetTempFileNameW(directory,L"gvr",0,temporary))return false;
    bool existed=GetFileAttributesW(path)!=INVALID_FILE_ATTRIBUTES;
    bool ok=!existed||CopyFileW(path,temporary,FALSE)!=FALSE;
    if(ok)ok=writeProfile(temporary);
    // An all-null flush returns zero even on success; normal key writes above
    // supply the success result. Flush before replacing the complete file.
    if(ok)WritePrivateProfileStringW(nullptr,nullptr,nullptr,temporary);
    swprintf_s(backup,L"%s.bak",path);
    if(ok&&existed)ok=CopyFileW(path,backup,FALSE)!=FALSE;
    if(ok)ok=MoveFileExW(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
    if(!ok)DeleteFileW(temporary);
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,path);return ok;
}
inline bool SaveVrSettings(wchar_t const* path,VrSettings const& s) {
    return SaveProfileAtomic(path,[&](wchar_t const* temporary){bool ok=true;
        auto write=[&](wchar_t const* key,int value) {wchar_t text[32];swprintf_s(text,L"%d",value);
            if(ok)ok=WritePrivateProfileStringW(L"Stereo",key,text,temporary)!=FALSE;};
        for(unsigned i=0;i<VrNumberCount;++i)write(VrNumbers[i].key,BoundVrNumber(VrNumbers[i],s.numbers[i]));
        write(L"AutoStart",s.autoStart);write(L"HideFirstPersonHead",s.hideHead);write(L"KeepFirstPerson",s.keepFirstPerson);
        write(L"StableHorizon",s.stableHorizon);
        write(L"BowReticle",s.bowReticle);
        write(L"HideCompass",s.hideCompass);write(L"ShowSlotBar",s.showSlotBar);write(L"ShowVitalsPanel",s.showVitalsPanel);write(L"HandHudEnabled",s.handHudEnabled);
        write(L"FastPickup",s.fastPickup);write(L"FirstPersonDialogue",s.firstPersonDialogue);write(L"FirstPersonChest",s.firstPersonChest);write(L"FirstPersonLectern",s.firstPersonLectern);
        write(L"TwoHandedMelee",s.twoHandedMelee);write(L"HeavyTwoHand",s.heavyTwoHand);
        write(L"SmoothCamera",s.smoothCamera);
        write(L"CameraSmoothingMs",std::clamp(s.cameraSmoothingMs,0,250));
        write(L"CameraVerticalSmoothingMs",std::clamp(s.cameraVerticalSmoothingMs,0,250));
        auto const& resolution=VrResolutions[ResolutionIndex(s.renderWidth,s.renderHeight)];
        write(L"RenderWidth",resolution.width);write(L"RenderHeight",resolution.height);
        write(L"FfrMode",FfrMode(s.ffrMode));write(L"FfrDiameterPercent",std::clamp(s.ffrDiameterPercent,40,95));
        write(L"FfrTransitionPercent",FfrTransition(s.ffrTransitionPercent));
        write(L"UpscaleMethod",int(ValidUpscaleMethod(int(s.upscaleMethod))));
        write(L"DlaaExperimental",s.dlaaExperimental);
        write(L"FxaaEnabled",s.fxaaEnabled&&!s.smaaEnabled);
        write(L"SmaaEnabled",s.smaaEnabled);
        write(L"SmaaQuality",ValidSmaaQuality(s.smaaQuality));
        write(L"ParticleEffectQuality",particles::Quality(s.particleQuality));
        write(L"OpenXrRuntime",int(ValidOpenXrRuntime(int(s.openXrRuntime))));
        write(L"UpscaleScalePercent",std::clamp(s.upscaleScalePercent,50,100));write(L"UpscaleSharpness",std::clamp(s.upscaleSharpness,0,100));
        if(ok)ok=WritePrivateProfileStringW(L"Controllers",L"Enabled",s.controllers?L"1":L"0",temporary)!=FALSE;
        if(ok)ok=WritePrivateProfileStringW(L"Controllers",L"TrackedArms",s.trackedArms?L"1":L"0",temporary)!=FALSE;
        if(ok)ok=WritePrivateProfileStringW(L"Controllers",L"StableBody",s.stableBody?L"1":L"0",temporary)!=FALSE;
        if(ok)ok=WritePrivateProfileStringW(L"Controllers",L"ThirdPersonOrbit",s.thirdPersonOrbit?L"1":L"0",temporary)!=FALSE;
        if(ok)ok=WritePrivateProfileStringW(L"Controllers",L"PhysicalCrouching",s.physicalCrouching?L"1":L"0",temporary)!=FALSE;
        if(ok)ok=WritePrivateProfileStringW(L"Controllers",L"HandsOnly",s.handsOnly?L"1":L"0",temporary)!=FALSE;
        if(ok)ok=WritePrivateProfileStringW(L"Controllers",L"ToggleSupportGrip",s.toggleSupportGrip?L"1":L"0",temporary)!=FALSE;
        if(ok){wchar_t reach[32];swprintf_s(reach,L"%d",std::clamp(s.interactionReachCm,30,250));ok=WritePrivateProfileStringW(L"Controllers",L"InteractionReachCm",reach,temporary)!=FALSE;}
        if(ok){wchar_t reach[32];swprintf_s(reach,L"%d",std::clamp(s.pickupReachCm,30,250));ok=WritePrivateProfileStringW(L"Controllers",L"PickupReachCm",reach,temporary)!=FALSE;}
        if(ok){wchar_t bias[32];swprintf_s(bias,L"%d",std::clamp(s.interactionAimUpPercent,InteractionAimMinimum,InteractionAimMaximum));ok=WritePrivateProfileStringW(L"Controllers",L"InteractionAimUpPercent",bias,temporary)!=FALSE;}
        if(ok)ok=WritePrivateProfileStringW(L"Locomotion",L"RoomScaleWalking",s.roomScaleWalking?L"1":L"0",temporary)!=FALSE;
        // Obsolete direct locomotion keys cannot re-enable the removed motor.
        if(ok)ok=WritePrivateProfileStringW(L"Locomotion",L"StableLocomotion",nullptr,temporary)!=FALSE;
        if(ok)ok=WritePrivateProfileStringW(L"Locomotion",L"DirectVelocity",nullptr,temporary)!=FALSE;
        if(ok)ok=WritePrivateProfileStringW(L"Stereo",L"LocomotionResponseMs",nullptr,temporary)!=FALSE;
        if(ok)ok=WritePrivateProfileStringW(L"Locomotion",L"InstantControllerTurn",s.instantControllerTurn?L"1":L"0",temporary)!=FALSE;
        if(ok)ok=mapping::Save(temporary,s.controllerBindings);
        return ok;
    });
}
struct UiLayout {float widthMeters=2.6f, distanceMeters=2.f, verticalMeters=-.3f;bool menu=false;};
struct UiCanvas {unsigned width,height;};
inline UiCanvas MenuCanvas(unsigned width,unsigned height,bool menu,bool cinema=false) {
    // Only native menu layout uses the landscape viewport. Eye targets, HUD
    // and video retain their full original dimensions. Same rule everywhere:
    // GUI layout/draw scale, compositor subrect and controller hit testing.
    unsigned landscape=unsigned(uint64_t(width)*9/16);
    return {width,menu&&!cinema&&landscape?std::min(height,landscape):height};
}
inline UiLayout PanelLayout(VrSettings const& s,bool menu) {
    float scale=float(s.numbers[menu?4:2])*.01f;
    return {2.6f*scale, float(s.numbers[5])*.01f, float(s.numbers[menu?3:1])*.01f,menu};
}
}
