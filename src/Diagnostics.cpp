// Gothic 3 VR — project created and maintained by PeCOS2k5.
// SPDX-License-Identifier: GPL-3.0-or-later
// Original and third-party credits: AUTHORS.md and THIRD-PARTY-NOTICES.md.
#include <g3sdk/Script.h>
#include "RuntimeProbePolicy.h"
#if defined(G3VR_CLEAN_RUNTIME) && !defined(G3VR_SCENE_COST_TRIAL) && !defined(G3VR_RENDERER_FEASIBILITY) && !defined(G3VR_DX11_VALIDATION)
#define G3VR_NAVIGATION_BUDGET_RUNTIME
#include "NativeNavigationBudget.h"
#endif
#include <windows.h>
#include <d3d9.h>
#include <process.h>
#include <atomic>
#include <type_traits>
#include <cmath>
#include <cstdio>
#include <share.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <intrin.h>
#include "TemporalRenderPhase.h"
#include "VirtualHook.h"
#include "NativeApplication.h"
#include "LocatorNativeInput.h"
#include "LocatorProjection.h"
#include "OpenXrClient.h"
#include "HeadControlMath.h"
#include "TemporaryOverride.h"
#include "NativeMovementInput.h"
#include "NativeControllerModes.h"
#include "PhysicalCrouchWalk.h"
#include "NativeControllerActions.h"
#include "NativeControllerTurn.h"
#include "NativeTurnInterpolation.h"
#include "NativeCameraReturn.h"
#include "NativeHeroVoice.h"
#include "HeadBodyHeading.h"
#include "NativeSneakAction.h"
#include "ControllerSneakToggle.h"
#include "NativeSprintPolicy.h"
#include "NativeMovementTrace.h"
#include "NativeDirectionGoal.h"
#include "GuardPose.h"
#include "NativeQuickHeal.h"
#include "InteractionTargetBox.h"
#include "InteractionFocusRanking.h"
#include "NativeAttachedParticles.h"
#include "ParticlePerformance.h"
#include "NativeFogIntensity.h"
#include "NativeHudVisibility.h"
#include "NativeRadialItems.h"
#include "NativeHandHud.h"
#include "NativePickup.h"
#include "NativeChestVisual.h"
#include "NativeLecternCamera.h"
#include "ControllerMappingIpc.h"
#include "NativeMagicHandEffects.h"
#include "RenderAlphaSnapshot.h"
#include "StereoWorldPass.h"
#include "NativeArmedSneak.h"
#include "CinemaScope.h"
#include "WorldBindingsLifecycle.h"
#include "NativeComfortProfile.h"
#include "RoomScale.h"
#include "RoomNativeState.h"
#include "RoomMotionScope.h"
#include "StereoRuntime.h"
#include "ProfilingOptions.h"
#include "NativeOcclusion.h"
#include "PupOcclusionProfile.h"
#include "CameraRenderHook.h"
#include "UiMouseHook.h"
#include "UiMouseCoordinates.h"
#include "ControllerInput.h"
#include "WorldAim.h"
#include "FocusHook.h"
#include "PlayerFocusDispatch.h"
#include "PupPrefetchBudget.h"
#include "PupLowPolyProfile.h"
#include <shlobj.h>
#include "GameStereoMath.h"
#include "CameraComfort.h"
#include "DialogCameraLock.h"
#include "ThirdPersonOrbit.h"
#include "NativeCameraStabilizer.h"
#include "StereoCameraState.h"
#include "FirstPersonCameraLock.h"
#include "AnimalCamera.h"
#include "AnimalReturn.h"
#include "FirstPersonHeadVisibility.h"
#include "NativeTrialMode.h"
#include "NativeTrackedArms.h"
#include "ArmPaletteHook.h"
#include "NativeMelee.h"
#include "NativeCombatMovementRecovery.h"
#include "NativeBow.h"
#include "NativeMagic.h"
#include "NativeFinisher.h"
#include "FinisherSearchGate.h"
#include "BowReticle.h"
#include "ControllerFocusQuery.h"
#include "NativeRangedLabelFont.h"
#include "NativeTrackedBow.h"
#include "NativeHeldWeaponVisuals.h"
#include "NativeRenderProfile.h"
#include "NativeSceneExecutionProfile.h"
#include "NativeSceneAdapter.h"
#include "NativeMaterialCapture.h"
#include "RendererFeasibility.h"
#ifdef G3VR_SCENE_COST_TRIAL
#include "SceneCostTrial.h"
#include "SceneCostEntries.h"
#ifdef G3VR_ENTITY_COST_TRIAL
#include "EntityCostEntries.h"
#endif
#ifdef G3VR_DRAW_COST_TRIAL
#include "DrawCostTrial.h"
#endif
#include <fstream>
#include <filesystem>
#endif

namespace
{
#ifdef G3VR_NAVIGATION_BUDGET_RUNTIME
diagnostic::navigation::NativeBudget gNavigationBudget;
#endif
#ifdef G3VR_SCENE_COST_TRIAL
void FlushSceneCost(bool exiting=false)noexcept;
#endif
#ifdef G3VR_ENTITY_COST_TRIAL
uint64_t EntityCostParticleStarted()noexcept;
void EntityCostParticleFinished(uint64_t started)noexcept;
#endif
diagnostic::NativeTrackedArms gTrackedArms;
diagnostic::ArmPaletteHook gArmPaletteHook;
diagnostic::NativeMelee gMelee;
diagnostic::NativeCombatMovementRecovery gCombatMovementRecovery;
diagnostic::NativeFinisher gFinisher;
diagnostic::combat::FinisherSearchGate gFinisherSearch;
diagnostic::PlayerFocusDispatch gPlayerFocusDispatch;
diagnostic::NativeBow gBow;
diagnostic::NativeMagic gMagic;
diagnostic::BowReticle gBowReticle;
diagnostic::NativeHudVisibility gNativeHudVisibility;
diagnostic::NativeRadialItems gNativeRadialItems;
diagnostic::NativeHandHud gHandHud;
diagnostic::NativePickup gPickup;
diagnostic::NativeChestVisual gChestVisual;
diagnostic::ChestViewTransition gChestView,gLecternView;
diagnostic::NativeTrackedBow gTrackedBow;
diagnostic::FocusHook gFlameProcessHook,gFlameAlphaHook,gParticleVerticesHook,gParticleWorldHook,gSignpostFocusHook;
diagnostic::FocusHook gFocusNameSizeHook;
diagnostic::RangedFocusLabel gRangedFocusLabel;
diagnostic::NativeRangedLabelFont gRangedLabelFont;
diagnostic::WorldLabelFrameCache gWorldLabelFrameCache;
bool gRangedLabelFault=false,gRangedLabelDrawing=false;
diagnostic::FocusHook gParticleBudgetHook,gParticleRefractionHook,gFogIntensityHook;
using ResetNativeFog=void(__stdcall*)();
ResetNativeFog gResetNativeFog=nullptr;
diagnostic::particles::RenderLease gParticleRenderLease;
bool gParticlePerformanceReady=false;
bool gParticleBudgetReported[3]{},gParticleHeatReported[3]{};
diagnostic::visual::ParticleAttachmentFrame gTorchParticles;diagnostic::visual::ParticleWorldScope gParticleWorld;
thread_local bool gTrackedRenderThread=false;
bool gTorchHooksReady=false,gMagicHandLayoutReady=false;
diagnostic::visual::NativeHandEffectScope gMagicHandEffects;
bool gStaticTrialHooksOptIn=false;
bool gFlameHooksReady=false,gFlameHooksFault=false;
diagnostic::visual::FlameTailTargets gFlameTargets;
uint64_t gFlameProcessSkipped=0,gFlameDrawSkipped=0;
diagnostic::KnockoutCamera gKnockoutCamera;diagnostic::ActivityCameraGate gActivityCameraGate;
diagnostic::CameraReturnTransition gCameraReturn;
bool CameraThirdPersonActivity(eCEntity* player){
    bool immediate=false,requested=diagnostic::NativeActivity(player,&immediate);
    bool activity=gActivityCameraGate.Step(reinterpret_cast<uintptr_t>(player),requested,GetTickCount64(),immediate);
    return diagnostic::NativeKnockoutThirdPerson(player,gKnockoutCamera)||activity||gFinisher.Busy(player);
}
bool gRadialOpen=false;
bool BlockPhysicalSwordClick(gCCharacterControl_PS* self,uint16_t key);
diagnostic::ParryLoadout NativeParryLoadout(eCEntity* player){
    if(!diagnostic::Readable(player,sizeof(eCDynamicEntity)))return {};
    auto* right=gCEntity::GetWeapon(player,GETrue);auto* left=gCEntity::GetWeapon(player,GEFalse);
    int r=diagnostic::Readable(right,sizeof(eCEntity))&&right!=player?gCEntity::GetUseType(right):0;
    int l=diagnostic::Readable(left,sizeof(eCEntity))&&left!=player?gCEntity::GetUseType(left):0;
    return {r,l};
}
bool AllowNativeParry(eCEntity* player){return NativeParryLoadout(player).Compatible();}
constexpr unsigned QueueSize = 2048;
constexpr char const* HookNames[] = {"OnProcess", "OnPreRender", "OnRenderDraw", "OnPostRender"};
enum class Kind { Start, Message, Hook, Validation, Phase, Summary, Camera, Device, Marker, VrPose, VrInfo, HeadControl, Velocity, StereoEye, NativeMovementTrace };

// POD records: no allocation, formatting or file I/O in the observed callbacks.
struct Event
{
    Kind kind = Kind::Message;
    DWORD thread = 0;
    int hook = 0;
    bool exit = false;
    uint64_t process = 0, render = 0;
    int64_t ticks = 0;
    uintptr_t pointer = 0, pointer2 = 0;
    uintptr_t resolvedOriginal = 0, resolvedExpected = 0, importTarget = 0;
    char originalModule[64]{}, targetModule[64]{};
    unsigned char originalBytes[16]{};
    DWORD code = 0, sessionKey = 0;
    char text[192]{};
    uint64_t counts[4]{}, dropped = 0;
    float position[3]{}, rotation[3]{}, scale[3]{};
    float fov = 0, aspect = 0, clip[2]{};
    float view[16]{}, projection[16]{};
    DWORD viewport[4]{}, target[4]{}, adapter = 0, deviceType = 0, behavior = 0;
    bool ex = false, gameRunning = false;
    uintptr_t cameraAI = 0;
    bool cameraModeValid = false, firstPerson = false, tempFirstPerson = false, dialogCamera = false;
    diagnostic::HeadPose headPose;
    diagnostic::HeadsetInfo headset;
    bool controlEnabled=false, centered=false, applied=false;
    bool movementSample=false;
    uint64_t actionCalls=0, actionRestores=0;
    float nativeRotation[3]{}, requestedRotation[3]{}, centerQuaternion[4]{};
    float viewRotationError=0;
};

struct Configuration
{
    bool enabled = true, autoTrace = true;
    DWORD sampleMs = 1000, traceFrames = 120, maxLogMiB = 16;
    bool trackingEnabled = true;
    DWORD trackingPollMs = 11, trackingSampleMs = 100, trackingRetryMs = 5000;
    bool headControlAllowed=true;
    bool headDirectedMovement=true;
    DWORD maxPoseAgeMs=50;
    bool stereoEnabled=true;
    bool keepFirstPerson=true;
    bool hideFirstPersonHead=true;
    float firstPersonHeightOffsetCm=-25.f;
    float unitsPerMeter=100.f;
} gConfig;
diagnostic::VrSettings gVrSettings;
bool gFirstPersonLockAttempted=false;
bool gFirstPersonPermissionFault=false,gHeadVisibilityFault=false;
bool gRoomScaleFault=false;
uint64_t gHumanHeightRestoreUntil=0,gRoomScaleMoves=0,gRoomScaleNextLog=0,gAutoFirstPersonNext=0,gAutoFirstPersonProcess=UINT64_MAX;
bool gAnimalCameraActive=false,gActivityCameraActive=false,gHumanHeightSaved=false;float gHumanHeight=0;diagnostic::AnimalReturn gAnimalReturn;
int64_t gRoomScalePreviousQpc=0;

HMODULE gModule = nullptr;
diagnostic::LocatorNativeInput gLocatorNativeInput;
diagnostic::LocatorProjection gLocatorProjection;
float gLocatorEyeView[2][16]{};
uint64_t gLocatorProjectionUses=0;
bCMatrix* __fastcall LocatorWorldToScreen(eCCameraBase const* camera,void*,bCMatrix* result){
    using Original=bCMatrix*(__thiscall*)(eCCameraBase const*,bCMatrix*);
    auto* output=reinterpret_cast<Original>(gLocatorNativeInput.projectionHook.original)(camera,result);float corrected[16];
    if(output&&gLocatorProjection.Read(GetTickCount64(),corrected)){
        for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)(*output)(r,c)=corrected[r*4+c];++gLocatorProjectionUses;
    }
    return output;
}
bool __fastcall LocatorKeyPressed(void* keyboard,void*,int key){
    using Original=bool(__thiscall*)(void*,int);
    bool native=reinterpret_cast<Original>(gLocatorNativeInput.hook.original)(keyboard,key);
    return gLocatorNativeInput.pulse.Read(unsigned(key),GetTickCount64())||native;
}
HANDLE gWake = nullptr;
SRWLOCK gQueueLock = SRWLOCK_INIT;
Event gQueue[QueueSize];
unsigned gRead = 0, gWrite = 0;
std::atomic<uint64_t> gDropped{0}, gProcess{0}, gRender{0}, gCounts[4]{};
std::atomic<uint64_t> gTraceEnd{0};
std::atomic<bool> gLogging{false}, gSample{false}, gCameraFault{false}, gDeviceFault{false};
std::atomic<bool> gCameraModeFault{false};
LONG gInitialized = 0;
LARGE_INTEGER gFrequency{}, gStarted{};
wchar_t gIni[MAX_PATH]{}, gLogDirectory[MAX_PATH]{};
wchar_t gRuntimeLoader[MAX_PATH]{};
SRWLOCK gHeadPoseLock = SRWLOCK_INIT;
diagnostic::HeadPose gHeadPose;
diagnostic::VirtualHook gHooks[4];
diagnostic::VirtualHook gSessionProcessHook;
diagnostic::VirtualHook gMovementHook, gActionHook, gPhysicsProcessHook,gRoomWorldHook;
diagnostic::RoomMotionScope gRoomMotionScope;
eCApplication* gApplication = nullptr;
std::atomic<DWORD> gApplicationThread{0};
uint64_t gNextSampleMs = 0;
// Accessed only by the application's original update/render thread.
struct HeadControl
{
    bool enabled=false, centered=false, recenterPending=false, active=false, fault=false;
    diagnostic::Orientation center;
    char origin[16]{};
    eCCameraBase* camera=nullptr;
    float nativeRotation[3]{}, requestedRotation[3]{}, nativeView[16]{};
    diagnostic::Rotation3 expected;
    uint64_t appliedCount=0, restoredCount=0;
    uint64_t centerSession=0;
    uint64_t movementCalls=0, movementRestores=0;
    uint64_t actionCalls=0, actionRestores=0;
    bool actionActive=false, movementActive=false, movementHeadingValid=false;
    float movementHeading=0;
    diagnostic::HeadPose appliedPose;
    char const* previousState=nullptr;
} gHeadControl;
diagnostic::ControllerSneakToggle gSneakToggle;
diagnostic::GuardPose gGuardPose;diagnostic::NativeQuickHeal gQuickHeal;
diagnostic::NativeArmedSneak gArmedSneak;
diagnostic::NativeSneakAction gNativeSneakAction;
bool gNativeSneakFault=false;
diagnostic::NativeControllerModes gControllerModes;
diagnostic::PhysicalCrouchWalk gPhysicalCrouchWalk;
diagnostic::NativeControllerActions gNativeMovement;bool gNativeMovementFault=false;
diagnostic::NativeMovementTraceWindow gMovementTrace;
diagnostic::DialogCameraLock gDialogCamera;
diagnostic::ThirdPersonOrbit gThirdPersonOrbit;
struct MovementTraceInput {uint64_t frame=UINT64_MAX;float x=0,z=0;bool sprintSelected=false,sprintPressed=false;} gMovementTraceInput;
diagnostic::HeroVoiceChannel gHeroVoice;
diagnostic::NativeVoiceApi gVoiceApi;
diagnostic::FocusHook gVoiceStartHook,gVoiceUpdateHook;
bool gVoiceReady=false,gVoiceFault=false;
diagnostic::HeadBodyHeading gHeadBodyHeading;
diagnostic::BodyFacingLease gBodyFacingLease;
uintptr_t gBodyFacingControl=0;
bool gBodyFacingFault=false;
diagnostic::FocusHook gMouseFilterXHook,gWishedRotationHook,gPlayVideoHook;
diagnostic::NativeTurnEvaluation gTurnEvaluation;
diagnostic::NativeTurnOutput gTurnOutput;
bool gNativeTurnInterpolationReady=false;
diagnostic::VirtualHook gMoviePresentHook,gMoviePresentExHook;
diagnostic::CinemaScope gCinemaScope;uint64_t gMovieEpoch=0,gMovieFrames=0;bool gMovieSkipArmed=false;
IDirect3DDevice9* gMovieDevice=nullptr;thread_local bool gMoviePresentEntered=false;

gCCameraAI_PS* gInstantTurnCamera=nullptr;gCCameraAI_PS* gLastControllerTurnCamera=nullptr;diagnostic::NativeControllerTurn gControllerYaw;
bool MenuPanelActive();
int NativeUiState();
diagnostic::StereoRuntime gStereoRuntime;
diagnostic::CameraRenderHook gCameraRenderHook;
diagnostic::FocusHook gNativeRenderHooks[diagnostic::performance::RenderEntryCount];
diagnostic::FocusHook gExecutionHooks[diagnostic::performance::ExecutionEntryCount];
thread_local bool gSceneExecutionSample=false;
thread_local bool gOcclusionWorldEye=false;
diagnostic::VirtualHook gOcclusionInfoHook;uintptr_t gOcclusionResolveReturn=0;
eCDX9Device* gOcclusionDevice=nullptr;bool gOcclusionInstallFinished=false;
struct OcclusionStats {uint64_t ready=0,conservative=0,errors=0,refused=0,nextLog=0;} gOcclusionStats;

diagnostic::FocusHook gTemporalSolidHook;uintptr_t gTemporalSolidCaller=0;
diagnostic::FocusHook gTemporalAnimationHook;
diagnostic::FocusHook gTemporalGeometryHooks[4];
diagnostic::FocusHook gGameSceneHooks[diagnostic::performance::GameSceneEntryCount];
bool gSceneProfileEnabled=false;int gSceneUpdateContext=0;
bool gNativeRenderScope=false;int gNativeRenderContext=0;
diagnostic::FocusHook gPupWorkHook;
diagnostic::pup::PrefetchBudget gPupBudget(diagnostic::performance::Frequency());
unsigned char const volatile* gPupLowPolyFlag=nullptr;
bCCriticalSectionWin32* gPupQueueLock=nullptr;uint64_t gPupStereoEpoch=0;unsigned gPupStereoEye=0;
struct PupWorkStats {uint64_t calls=0,run=0,busy=0,budget=0,bypass=0,maximum=0,nextLog=0;
    uint64_t eyesAllowed=0,eyesSync=0,worldForced=0,worldOptional=0;} gPupStats;
diagnostic::UiMouseHook gUiMouseHook;
diagnostic::ControllerInput gControllerInput;
diagnostic::ControllerTranslator gControllerTranslator;
diagnostic::FocusHook gFindFocusHook,gFocusAnglesHook,gFocusCheckHook;
struct WorldInteraction {
    bool fault=false,scope=false,crossHair=false;gCFocus_PS* focus=nullptr;diagnostic::WorldAim aim;
    diagnostic::InteractionFocusRanking ranking;
    uintptr_t selected=0;uint64_t calls=0,angles=0,surfaces=0,contacts=0,restores=0,nextLog=0,pickups=0,pickupHits=0,pickupBlocked=0,pickupNextLog=0;
} gWorldInteraction;
diagnostic::VirtualHook gProjectionHook;
diagnostic::VirtualHook gDestroyEngineHook,gDestroyWorkspaceHook;
diagnostic::FocusHook gSessionStopHook;
diagnostic::WorldBindingsLifecycle gWorldLifecycle;
diagnostic::VirtualHook gCameraPermissionHook;
uint64_t gCameraPermissionOverrides=0,gCameraPermissionNextLog=0;
struct StereoState
{
    bool rendering=false,eyeActive=false,fault=false,shuttingDown=false;
    bool framePending=false,uiRendering=false;
    eCCameraBase* camera=nullptr;
    float projection[16]{},centerPosition[3]{};
    float detailFar=0,lastDistantFar=0;
    float nativePosition[3]{},nativeRotation[3]{};
    float heightOffsetUnits=0;
    bool firstPerson=false;uintptr_t player=0;float playerPosition[3]{};diagnostic::CameraHeightAnchor heightAnchor;
    bool dialog=false,chest=false,lectern=false;uintptr_t dialogPlayer=0;
    diagnostic::NativeCameraStabilizer cameraStabilizer;
    bool basePrepared=false,holdPlayerHeading=false,playerHeadingValid=false;
    diagnostic::Rotation3 frameBaseRotation,lastPlayerRotation;
    float frameBasePosition[3]{};
    uint64_t comfortNextLog=0;
    bool hidePlayerHead=false;
    bool headVisibilityReported=false;
    uint64_t nextRetry=0,eyeDraws[2]{},restores=0;
    int cameraMode=-1;
} gStereo;

class ApplicationAccess : public eCApplication
{
public:
    using eCApplication::OnPreRender;
    using eCApplication::OnRenderDraw;
    using eCApplication::OnPostRender;
    using eCApplication::OnDestroyEngine;
    using eCApplication::OnDestroyWorkspace;
};
class DeviceAccess : public eCGfxContextAdmin
{
public:
    using eCGfxContextAdmin::sGetAPIDevice;
};
class CharacterControlAccess : public gCCharacterControl_PS
{
public: using gCCharacterControl_PS::ProcessMovements;
};

class CharacterMovementAccess : public gCCharacterMovement_PS
{
public: using gCCharacterMovement_PS::OnProcess;using gCCharacterMovement_PS::OnUpdatedWorldMatrix;using gCCharacterMovement_PS::GetCurrentMaxSpeedOfDir;using gCCharacterMovement_PS::AddToCurrentVelocity;using gCCharacterMovement_PS::SetSpeedByMode;
};
class CharacterSensorAccess : public gCCharacterSensor_PS
{
public: using gCCharacterSensor_PS::ProcessPlayerMovements;
};

Event Record(Kind kind, int hook = 0, bool exit = false)
{
    Event event;
    event.kind = kind;
    event.hook = hook;
    event.exit = exit;
    event.thread = GetCurrentThreadId();
    event.process = gProcess.load(std::memory_order_relaxed);
    event.render = gRender.load(std::memory_order_relaxed);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    event.ticks = now.QuadPart - gStarted.QuadPart;
    return event;
}

void Push(Event const& event, bool reliableStartupRecord = false)
{
    if (!gLogging.load(std::memory_order_relaxed)) return;
    if (reliableStartupRecord) AcquireSRWLockExclusive(&gQueueLock);
    else if (!TryAcquireSRWLockExclusive(&gQueueLock))
    { gDropped.fetch_add(1, std::memory_order_relaxed); return; }
    unsigned next = (gWrite + 1) % QueueSize;
    bool empty = gWrite == gRead;
    if (next == gRead) gDropped.fetch_add(1, std::memory_order_relaxed);
    else { gQueue[gWrite] = event; gWrite = next; }
    ReleaseSRWLockExclusive(&gQueueLock);
    if (empty) SetEvent(gWake);
}

bool Pop(Event& event)
{
    AcquireSRWLockExclusive(&gQueueLock);
    bool available = gRead != gWrite;
    if (available) { event = gQueue[gRead]; gRead = (gRead + 1) % QueueSize; }
    ReleaseSRWLockExclusive(&gQueueLock);
    return available;
}

void Message(char const* text, DWORD code = 0, bool reliableStartupRecord = false)
{
    Event event = Record(Kind::Message);
    strcpy_s(event.text, text);
    event.code = code;
    Push(event, reliableStartupRecord);
}

void Quoted(FILE* file, char const* text)
{
    std::fputc('"', file);
    for (unsigned char c; (c = static_cast<unsigned char>(*text++)) != 0;)
    {
        if (c == '"' || c == '\\') { std::fputc('\\', file); std::fputc(c, file); }
        else if (c < 32) std::fprintf(file, "\\u%04x", c);
        else std::fputc(c, file);
    }
    std::fputc('"', file);
}

void Float(FILE* file, float value)
{
    if (std::isfinite(value)) std::fprintf(file, "%.9g", static_cast<double>(value));
    else std::fputs("null", file);
}
void Floats(FILE* file, float const* values, unsigned count)
{
    std::fputc('[', file);
    for (unsigned i = 0; i < count; ++i) { if (i) std::fputc(',', file); Float(file, values[i]); }
    std::fputc(']', file);
}
void Address(FILE* file, char const* key, uintptr_t value)
{
    std::fprintf(file, ",\"%s\":\"0x%08lx\"", key, static_cast<unsigned long>(value));
}

void WriteEvent(FILE* file, Event const& event)
{
    std::fprintf(file, "{\"time_ms\":%.3f,\"qpc_ticks\":%lld,\"thread_id\":%lu,\"process_id\":%llu,\"render_id\":%llu",
        1000.0 * static_cast<double>(event.ticks) / static_cast<double>(gFrequency.QuadPart),
        event.ticks, event.thread, event.process, event.render);
    switch (event.kind)
    {
    case Kind::StereoEye:
        std::fprintf(file,",\"type\":\"stereo_eye\",\"eye\":%d,\"position_sdk\":",event.hook);Floats(file,event.position,3);
        std::fputs(",\"rotation_sdk\":",file);Floats(file,event.rotation,3);std::fputs(",\"projection\":",file);Floats(file,event.projection,16);
        std::fprintf(file,",\"eye_draws\":%llu",event.counts[0]);break;
    case Kind::Start:
        std::fprintf(file, ",\"type\":\"start\",\"version\":\"%s\",\"qpc_frequency\":%lld,\"pid\":%lu,"
            "\"sample_interval_ms\":%lu,\"trace_updates\":%lu,\"max_log_mib\":%lu,"
            "\"architecture\":\"x86\",\"matrix_order\":\"SDK operator(row,column), flattened row-major\"",
            G3DIAG_VERSION, gFrequency.QuadPart, GetCurrentProcessId(), gConfig.sampleMs,
            gConfig.traceFrames, gConfig.maxLogMiB);
        break;
    case Kind::Message: case Kind::Marker:
        std::fprintf(file, ",\"type\":\"%s\",\"message\":", event.kind == Kind::Marker ? "marker" : "message");
        Quoted(file, event.text);
        std::fprintf(file, ",\"code\":%lu", event.code);
        break;
    case Kind::Hook:
        std::fprintf(file, ",\"type\":\"hook\",\"name\":\"%s\",\"slot\":%lu", HookNames[event.hook], event.code);
        Address(file, "original", event.pointer);
        Address(file, "observer", event.pointer2);
        break;
    case Kind::Validation:
        std::fprintf(file, ",\"type\":\"hook_validation\",\"name\":\"%s\",\"slot\":%lu,\"result\":",
                     HookNames[event.hook], event.code);
        Quoted(file, event.text);
        Address(file, "original", event.pointer);
        Address(file, "expected", event.pointer2);
        Address(file, "resolved_original", event.resolvedOriginal);
        Address(file, "resolved_expected", event.resolvedExpected);
        Address(file, "import_target", event.importTarget);
        std::fputs(",\"original_module\":", file); Quoted(file, event.originalModule);
        std::fputs(",\"target_module\":", file); Quoted(file, event.targetModule);
        std::fputs(",\"original_bytes\":\"", file);
        for (unsigned i = 0; i < 16; ++i) std::fprintf(file, "%s%02x", i ? " " : "", event.originalBytes[i]);
        std::fputc('"', file);
        break;
    case Kind::Phase:
        std::fprintf(file, ",\"type\":\"phase\",\"name\":\"%s\",\"edge\":\"%s\"",
                     HookNames[event.hook], event.exit ? "exit" : "enter");
        break;
    case Kind::Summary:
        std::fprintf(file, ",\"type\":\"summary\",\"observed_entries\":{"
            "\"OnProcess\":%llu,\"OnPreRender\":%llu,\"OnRenderDraw\":%llu,\"OnPostRender\":%llu},\"dropped_records\":%llu",
            event.counts[0], event.counts[1], event.counts[2], event.counts[3], event.dropped);
        break;
    case Kind::Camera:
        std::fprintf(file, ",\"type\":\"camera\",\"phase\":\"%s.exit\",\"game_running\":%s",
            HookNames[event.hook], event.gameRunning ? "true" : "false");
        Address(file, "camera", event.pointer);
        Address(file, "camera_ai", event.cameraAI);
        std::fprintf(file, ",\"first_person\":%s,\"temp_first_person\":%s,\"dialog_camera\":%s",
            event.cameraModeValid ? (event.firstPerson ? "true" : "false") : "null",
            event.cameraModeValid ? (event.tempFirstPerson ? "true" : "false") : "null",
            event.cameraModeValid ? (event.dialogCamera ? "true" : "false") : "null");
        std::fputs(",\"position\":", file); Floats(file, event.position, 3);
        std::fputs(",\"rotation_sdk\":", file); Floats(file, event.rotation, 3);
        std::fputs(",\"scale\":", file); Floats(file, event.scale, 3);
        std::fputs(",\"fov_sdk\":", file); Float(file, event.fov);
        std::fputs(",\"aspect\":", file); Float(file, event.aspect);
        std::fputs(",\"clip_depth\":", file); Floats(file, event.clip, 2);
        std::fputs(",\"view\":", file); Floats(file, event.view, 16);
        std::fputs(",\"projection\":", file); Floats(file, event.projection, 16);
        break;
    case Kind::Device:
        std::fputs(",\"type\":\"device\"", file);
        Address(file, "sdk_wrapper", event.pointer);
        Address(file, "d3d9_device", event.pointer2);
        std::fprintf(file, ",\"is_d3d9ex\":%s,\"adapter\":%lu,\"device_type\":%lu,\"behavior_flags\":%lu,"
            "\"viewport\":[%lu,%lu,%lu,%lu],\"target\":{\"width\":%lu,\"height\":%lu,\"format\":%lu,\"msaa\":%lu},\"query_hresult\":%lu",
            event.ex ? "true" : "false", event.adapter, event.deviceType, event.behavior,
            event.viewport[0], event.viewport[1], event.viewport[2], event.viewport[3],
            event.target[0], event.target[1], event.target[2], event.target[3], event.code);
        break;
    case Kind::VrPose:
        std::fputs(",\"type\":\"vr_pose\",\"source\":", file); Quoted(file, event.text);
        std::fprintf(file, ",\"runtime_connected\":%s,\"device_connected\":%s,\"pose_valid\":%s,"
            "\"tracking_result\":%d,\"sample_qpc\":%lld,\"age_ms\":%.3f,\"origin\":\"%s\","
            "\"units\":\"metres\",\"quaternion_order\":\"wxyz\",\"prediction_seconds\":0",
            event.headPose.runtimeConnected ? "true" : "false", event.headPose.deviceConnected ? "true" : "false",
            event.headPose.valid ? "true" : "false", event.headPose.trackingResult, event.headPose.qpc,
            1000.0*(gStarted.QuadPart + event.ticks - event.headPose.qpc)/gFrequency.QuadPart, event.headPose.origin);
        if (event.headPose.valid)
        {
            std::fputs(",\"position\":", file); Floats(file, event.headPose.position, 3);
            std::fputs(",\"quaternion\":", file); Floats(file, event.headPose.quaternion, 4);
            std::fputs(",\"device_to_standing\":", file); Floats(file, event.headPose.transform, 12);
        }
        else std::fputs(",\"position\":null,\"quaternion\":null,\"device_to_standing\":null", file);
        break;
    case Kind::VrInfo:
        std::fputs(",\"type\":\"vr_info\",\"backend\":\"OpenXR / VDXR headless Win32\",\"model\":", file);
        Quoted(file, event.headset.model);
        std::fputs(",\"manufacturer\":", file); Quoted(file, event.headset.manufacturer);
        std::fputs(",\"tracking_system\":", file); Quoted(file, event.headset.trackingSystem);
        std::fprintf(file, ",\"recommended_eye_target\":[%u,%u],\"eye_to_head\":",
            event.headset.recommendedWidth, event.headset.recommendedHeight);
        if (event.headset.opticsValid) Floats(file, event.headset.eyeToHead, 24); else std::fputs("null",file);
        std::fputs(",\"projection_raw_left_right_top_bottom\":null", file);
        break;
    case Kind::NativeMovementTrace:
        std::fputs(",\"type\":\"native_movement_trace\",\"stage\":",file);Quoted(file,event.text);
        std::fprintf(file,",\"mask\":%lu,\"wish_mode\":%u,\"native_mode\":%u,\"reference\":%u,\"movement_flags\":%u,\"head_scope\":%s",
            event.sessionKey,event.target[0],event.target[1],event.target[2],event.target[3],event.applied?"true":"false");
        std::fprintf(file,",\"native_ground_speed\":%.4f,\"native_speed_modifier\":%.4f",event.fov,event.aspect);
        std::fputs(",\"position_world\":",file);Floats(file,event.position,3);
        std::fputs(",\"wish_world\":",file);Floats(file,event.rotation,3);
        std::fputs(",\"alignment_world\":",file);Floats(file,event.scale,3);
        std::fputs(",\"actor_z\":",file);Floats(file,event.nativeRotation,3);
        std::fputs(",\"fresh_goal_delta\":",file);Floats(file,event.requestedRotation,3);
        std::fputs(",\"velocity_world\":",file);Floats(file,event.view,3);
        std::fputs(",\"translation_target_world\":",file);Floats(file,event.view+3,3);
        std::fprintf(file,",\"timers_FLRB\":[%lu,%lu,%lu,%lu],\"looking_back\":%s",event.viewport[0],event.viewport[1],event.viewport[2],event.viewport[3],event.tempFirstPerson?"true":"false");
        std::fputs(",\"stick_xz\":",file);if(event.ex)Floats(file,event.view+6,2);else std::fputs("null",file);
        std::fprintf(file,",\"sprint_selected\":%s,\"sprint_pressed\":%s",event.ex&&event.view[8]!=0?"true":"false",event.ex&&event.view[9]!=0?"true":"false");
        std::fputs(",\"camera_view\":",file);if(event.cameraModeValid)Floats(file,event.projection,16);else std::fputs("null",file);
        break;
    case Kind::Velocity:
        std::fprintf(file,",\"type\":\"locomotion_velocity\",\"redirected_calls\":%llu",event.counts[0]);
        Address(file,"rigid_body",event.pointer);
        std::fputs(",\"native_velocity_world\":",file);Floats(file,event.position,3);
        std::fputs(",\"requested_velocity_world\":",file);Floats(file,event.rotation,3);
        std::fputs(",\"direction_world\":",file);Floats(file,event.scale,3);
        break;
    case Kind::HeadControl:
        std::fputs(",\"type\":\"head_control\",\"action\":",file); Quoted(file,event.text);
        std::fprintf(file,",\"enabled\":%s,\"centered\":%s,\"applied\":%s,\"applied_frames\":%llu,\"restored_frames\":%llu",
            event.controlEnabled?"true":"false",event.centered?"true":"false",event.applied?"true":"false",event.counts[0],event.counts[1]);
        std::fprintf(file,",\"movement_calls\":%llu,\"movement_restores\":%llu",event.counts[2],event.counts[3]);
        std::fprintf(file,",\"action_calls\":%llu,\"action_restores\":%llu,\"session_key\":%lu",event.actionCalls,event.actionRestores,event.sessionKey);
        std::fputs(",\"wished_direction_world\":",file);if(event.movementSample)Floats(file,event.position,3);else std::fputs("null",file);
        Address(file,"camera",event.pointer);
        std::fputs(",\"native_rotation_sdk\":",file);Floats(file,event.nativeRotation,3);
        std::fputs(",\"requested_rotation_sdk\":",file);Floats(file,event.requestedRotation,3);
        std::fputs(",\"readback_rotation_sdk\":",file);if(event.code)Floats(file,event.rotation,3);else std::fputs("null",file);
        std::fputs(",\"native_view\":",file);if(event.code)Floats(file,event.projection,16);else std::fputs("null",file);
        std::fputs(",\"readback_view\":",file);if(event.code)Floats(file,event.view,16);else std::fputs("null",file);
        std::fputs(",\"center_quaternion_wxyz\":",file);Floats(file,event.centerQuaternion,4);
        std::fputs(",\"view_rotation_max_error\":",file);Float(file,event.viewRotationError);
        std::fprintf(file,",\"sample_qpc\":%lld,\"age_ms\":%.3f",event.headPose.qpc,
            event.headPose.qpc?1000.0*(gStarted.QuadPart+event.ticks-event.headPose.qpc)/gFrequency.QuadPart:0.0);
        std::fputs(",\"head_quaternion_wxyz\":",file);Floats(file,event.headPose.quaternion,4);
        break;
    }
    std::fputs("}\n", file);
}

unsigned __stdcall Writer(void*)
{
    SYSTEMTIME time;
    GetLocalTime(&time);
    wchar_t path[MAX_PATH];
    _snwprintf_s(path, _TRUNCATE, L"%s\\diagnostic-%04u%02u%02u-%02u%02u%02u-%lu.jsonl",
        gLogDirectory, time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, GetCurrentProcessId());
    FILE* file = nullptr;
    file = _wfsopen(path, L"wb", _SH_DENYNO);
    if (!file)
    { gLogging.store(false); OutputDebugStringW(L"Gothic3VRDiagnostics: could not open log file\n"); return 1; }
    std::setvbuf(file, nullptr, _IOFBF, 16384);
    uint64_t nextHeartbeat = GetTickCount64() + 5000;
    for (;;)
    {
        Event event;
        while (Pop(event))
        {
            long position = std::ftell(file);
            if (position < 0 || static_cast<uint64_t>(position) + 8192 >=
                static_cast<uint64_t>(gConfig.maxLogMiB) * 1024 * 1024)
            {
                std::fputs("{\"type\":\"log_limit\",\"message\":\"session size limit reached; diagnostics stopped\"}\n", file);
                gLogging.store(false);
                std::fclose(file);
                return 0;
            }
            WriteEvent(file, event);
            if (std::ferror(file)) { gLogging.store(false); std::fclose(file); return 2; }
        }
        std::fflush(file);
        uint64_t now = GetTickCount64();
        if (diagnostic::probes::Enabled&&now >= nextHeartbeat)
        {
            Event summary = Record(Kind::Summary);
            for (unsigned i = 0; i < 4; ++i) summary.counts[i] = gCounts[i].load(std::memory_order_relaxed);
            summary.dropped = gDropped.load(std::memory_order_relaxed);
            Push(summary);
            nextHeartbeat = now + 5000;
        }
        WaitForSingleObject(gWake, 250);
    }
}

void PublishHeadPose(diagnostic::HeadPose const& pose)
{
    AcquireSRWLockExclusive(&gHeadPoseLock);
    gHeadPose = pose;
    ReleaseSRWLockExclusive(&gHeadPoseLock);
}
void CaptureHeadPose(char const* source)
{
    diagnostic::HeadPose pose;
    if (!TryAcquireSRWLockShared(&gHeadPoseLock)) return;
    pose = gHeadPose;
    ReleaseSRWLockShared(&gHeadPoseLock);
    if (!pose.qpc) return;
    Event event = Record(Kind::VrPose, 1, true);
    event.headPose = pose;
    strcpy_s(event.text, source);
    Push(event);
}
unsigned __stdcall TrackingWorker(void*)
{
    diagnostic::OpenXrClient client;
    if (!client.Load(gRuntimeLoader))
    { Message("OpenXR_loader_or_VDXR_manifest_unavailable; camera diagnostics remain active", client.LoaderError(), true); return 1; }
    Message("OpenXR_Win32_loader_ready; VDXR_manifest_selected_locally", 0, true);
    bool connected = false;
    int previousError = -1;
    uint64_t nextRetry = 0, nextLog = 0, session = 0;
    while (gLogging.load())
    {
        uint64_t now = GetTickCount64();
        if (!connected && now >= nextRetry)
        {
            connected = client.Connect();
            if (connected)
            {
                ++session;
                Message("VDXR_OpenXR_headless_session_created", 0, true);
                Event info = Record(Kind::VrInfo);
                if (client.Info(info.headset)) Push(info, true);
                previousError = -1;
            }
            else if (client.InitError() != previousError)
            {
                Event failure = Record(Kind::Message);
                failure.code = static_cast<DWORD>(client.InitError());
                sprintf_s(failure.text, "VDXR_waiting: %.140s", client.ErrorDescription());
                Push(failure, true);
                previousError = client.InitError();
            }
            nextRetry = now + gConfig.trackingRetryMs;
        }
        diagnostic::HeadPose pose;
        if (connected)
        {
            connected = client.Read(pose);
            if (!connected)
            {
                Event failure = Record(Kind::Message);
                failure.code = static_cast<DWORD>(client.InitError());
                sprintf_s(failure.text,"VDXR_session_disconnected: %.140s",client.ErrorDescription());
                Push(failure,true);
                nextRetry = now + gConfig.trackingRetryMs;
            }
        }
        else { LARGE_INTEGER qpc; QueryPerformanceCounter(&qpc); pose.qpc = qpc.QuadPart; }
        pose.session=session;
        PublishHeadPose(pose);
        if (now >= nextLog) { CaptureHeadPose("worker"); nextLog = now + gConfig.trackingSampleMs; }
        Sleep(gConfig.trackingPollMs);
    }
    return 0;
}

void CaptureCameraMode(Event& event)
{
    if (gCameraModeFault.load(std::memory_order_relaxed)) return;
    __try
    {
        auto* cameraAI = gCSession::GetInstance().GetCamera_PS();
        if (!diagnostic::Readable(cameraAI, sizeof(gCCameraAI_PS))) return;
        event.firstPerson = cameraAI->IsInFirstPerson();
        event.tempFirstPerson = cameraAI->IsInTempFirstPerson();
        event.dialogCamera = cameraAI->IsDialogCameraActive();
        event.cameraAI = reinterpret_cast<uintptr_t>(cameraAI);
        event.cameraModeValid = true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        gCameraModeFault.store(true);
        Message("camera_mode_probe_fault; mode sampling disabled for this session", GetExceptionCode());
    }
}

void CaptureCamera(eCApplication* app, int hook)
{
    if (gCameraFault.load(std::memory_order_relaxed)) return;
    Event event = Record(Kind::Camera, hook, true);
    __try
    {
        auto& camera = app->GetCurrentCamera();
        if (!diagnostic::Readable(&camera, sizeof(eCCameraBase))) return;
        event.pointer = reinterpret_cast<uintptr_t>(&camera);
        auto const& position = camera.GetPosition();
        event.position[0] = position.m_fX; event.position[1] = position.m_fY; event.position[2] = position.m_fZ;
        auto const& rotation = camera.GetRotation();
        event.rotation[0] = rotation.m_fX; event.rotation[1] = rotation.m_fY; event.rotation[2] = rotation.m_fZ;
        auto const& scale = camera.GetScale();
        event.scale[0] = scale.m_fX; event.scale[1] = scale.m_fY; event.scale[2] = scale.m_fZ;
        event.fov = camera.GetFieldOfView();
        event.aspect = camera.GetAspectRatio();
        // Copy through the SDK indexing operator instead of assuming storage order.
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 4; ++column)
            {
                event.view[row * 4 + column] = camera.GetViewMatrix()(row, column);
                event.projection[row * 4 + column] = camera.GetProjectionMatrix()(row, column);
            }
        auto const& clip = camera.GetClipDepth();
        event.clip[0] = clip.GetX(); event.clip[1] = clip.GetY();
        event.gameRunning = app->IsGameRunning();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        gCameraFault.store(true);
        Message("camera_probe_fault; camera sampling disabled for this session", GetExceptionCode());
        return;
    }
    CaptureCameraMode(event);
    Push(event);
}

void CaptureDevice()
{
    if (gDeviceFault.load(std::memory_order_relaxed)) return;
    Event event = Record(Kind::Device);
    __try
    {
        auto* wrapper = DeviceAccess::sGetAPIDevice();
        if (!diagnostic::Readable(wrapper, sizeof(eCDX9Device))) return;
        auto* device = reinterpret_cast<eCDX9Device*>(wrapper)->m_pDevice;
        if (!diagnostic::Readable(device, sizeof(void*))) return;
        auto table = *reinterpret_cast<void***>(device);
        if (!diagnostic::Readable(table, 3 * sizeof(void*)) || !diagnostic::Executable(table[0])) return;
        event.pointer = reinterpret_cast<uintptr_t>(wrapper);
        event.pointer2 = reinterpret_cast<uintptr_t>(device);
        IDirect3DDevice9Ex* exDevice = nullptr;
        event.ex = SUCCEEDED(device->QueryInterface(__uuidof(IDirect3DDevice9Ex), reinterpret_cast<void**>(&exDevice)));
        if (exDevice) exDevice->Release();
        D3DDEVICE_CREATION_PARAMETERS creation{};
        HRESULT result = device->GetCreationParameters(&creation);
        if (SUCCEEDED(result))
        {
            event.adapter = creation.AdapterOrdinal;
            event.deviceType = creation.DeviceType;
            event.behavior = creation.BehaviorFlags;
        }
        event.code = static_cast<DWORD>(result);
        D3DVIEWPORT9 viewport{};
        if (SUCCEEDED(device->GetViewport(&viewport)))
        {
            event.viewport[0] = viewport.X; event.viewport[1] = viewport.Y;
            event.viewport[2] = viewport.Width; event.viewport[3] = viewport.Height;
        }
        IDirect3DSurface9* target = nullptr;
        if (SUCCEEDED(device->GetRenderTarget(0, &target)) && target)
        {
            D3DSURFACE_DESC description{};
            HRESULT descResult = target->GetDesc(&description);
            target->Release();
            if (SUCCEEDED(descResult))
            {
                event.target[0] = description.Width; event.target[1] = description.Height;
                event.target[2] = description.Format; event.target[3] = description.MultiSampleType;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        gDeviceFault.store(true);
        Message("device_probe_fault; device sampling disabled for this session", GetExceptionCode());
        return;
    }
    Push(event);
}

bool Tracing();
void ControlRecord(char const* action, diagnostic::HeadPose const* pose=nullptr,
                   eCCameraBase const* readback=nullptr, float error=0, gCCharacterControl_PS* control=nullptr, uint16_t key=0)
{
    Event event=Record(Kind::HeadControl);
    strcpy_s(event.text,action);
    event.controlEnabled=gHeadControl.enabled; event.centered=gHeadControl.centered; event.applied=gHeadControl.active;
    event.pointer=reinterpret_cast<uintptr_t>(gHeadControl.camera);
    event.counts[0]=gHeadControl.appliedCount;event.counts[1]=gHeadControl.restoredCount;
    event.counts[2]=gHeadControl.movementCalls;event.counts[3]=gHeadControl.movementRestores;
    event.actionCalls=gHeadControl.actionCalls;event.actionRestores=gHeadControl.actionRestores;event.sessionKey=key;
    memcpy(event.nativeRotation,gHeadControl.nativeRotation,sizeof(event.nativeRotation));
    memcpy(event.requestedRotation,gHeadControl.requestedRotation,sizeof(event.requestedRotation));
    memcpy(event.projection,gHeadControl.nativeView,sizeof(event.projection));
    event.centerQuaternion[0]=gHeadControl.center.w;event.centerQuaternion[1]=gHeadControl.center.x;
    event.centerQuaternion[2]=gHeadControl.center.y;event.centerQuaternion[3]=gHeadControl.center.z;
    if(pose)event.headPose=*pose;
    if(control)
    {
        auto const& direction=control->GetWishedDirection();
        event.position[0]=direction.m_fX;event.position[1]=direction.m_fY;event.position[2]=direction.m_fZ;
        event.movementSample=true;
    }
    if(readback)
    {
        event.code=1;
        auto const& r=readback->GetRotation();event.rotation[0]=r.m_fX;event.rotation[1]=r.m_fY;event.rotation[2]=r.m_fZ;
        for(int row=0;row<4;++row)for(int col=0;col<4;++col)event.view[row*4+col]=readback->GetViewMatrix()(row,col);
    }
    event.viewRotationError=error;
    Push(event);
}
void ControlState(char const* state)
{
    if(!gHeadControl.previousState || strcmp(state,gHeadControl.previousState))
    {gHeadControl.previousState=state;ControlRecord(state);}
}
diagnostic::Rotation3 CameraViewRotation(eCCameraBase const& camera)
{
    diagnostic::Rotation3 r;
    for(int row=0;row<3;++row)for(int col=0;col<3;++col)r.m[row*3+col]=camera.GetViewMatrix()(row,col);
    return r;
}
void RestoreHeadCameraUnsafe(eCApplication* app)
{
    if(!gHeadControl.active)return;
    // Never write a cached object after the game has replaced the active camera.
    auto* camera=&app->GetCurrentCamera();
    if(camera!=gHeadControl.camera || !diagnostic::Readable(camera,sizeof(eCCameraBase)))
    {
        gHeadControl.active=false;gHeadControl.enabled=false;gHeadControl.fault=true;
        ControlState("restore_camera_changed; control_disabled");return;
    }
    auto const& current=camera->GetRotation();
    float values[3]={current.m_fX,current.m_fY,current.m_fZ};
    for(int i=0;i<3;++i)if(!std::isfinite(values[i]) || std::fabs(values[i]-gHeadControl.requestedRotation[i])>.0001f)
    {
        // A game system/other mod wrote a new rotation during this frame.
        gHeadControl.active=false;gHeadControl.enabled=false;gHeadControl.fault=true;
        ControlState("restore_rotation_changed; control_disabled");return;
    }
    bCVector native(gHeadControl.nativeRotation[0],gHeadControl.nativeRotation[1],gHeadControl.nativeRotation[2]);
    camera->SetRotation(native);
    camera->UpdateViewport(GETrue); // Recalculate view and invalidate the attached gfx camera.
    bool movement=gHeadControl.movementActive, action=gHeadControl.actionActive;
    gHeadControl.active=false;
    if(action)++gHeadControl.actionRestores;else if(movement)++gHeadControl.movementRestores;else ++gHeadControl.restoredCount;
    auto baseline=diagnostic::NativeCameraRotation(gHeadControl.nativeRotation);
    float error=diagnostic::RotationError(baseline,CameraViewRotation(*camera));
    if(error>.001f)
    {gHeadControl.enabled=false;gHeadControl.fault=true;ControlRecord("restore_view_mismatch; control_disabled",nullptr,camera,error);}
    else if(Tracing() || gSample.load(std::memory_order_relaxed))ControlRecord(action?"action_restored":movement?"movement_restored":"restored",nullptr,camera,error);
    gHeadControl.movementActive=false;gHeadControl.actionActive=false;
}
void RestoreHeadCamera(eCApplication* app)
{
    __try {RestoreHeadCameraUnsafe(app);}
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        gHeadControl.active=false;gHeadControl.enabled=false;gHeadControl.fault=true;
        Message("head_camera_restore_fault; control_disabled",GetExceptionCode());
    }
}
void ApplyHeadCameraUnsafe(eCApplication* app, bool movement=false, bool action=false)
{
    if(gConfig.stereoEnabled && !movement)return; // Graphical stereo owns the render camera.
    if(!gHeadControl.enabled || !gLogging.load(std::memory_order_relaxed))return;
    if(gHeadControl.fault){ControlState("camera_control_fault");return;}
    DWORD foregroundProcess=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundProcess);
    if(foregroundProcess!=GetCurrentProcessId()){ControlState("background_suspended");return;}
    if(!app->IsGameRunning()){ControlState("game_not_running");return;}
    if(diagnostic::NativeAnimalForm(gCSession::GetInstance().GetPlayer())){ControlState("animal_native_camera");return;}
    Event mode;CaptureCameraMode(mode);
    if(!mode.cameraModeValid){ControlState("camera_mode_unavailable");return;}
    if(mode.dialogCamera){ControlState("dialog_suspended");return;}
    if(!mode.firstPerson || mode.tempFirstPerson){ControlState("native_first_person_required");return;}
    diagnostic::HeadPose pose;
    if(!TryAcquireSRWLockShared(&gHeadPoseLock)){ControlState("pose_cache_busy");return;}
    pose=gHeadPose;ReleaseSRWLockShared(&gHeadPoseLock);
    LARGE_INTEGER now;QueryPerformanceCounter(&now);
    if(!diagnostic::FreshOrientation(pose,now.QuadPart,gFrequency.QuadPart,(std::max)(gConfig.maxPoseAgeMs,DWORD(movement?100:0))))
    {
        if(gHeadControl.centered&&!gConfig.stereoEnabled){gHeadControl.centered=false;gHeadControl.recenterPending=false;}
        ControlState(gHeadControl.recenterPending?"waiting_tracking_for_recenter":"tracking_lost; press_recenter");return;
    }
    if(gHeadControl.centered && (gHeadControl.centerSession!=pose.session || strcmp(gHeadControl.origin,pose.origin)))
    {gHeadControl.centered=false;gHeadControl.recenterPending=false;ControlState("tracking_origin_changed; press_recenter");return;}
    diagnostic::Orientation head{pose.quaternion[0],pose.quaternion[1],pose.quaternion[2],pose.quaternion[3]};
    if(gHeadControl.recenterPending)
    {
        if(!diagnostic::RecenterTracking(head,pose.position,false,gHeadControl.center,gStereo.centerPosition,diagnostic::RecenterPhase::LegacyCamera,gConfig.stereoEnabled))
        {ControlState("recenter_waiting_stereo_frame");return;}
        strcpy_s(gHeadControl.origin,pose.origin);
        gHeadControl.centerSession=pose.session;
        gHeadControl.centered=true;gHeadControl.recenterPending=false;
        gHeadControl.movementHeadingValid=false;
        ControlRecord("recentered",&pose);
    }
    if(!gHeadControl.centered){ControlState("recenter_required");return;}
    auto* camera=&app->GetCurrentCamera();
    if(!diagnostic::Readable(camera,sizeof(eCCameraBase)) || !camera->GetPerspective())
    {ControlState("perspective_camera_unavailable");return;}
    auto const& scale=camera->GetScale();
    if(std::fabs(scale.m_fX-1)>.0001f || std::fabs(scale.m_fY-1)>.0001f || std::fabs(scale.m_fZ-1)>.0001f)
    {ControlState("non_unit_camera_scale");return;}
    auto const& rotation=camera->GetRotation();
    gHeadControl.nativeRotation[0]=rotation.m_fX;gHeadControl.nativeRotation[1]=rotation.m_fY;gHeadControl.nativeRotation[2]=rotation.m_fZ;
    auto baseline=CameraViewRotation(*camera);
    if(diagnostic::RotationError(baseline,diagnostic::NativeCameraRotation(gHeadControl.nativeRotation))>.001f)
    {ControlState("native_camera_convention_mismatch");return;}
    diagnostic::Rotation3 relative;
    if(!diagnostic::RelativeHeadRotation(gHeadControl.center,head,relative))return;
    if(gConfig.stereoEnabled&&gVrSettings.stableHorizon)baseline=diagnostic::UprightCamera(baseline);
    gHeadControl.expected=diagnostic::Product(baseline,relative);
    if(movement)
    {
        float heading=gHeadControl.movementHeadingValid?gHeadControl.movementHeading:gHeadControl.nativeRotation[1];
        if(diagnostic::HorizontalHeading(gHeadControl.expected,heading))
        {gHeadControl.movementHeading=heading;gHeadControl.movementHeadingValid=true;}
        gHeadControl.requestedRotation[0]=0;gHeadControl.requestedRotation[1]=heading;gHeadControl.requestedRotation[2]=0;
        gHeadControl.expected=diagnostic::NativeCameraRotation(gHeadControl.requestedRotation);
    }
    else diagnostic::NativeCameraEuler(gHeadControl.expected,gHeadControl.requestedRotation);
    gHeadControl.camera=camera;
    for(int row=0;row<4;++row)for(int col=0;col<4;++col)gHeadControl.nativeView[row*4+col]=camera->GetViewMatrix()(row,col);
    bCVector requested(gHeadControl.requestedRotation[0],gHeadControl.requestedRotation[1],gHeadControl.requestedRotation[2]);
    gHeadControl.active=true; // Enable rollback before the first write.
    gHeadControl.movementActive=movement;gHeadControl.actionActive=action;gHeadControl.appliedPose=pose;
    camera->SetRotation(requested);
    camera->UpdateViewport(GETrue);
    float error=diagnostic::RotationError(gHeadControl.expected,CameraViewRotation(*camera));
    if(error>.001f)
    {
        ControlRecord("apply_view_mismatch",&pose,camera,error);
        RestoreHeadCameraUnsafe(app);gHeadControl.enabled=false;gHeadControl.fault=true;return;
    }
    if(action)++gHeadControl.actionCalls;else if(movement)++gHeadControl.movementCalls;else ++gHeadControl.appliedCount;
    ControlState("applying");
    if(Tracing() || gSample.load(std::memory_order_relaxed))ControlRecord(action?"action_applied":movement?"movement_applied":"applied",&pose,camera,error);
}
void ApplyHeadCamera(eCApplication* app)
{
    __try {ApplyHeadCameraUnsafe(app);}
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        gHeadControl.enabled=false;gHeadControl.fault=true;
        Message("head_camera_apply_fault; control_disabled",GetExceptionCode());
    }
    if(gHeadControl.fault)RestoreHeadCamera(app);
}
void RefreshHeroVoice(gCScriptProcessingUnit* unit,bool enabled){
    if(!gVoiceReady||gVoiceFault||GetCurrentThreadId()!=gApplicationThread.load()||!gApplication)return;
    if(!enabled&&gHeroVoice.Empty())return;
    if(enabled&&gHeroVoice.Empty()&&(!gHeadControl.enabled||!gConfig.stereoEnabled||!gStereoRuntime.Connected()||gStereo.shuttingDown||gStereo.fault))return;
    __try {
        if(!gApplication->IsGameRunning()||(enabled&&gWorldLifecycle.Blocked()))return;
        auto* player=gCSession::GetInstance().GetPlayer();
        if(!diagnostic::Readable(player,sizeof(eCDynamicEntity))||!diagnostic::Readable(unit,sizeof(*unit))||unit->GetSelfEntity()!=player)return;
        static_assert(offsetof(gCScriptProcessingUnit,m_pAudioChannel)==0x6c,"Verified CP/PUP speech channel offset");
        auto* channel=static_cast<eCAudioChannel*>(unit->m_pAudioChannel);
        uintptr_t handle=diagnostic::Readable(channel,sizeof(*channel))&&channel->IsPlaying()?reinterpret_cast<uintptr_t>(channel->GetHandle()):0;
        bool active=enabled&&gHeadControl.enabled&&gConfig.stereoEnabled&&gStereoRuntime.Connected()&&!gStereo.shuttingDown&&!gStereo.fault;
        int result=gHeroVoice.Update(reinterpret_cast<uintptr_t>(unit),handle,active,gVoiceApi);
        if(result==1)Message("vr_hero_voice_centered; current_speech_channel_only; NPC_spatial_audio_preserved",0,true);
        else if(result<0){gVoiceFault=true;Message("vr_hero_voice_api_refused; native_audio_preserved",0,true);}
    }__except(EXCEPTION_EXECUTE_HANDLER){gHeroVoice.Forget();gVoiceFault=true;Message("vr_hero_voice_fault; native_audio_preserved",GetExceptionCode(),true);}
}
void RestoreCurrentHeroVoice(){
    if(gHeroVoice.Empty())return;
    __try {if(gApplication&&gApplication->IsGameRunning()){
        auto* player=gCSession::GetInstance().GetPlayer();
        if(diagnostic::Readable(player,sizeof(eCDynamicEntity))){auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
            if(diagnostic::Readable(routine,sizeof(*routine)))RefreshHeroVoice(&routine->GetSPU(),false);}
    }}__except(EXCEPTION_EXECUTE_HANDLER){Message("vr_hero_voice_release_refused; borrowed_channel_forgotten",GetExceptionCode(),true);}
    gHeroVoice.Forget();
}
GEBool __fastcall StartHeroOutput(gCScriptProcessingUnit* unit,void*,bCString const& text){
    // Restore the previous live channel before native playback can reuse it.
    // A new line may reuse the same FMOD address, so it needs a fresh lease.
    RefreshHeroVoice(unit,false);
    GEBool result=reinterpret_cast<GEBool(__thiscall*)(gCScriptProcessingUnit*,bCString const&)>(gVoiceStartHook.original)(unit,text);
    if(result)RefreshHeroVoice(unit,true);return result;
}
void __fastcall UpdateHeroOutput(gCScriptProcessingUnit* unit,void*){
    reinterpret_cast<void(__thiscall*)(gCScriptProcessingUnit*)>(gVoiceUpdateHook.original)(unit);
    if(gHeroVoice.Tracks(reinterpret_cast<uintptr_t>(unit)))RefreshHeroVoice(unit,true);
}
bool InstallHeroVoiceHooks(HMODULE game){
    if(!gVoiceApi.Resolve())return false;
    auto start=GetProcAddress(game,"?StartOutput@gCScriptProcessingUnit@@QAE_NABVbCString@@@Z");
    auto update=GetProcAddress(game,"?UpdateChannel@gCScriptProcessingUnit@@IAEXXZ");
    if(reinterpret_cast<uintptr_t>(start)!=reinterpret_cast<uintptr_t>(game)+diagnostic::VoiceStartRva||reinterpret_cast<uintptr_t>(update)!=reinterpret_cast<uintptr_t>(game)+diagnostic::VoiceUpdateRva||
       !diagnostic::Readable(start,sizeof(diagnostic::VoiceStartPrefix))||memcmp(start,diagnostic::VoiceStartPrefix,sizeof(diagnostic::VoiceStartPrefix))||
       !diagnostic::Readable(update,sizeof(diagnostic::VoiceUpdatePrefix))||memcmp(update,diagnostic::VoiceUpdatePrefix,sizeof(diagnostic::VoiceUpdatePrefix)))return false;
    gVoiceReady=gVoiceStartHook.Install(start,reinterpret_cast<void*>(&StartHeroOutput),diagnostic::VoiceStartPrefix,sizeof(diagnostic::VoiceStartPrefix))&&
        gVoiceUpdateHook.Install(update,reinterpret_cast<void*>(&UpdateHeroOutput),diagnostic::VoiceUpdatePrefix,sizeof(diagnostic::VoiceUpdatePrefix));
    return gVoiceReady;
}
bool NativeControllerOwns(gCCharacterControl_PS* control){
    if(gWorldLifecycle.Blocked()||gStereo.shuttingDown||!gApplication||!gApplication->IsGameRunning()||gCSession::GetInstance().IsPaused())return false;
    auto* player=gCSession::GetInstance().GetPlayer();return diagnostic::Readable(player,sizeof(eCDynamicEntity))&&
        player->GetPropertySet(eEPropertySetType_CharacterControl)==control&&gControllerModes.Owns(reinterpret_cast<uintptr_t>(player),GetTickCount64(),gProcess.load(std::memory_order_relaxed));
}
void BeginControllerReference(gCCharacterControl_PS* self,diagnostic::ControllerReferenceLease& reference){
    if(GetCurrentThreadId()!=gApplicationThread.load()||!gConfig.headDirectedMovement||!NativeControllerOwns(self))return;
    auto* player=gCSession::GetInstance().GetPlayer();
    bool owns=gNativeMovement.OwnsPoll(reinterpret_cast<uintptr_t>(player),GetTickCount64(),1,gProcess.load(std::memory_order_relaxed));
    if(reference.Begin(owns,int(self->GetControlFrameOfReference().GetValue())))self->AccessControlFrameOfReference().SetValue(gECharacterControlFOR_Camera);
}
void EndControllerReference(gCCharacterControl_PS* self,diagnostic::ControllerReferenceLease& reference){
    if(!reference.Active())return;
    int restore=0;if(reference.End(int(self->GetControlFrameOfReference().GetValue()),restore))
        self->AccessControlFrameOfReference().SetValue(static_cast<gECharacterControlFOR>(restore));
}
void RecoverIdleCombatMovement(gCCharacterControl_PS* control){
    // This hook also runs for NPCs. Their calls must not reset the hero's
    // continuous observation window or operate on the hero's movement state.
    if(gApplication&&gApplication->IsGameRunning()){
        auto* current=gCSession::GetInstance().GetPlayer();
        if(diagnostic::Readable(current,sizeof(eCDynamicEntity))&&current->GetPropertySet(eEPropertySetType_CharacterControl)!=control)return;
    }
    if(!gMelee.ready||gMelee.fault||gWorldLifecycle.Blocked()||gStereo.shuttingDown||gStereo.fault||!gConfig.stereoEnabled||
       !gHeadControl.enabled||!gVrSettings.controllers||!gVrSettings.trackedArms||!gStereoRuntime.Connected()||
       !gApplication||!gApplication->IsGameRunning()||gCinemaScope.Active(GetCurrentThreadId())){gCombatMovementRecovery.Reset();return;}
    auto& session=gCSession::GetInstance();auto* player=session.GetPlayer();DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    if(session.IsPaused()||foreground!=GetCurrentProcessId()||MenuPanelActive()||gRadialOpen||!gStereo.firstPerson||
       !diagnostic::Readable(player,sizeof(eCDynamicEntity))||player->GetPropertySet(eEPropertySetType_CharacterControl)!=control||
       CameraThirdPersonActivity(player)||diagnostic::NativeRecovery(player)||diagnostic::NativeBlocking(player)||
       diagnostic::NativeAnimalForm(player)||gMagic.Selected(player)||gBow.Weapon(player)){gCombatMovementRecovery.Reset();return;}
    auto* movement=static_cast<gCCharacterMovement_PS*>(player->GetPropertySet(eEPropertySetType_CharacterMovement));
    auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
    if(!diagnostic::Readable(movement,sizeof(*movement))||movement->IsMounted()){gCombatMovementRecovery.Reset();return;}
    auto repair=gCombatMovementRecovery.Update(player,movement,routine,true,gMelee.Weapon(player)!=nullptr,GetTickCount64());
    if(repair!=diagnostic::CombatMoveRepair::None){Event event=Record(Kind::Message);
        sprintf_s(event.text,"vr_idle_combat_move_released reason=%s enabled=%d combat=%d; native_cleanup; position_and_input_preserved",
            repair==diagnostic::CombatMoveRepair::Orphan?"orphan_instruction":"physical_attack_animation",movement->IsMovementEnabledFromSPU(),movement->IsCombatMovementEnabledFromSPU());Push(event,true);}
}
void ApplyNativeControllerMode(gCCharacterControl_PS* control){
    if(!NativeControllerOwns(control))return;
    auto* player=gCSession::GetInstance().GetPlayer();int nativeWish=-1;
    auto* movement=static_cast<gCCharacterMovement_PS*>(player->GetPropertySet(eEPropertySetType_CharacterMovement));
    if(!diagnostic::Readable(movement,sizeof(*movement))||!movement->IsMovementEnabledFromSPU()||movement->IsCombatMovementEnabledFromSPU()||movement->IsMounted()||
       diagnostic::NativeRecovery(player)||diagnostic::NativeActivity(player)||diagnostic::NativeAnimalForm(player))return;
    // Exact verified Game.dll SetWishedMovementMode writes this SDK member.
    if(diagnostic::Readable(control,sizeof(*control)))memcpy(&nativeWish,reinterpret_cast<unsigned char const*>(control)+0xac,sizeof(nativeWish));
    int mode=gControllerModes.Request(reinterpret_cast<uintptr_t>(player),GetTickCount64(),nativeWish,int(movement->GetMovementMode()),gProcess.load(std::memory_order_relaxed));
    if(mode&&!diagnostic::NativeSneaking(player))control->SetWishedMovementMode(static_cast<gECharMovementMode>(mode));
}
bool BeginMovementSteering(gCCharacterControl_PS* self, bool action=false)
{
    __try
    {
        if(!gConfig.headDirectedMovement || !gHeadControl.enabled || gHeadControl.active || !gLogging.load())return false;
        auto& session=gCSession::GetInstance();
        if(session.IsPaused())return false;
        auto* player=session.GetPlayer();
        if(gCameraReturn.Blocks(reinterpret_cast<uintptr_t>(player))||!diagnostic::Readable(player,sizeof(eCDynamicEntity)) ||
           player->GetPropertySet(eEPropertySetType_CharacterControl)!=self ||
           !diagnostic::Readable(self,sizeof(gCCharacterControl_PS)))return false;
        static bool targetFrameReported=false;
        if(self->GetControlFrameOfReference().GetValue()!=gECharacterControlFOR_Camera)
        {
            if(!targetFrameReported){Message("movement_target_reference; head_steering_suspended");targetFrameReported=true;}
            return false;
        }
        targetFrameReported=false;
        auto* cameraAI=session.GetCamera_PS();
        if(!diagnostic::Readable(cameraAI,sizeof(gCCameraAI_PS)) || cameraAI->IsLookingBack())return false;
        ApplyHeadCameraUnsafe(gApplication,true,action);
        return gHeadControl.active && gHeadControl.movementActive;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        gHeadControl.enabled=false;gHeadControl.fault=true;
        Message("movement_camera_apply_fault; control_disabled",GetExceptionCode());
    }
    RestoreHeadCamera(gApplication);return false;
}
void InspectMovementDirection(gCCharacterControl_PS* self)
{
        auto* player=gCSession::GetInstance().GetPlayer();auto mask=gNativeMovement.DirectionMask(reinterpret_cast<uintptr_t>(player),GetTickCount64(),gProcess.load(std::memory_order_relaxed));
        auto* movement=diagnostic::Readable(player,sizeof(eCDynamicEntity))?static_cast<gCCharacterMovement_PS*>(player->GetPropertySet(eEPropertySetType_CharacterMovement)):nullptr;
        if(mask&&diagnostic::Readable(movement,sizeof(*movement))&&gHeadControl.camera){
            auto const& view=gHeadControl.camera->GetViewMatrix();float forward=float(bool(mask&1))-float(bool(mask&2)),right=float(bool(mask&8))-float(bool(mask&4));
            float x=view(0,2)*forward+view(0,0)*right,z=view(2,2)*forward+view(2,0)*right;
            auto goal=movement->GetGoalPosition(),position=player->GetWorldPosition();float gx=goal.m_fX-position.m_fX,gz=goal.m_fZ-position.m_fZ,length=std::hypot(x,z)*std::hypot(gx,gz);
            if(length>1.f){float dot=(x*gx+z*gz)/length;if(dot<.25f){Event event=Record(Kind::Message);
                std::snprintf(event.text,sizeof(event.text),"vr_direction_phase mask=%u for=%d dot=%.3f input=%.3f,%.3f goal=%.1f,%.1f mode=%d combat=%d; old_consumed_goal; observe_only",mask,int(self->GetControlFrameOfReference().GetValue()),dot,x,z,gx,gz,int(movement->GetMovementMode()),movement->IsCombatMovementEnabledFromSPU());Push(event,true);}}
        }
}
void CaptureMovementDirection(gCCharacterControl_PS* self)
{
    if(!Tracing() && !gSample.load(std::memory_order_relaxed))return;
    __try {
        ControlRecord("movement_direction",&gHeadControl.appliedPose,gHeadControl.camera,0,self);
        InspectMovementDirection(self);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {Message("movement_direction_probe_fault",GetExceptionCode());}
}
void RestoreBodyFacing(gCCharacterMovement_PS* movement){
    if(gBodyFacingLease.Empty()||!gApplication||!gApplication->IsGameRunning())return;
    auto* player=gCSession::GetInstance().GetPlayer();auto actor=reinterpret_cast<uintptr_t>(player);
    if(!gBodyFacingLease.Matches(actor,reinterpret_cast<uintptr_t>(movement)))return;
    if(!diagnostic::Readable(player,sizeof(eCDynamicEntity))||player->GetPropertySet(eEPropertySetType_CharacterMovement)!=movement||!diagnostic::Readable(movement,sizeof(*movement))){gBodyFacingLease.Forget();return;}
    auto current=movement->GetAlignmentLookAt();float actual[3]{current.m_fX,current.m_fY,current.m_fZ},restore[3]{};
    if(gBodyFacingLease.Take(actor,reinterpret_cast<uintptr_t>(movement),movement->GetAlignmentTarget()!=nullptr,actual,restore))
        movement->SetAlignmentLookAt(bCVector(restore[0],restore[1],restore[2]));
    if(gBodyFacingLease.Empty())gBodyFacingControl=0;
}
void RestoreCurrentBodyFacing(){
    if(gBodyFacingLease.Empty()){gBodyFacingControl=0;return;}
    __try {if(gApplication&&gApplication->IsGameRunning()){
        auto* player=gCSession::GetInstance().GetPlayer();if(diagnostic::Readable(player,sizeof(eCDynamicEntity)))
            RestoreBodyFacing(static_cast<gCCharacterMovement_PS*>(player->GetPropertySet(eEPropertySetType_CharacterMovement)));
    }}__except(EXCEPTION_EXECUTE_HANDLER){Message("vr_head_body_alignment_release_refused",GetExceptionCode(),true);}
    gBodyFacingLease.Forget();gBodyFacingControl=0;
}
void ApplyHeadBodyFacing(gCCharacterControl_PS* control){
    if(gBodyFacingFault||gWorldLifecycle.Blocked()||gStereo.shuttingDown||gStereo.fault||!gConfig.stereoEnabled||!gHeadControl.enabled||!gHeadControl.centered||gHeadControl.recenterPending||
       !gStereoRuntime.Connected()||!gApplication||!gApplication->IsGameRunning()||gCinemaScope.Active(GetCurrentThreadId()))return;
    auto& session=gCSession::GetInstance();auto* player=session.GetPlayer();auto* camera=session.GetCamera_PS();
    if(!diagnostic::Readable(player,sizeof(eCDynamicEntity))||player->GetPropertySet(eEPropertySetType_CharacterControl)!=control)return;
    if(gCameraReturn.Blocks(reinterpret_cast<uintptr_t>(player))||session.IsPaused()||MenuPanelActive()||gRadialOpen||
       !diagnostic::Readable(camera,sizeof(*camera))||!camera->IsInFirstPerson()||camera->IsDialogCameraActive()||
       CameraThirdPersonActivity(player)||diagnostic::NativeRecovery(player)||diagnostic::NativeAnimalForm(player)||control->GetControlFrameOfReference().GetValue()!=gECharacterControlFOR_Camera)return;
    auto* movement=static_cast<gCCharacterMovement_PS*>(player->GetPropertySet(eEPropertySetType_CharacterMovement));
    if(!diagnostic::Readable(movement,sizeof(*movement))||!diagnostic::RoomWalkingMode(int(movement->GetMovementMode()))||movement->IsMounted()||!movement->IsMovementEnabledFromSPU()||movement->IsCombatRotationEnabledFromSPU()||movement->GetAlignmentTarget())return;
    DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);if(foreground!=GetCurrentProcessId())return;
    diagnostic::HeadPose head;if(!TryAcquireSRWLockShared(&gHeadPoseLock))return;head=gHeadPose;ReleaseSRWLockShared(&gHeadPoseLock);
    if(head.session!=gHeadControl.centerSession)return;
    LARGE_INTEGER now;QueryPerformanceCounter(&now);auto const& angles=gApplication->GetCurrentCamera().GetRotation();float euler[3]{angles.m_fX,angles.m_fY,angles.m_fZ};
    if(gHeadControl.active)memcpy(euler,gHeadControl.nativeRotation,sizeof(euler));diagnostic::Rotation3 body;
    auto wished=control->GetWishedDirection();
    bool controllerMoving=gNativeMovement.DirectionMask(reinterpret_cast<uintptr_t>(player),GetTickCount64(),gProcess.load(std::memory_order_relaxed))!=0;
    if(!diagnostic::HeadMayAlignNativeBody(controllerMoving,wished.m_fX,wished.m_fZ))return;
    bool locomotion=false;
    if(!gHeadBodyHeading.Step(reinterpret_cast<uintptr_t>(player),diagnostic::NativeCameraRotation(euler),gHeadControl.center,head,now.QuadPart,gFrequency.QuadPart,body,locomotion))return;
    auto before=movement->GetAlignmentLookAt();float previous[3]{before.m_fX,before.m_fY,before.m_fZ},desired[3]{body.m[2],0,body.m[8]};
    gBodyFacingLease.Begin(reinterpret_cast<uintptr_t>(player),reinterpret_cast<uintptr_t>(movement),previous,desired);
    gBodyFacingControl=reinterpret_cast<uintptr_t>(control);
    // Same native direction setter used by ProcessMovements. No entity/world
    // transform, translation, velocity, capsule or camera rotation is written.
    movement->SetAlignmentLookAt(bCVector(desired[0],0,desired[2]));
    if(gSample.load()){Event event=Record(Kind::Message);float yaw=0;diagnostic::HorizontalHeading(body,yaw);sprintf_s(event.text,"vr_head_body_heading yaw_deg=%.1f free_look_deg=%.0f moving=%d; native_alignment; tracking_base_unchanged",yaw*57.2957795f,diagnostic::HeadBodyHeading::FreeLookDegrees,locomotion);Push(event,true);}
}
void DispatchNativeControllerActions(gCCharacterControl_PS* self){
    if(GetCurrentThreadId()!=gApplicationThread.load()||gNativeMovementFault||!gActionHook.installed||!gApplication||!gApplication->IsGameRunning()||gWorldLifecycle.Blocked())return;
    auto* player=gCSession::GetInstance().GetPlayer();
    if(!diagnostic::Readable(player,sizeof(eCDynamicEntity))||player->GetPropertySet(eEPropertySetType_CharacterControl)!=self)return;
    DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    bool allowed=foreground==GetCurrentProcessId()&&gVrSettings.controllers&&gHeadControl.enabled&&gConfig.stereoEnabled&&!gStereo.shuttingDown&&!gStereo.fault&&
        !gCameraReturn.Blocks(reinterpret_cast<uintptr_t>(player))&&!gCSession::GetInstance().IsPaused()&&!MenuPanelActive()&&!gRadialOpen&&!diagnostic::NativeActivity(player)&&!diagnostic::NativeDirectionalActivity(player)&&!diagnostic::NativeRecovery(player)&&!gFinisher.Busy(player);
    // A late headset sample must not silently swap a first-person controller
    // step to the desktop camera's (possibly opposite) heading for one tick.
    if(gStereo.firstPerson&&gConfig.headDirectedMovement&&!(gHeadControl.active&&gHeadControl.movementActive))allowed=false;
    auto now=GetTickCount64();auto update=gProcess.load(std::memory_order_relaxed);auto actor=reinterpret_cast<uintptr_t>(player);
    bool owns=gNativeMovement.NeedsDirectionReset(actor,now,update);
    if(owns)diagnostic::ClearControllerDirectionFrame(self,sizeof(*self));
    gNativeMovement.Dispatch(actor,now,allowed,[self](diagnostic::NativeMovementAction const& message){
        reinterpret_cast<int(__thiscall*)(gCCharacterControl_PS*,eCInpShared::eSGameKeyMessage const&)>(gActionHook.original)(self,*reinterpret_cast<eCInpShared::eSGameKeyMessage const*>(&message));
    },update);
}
void ReleaseNativeControllerActions(){
    // A paused menu or VR shutdown may skip ProcessMovements. Release Sprint
    // while the verified live actor still exists; never retain a native hold.
    __try {if(!gNativeMovementFault&&gActionHook.installed&&gApplication&&gApplication->IsGameRunning()){
        auto* player=gCSession::GetInstance().GetPlayer();
        if(diagnostic::Readable(player,sizeof(eCDynamicEntity))){
            auto* control=static_cast<gCCharacterControl_PS*>(player->GetPropertySet(eEPropertySetType_CharacterControl));
            if(diagnostic::Readable(control,sizeof(*control)))gNativeMovement.Dispatch(reinterpret_cast<uintptr_t>(player),GetTickCount64(),false,[control](diagnostic::NativeMovementAction const& message){
                reinterpret_cast<int(__thiscall*)(gCCharacterControl_PS*,eCInpShared::eSGameKeyMessage const&)>(gActionHook.original)(control,*reinterpret_cast<eCInpShared::eSGameKeyMessage const*>(&message));
            });
        }
    }}__except(EXCEPTION_EXECUTE_HANDLER){gNativeMovementFault=true;Message("vr_native_movement_release_refused",GetExceptionCode(),true);}
    gNativeMovement.Reset();
}
// A bounded, explicitly armed observer. It never writes movement state, polls
// files in gameplay, enables general profilers, or reads renderer resources.
void TraceControllerMovement(gCCharacterControl_PS* control,char const* stage){
    if(GetCurrentThreadId()!=gApplicationThread.load()||!gMovementTrace.Selected(gProcess.load(std::memory_order_relaxed))||
       gWorldLifecycle.Blocked()||!gApplication||!gApplication->IsGameRunning())return;
    auto* player=gCSession::GetInstance().GetPlayer();
    if(!diagnostic::Readable(player,sizeof(eCDynamicEntity))||player->GetPropertySet(eEPropertySetType_CharacterControl)!=control)return;
    auto* movement=static_cast<gCCharacterMovement_PS*>(player->GetPropertySet(eEPropertySetType_CharacterMovement));
    if(!diagnostic::Readable(control,sizeof(*control))||!diagnostic::Readable(movement,sizeof(*movement)))return;
    Event event=Record(Kind::NativeMovementTrace);strcpy_s(event.text,stage);
    auto position=player->GetWorldPosition(),wish=control->GetWishedDirection(),alignment=movement->GetAlignmentLookAt();
    auto const& actor=player->GetWorldMatrix();
    event.position[0]=position.m_fX;event.position[1]=position.m_fY;event.position[2]=position.m_fZ;
    event.rotation[0]=wish.m_fX;event.rotation[1]=wish.m_fY;event.rotation[2]=wish.m_fZ;
    event.scale[0]=alignment.m_fX;event.scale[1]=alignment.m_fY;event.scale[2]=alignment.m_fZ;
    event.nativeRotation[0]=actor(2,0);event.nativeRotation[1]=actor(2,1);event.nativeRotation[2]=actor(2,2);
    auto const* cb=reinterpret_cast<unsigned char const*>(control);
    auto const* mb=reinterpret_cast<unsigned char const*>(movement);
    // Verified CP SetGoalPose writes +2dc BEFORE passing the request to its
    // sensor. GetGoalPosition/GetLatestGoalPosition both expose an older +2c0
    // result and cannot identify what this input call actually requested.
    float request[3];memcpy(request,mb+0x2dc,sizeof(request));
    for(int i=0;i<3;++i)event.requestedRotation[i]=request[i]-event.position[i];
    memcpy(event.view,mb+0x134,3*sizeof(float)); // native world velocity, observation only
    memcpy(event.view+3,mb+0x360,3*sizeof(float)); // current native translation target
    memcpy(event.viewport,cb+0x3c,sizeof(event.viewport)); // F,L,R,B timers before/after consumption
    memcpy(event.target,cb+0xac,sizeof(uint32_t));event.target[1]=int(movement->GetMovementMode());
    memcpy(event.target+2,cb+0x18,sizeof(uint32_t));
    event.target[3]=(movement->IsMovementEnabledFromSPU()?1u:0u)|(movement->IsCombatMovementEnabledFromSPU()?2u:0u)|
        (movement->IsCombatRotationEnabledFromSPU()?4u:0u)|(movement->GetAlignmentTarget()?8u:0u);
    event.sessionKey=gNativeMovement.DirectionMask(reinterpret_cast<uintptr_t>(player),GetTickCount64(),gProcess.load(std::memory_order_relaxed));
    event.fov=movement->GetCurrentVelocity();event.aspect=movement->GetCurrentSpeedModifier();
    event.applied=gHeadControl.active&&gHeadControl.movementActive;
    event.ex=gMovementTraceInput.frame==gProcess.load(std::memory_order_relaxed);
    if(event.ex){event.view[6]=gMovementTraceInput.x;event.view[7]=gMovementTraceInput.z;event.view[8]=gMovementTraceInput.sprintSelected?1.f:0.f;event.view[9]=gMovementTraceInput.sprintPressed?1.f:0.f;}
    auto* camera=&gApplication->GetCurrentCamera();
    if(diagnostic::Readable(camera,sizeof(*camera))){auto const& view=camera->GetViewMatrix();for(int r=0;r<4;++r)for(int c=0;c<4;++c)event.projection[r*4+c]=view(r,c);event.cameraModeValid=true;}
    auto* cameraAI=gCSession::GetInstance().GetCamera_PS();
    if(diagnostic::Readable(cameraAI,sizeof(*cameraAI)))event.tempFirstPerson=cameraAI->IsLookingBack();
    Push(event,true);
}
void TraceControllerPhysics(gCCharacterMovement_PS* movement,char const* stage){
    if(GetCurrentThreadId()!=gApplicationThread.load()||!gMovementTrace.Selected(gProcess.load(std::memory_order_relaxed))||
       gWorldLifecycle.Blocked()||!gApplication||!gApplication->IsGameRunning())return;
    auto* player=gCSession::GetInstance().GetPlayer();
    if(!diagnostic::Readable(player,sizeof(eCDynamicEntity))||player->GetPropertySet(eEPropertySetType_CharacterMovement)!=movement)return;
    TraceControllerMovement(static_cast<gCCharacterControl_PS*>(player->GetPropertySet(eEPropertySetType_CharacterControl)),stage);
}
void __cdecl MovementOriginal(void* context)
{
    auto* self=static_cast<gCCharacterControl_PS*>(context);
    __try{DispatchNativeControllerActions(self);}__except(EXCEPTION_EXECUTE_HANDLER){gNativeMovement.Reset();gNativeMovementFault=true;Message("vr_native_movement_actions_refused; controller_keys_disabled",GetExceptionCode(),true);}
    if(GetCurrentThreadId()==gApplicationThread.load())ApplyNativeControllerMode(self);
    // A native OnAction script can reselect Target after the outer camera
    // scope began. Recheck at actual consumption, then restore its selection.
    diagnostic::ControllerReferenceLease reference;
    if(gHeadControl.active&&gHeadControl.movementActive)BeginControllerReference(self,reference);
    __try {
        if(gMovementTrace.Armed()&&GetCurrentThreadId()==gApplicationThread.load())__try{
            if(NativeControllerOwns(self))gMovementTrace.Select(gProcess.load(std::memory_order_relaxed),GetTickCount64());
            TraceControllerMovement(self,"before_native_input");
        }__except(EXCEPTION_EXECUTE_HANDLER){gMovementTrace.Reset();Message("vr_movement_trace_refused; native_movement_unchanged",GetExceptionCode(),true);}
        reinterpret_cast<void(__thiscall*)(gCCharacterControl_PS*)>(gMovementHook.original)(self);
        if(gMovementTrace.Armed())__try{TraceControllerMovement(self,"after_native_input");}
            __except(EXCEPTION_EXECUTE_HANDLER){gMovementTrace.Reset();}
        if(GetCurrentThreadId()==gApplicationThread.load()&&gHeadControl.active&&gHeadControl.movementActive){
            // GetGoalPosition is not this update's fresh sensor request.
            // Keep the native goal; reading an older result cannot justify a rewrite.
            CaptureMovementDirection(self);
        }}
    __finally {EndControllerReference(self,reference);}
}
void __cdecl MovementCleanup(void*) {RestoreHeadCamera(gApplication);}
void __fastcall ProcessPlayerMovements(gCCharacterControl_PS* self, void*)
{
    if(GetCurrentThreadId()==gApplicationThread.load()&&gBodyFacingControl==reinterpret_cast<uintptr_t>(self))RestoreCurrentBodyFacing();
    if(GetCurrentThreadId()==gApplicationThread.load()){
        __try{RecoverIdleCombatMovement(self);}__except(EXCEPTION_EXECUTE_HANDLER){gCombatMovementRecovery.Reset();Message("vr_idle_combat_recovery_refused; native_state_preserved",GetExceptionCode(),true);}
    }
    diagnostic::ControllerReferenceLease reference;
    BeginControllerReference(self,reference);
    __try {
        if(GetCurrentThreadId()==gApplicationThread.load() && BeginMovementSteering(self))
            diagnostic::CallWithRestoration(&MovementOriginal,&MovementCleanup,self);
        else MovementOriginal(self);
    } __finally {
        EndControllerReference(self,reference);
    }
    if(GetCurrentThreadId()==gApplicationThread.load()){
        __try{ApplyHeadBodyFacing(self);}__except(EXCEPTION_EXECUTE_HANDLER){RestoreCurrentBodyFacing();gBodyFacingFault=true;Message("vr_head_body_heading_refused; native_facing_preserved",GetExceptionCode(),true);}
    }
}
// Native OnAction authors the persistent wished vector BEFORE ProcessMovements.
// Keep its camera override active until the original handler has consumed it;
// preserve the reference argument, integer return value and game exceptions.
struct ActionContext
{
    gCCharacterControl_PS* self;
    eCInpShared::eSGameKeyMessage const* message;
    uint16_t key;
    int result;
};
using ActionCallback=int(__thiscall*)(gCCharacterControl_PS*,eCInpShared::eSGameKeyMessage const&);
static_assert(gESessionKey_Down==14,"Native Sneak logical action profile");
static_assert(gESessionKey_Forward==1&&gESessionKey_Backward==2&&gESessionKey_StrafeLeft==3&&gESessionKey_StrafeRight==4&&gESessionKey_Walk==11,"Native controller movement action profile");
void ReportNativeControl(gCCharacterControl_PS* control,uint16_t key){
    if(GetCurrentThreadId()!=gApplicationThread.load()||(key!=gESessionKey_Down&&key!=gESessionKey_StrafeLeft&&key!=gESessionKey_StrafeRight))return;
    __try {
        auto* player=gCSession::GetInstance().GetPlayer();
        if(!diagnostic::Readable(control,sizeof(*control))||!diagnostic::Readable(player,sizeof(eCDynamicEntity))||player->GetPropertySet(eEPropertySetType_CharacterControl)!=control)return;
        if(key!=gESessionKey_Down&&!diagnostic::NativeDirectionalActivity(player))return;
        static uint64_t next[3]{};unsigned index=key==gESessionKey_Down?0:key==gESessionKey_StrafeLeft?1:2;auto now=GetTickCount64();
        if(control->GetIsPressed()==control->GetIsPressedBefore()&&now<next[index])return;next[index]=now+1000;
        Event event=Record(Kind::Message);sprintf_s(event.text,"vr_native_control_received key=%u pressed=%d before=%d duration=%u native_sneak=%d; original_OnAction_returned",key,control->GetIsPressed(),control->GetIsPressedBefore(),control->GetDurationPressedMSecs(),diagnostic::NativeSneaking(player));Push(event,true);
    }__except(EXCEPTION_EXECUTE_HANDLER){Message("vr_native_control_probe_refused",GetExceptionCode(),true);}
}
bool SendNativeSneakAction(uintptr_t actor,diagnostic::NativeGameAction const& message){
    // Dispatch the same logical action that keyboard bindings resolve to. Use
    // the verified original handler so its properties AND native game script
    // run; changing only the wished movement mode does not activate perception.
    if(GetCurrentThreadId()!=gApplicationThread.load()||!gActionHook.installed||!gActionHook.original||!gApplication||gNativeSneakFault)return false;
    if((message.flags&1)&&(!gApplication->IsGameRunning()||gWorldLifecycle.Blocked()))return false;
    __try {
        auto* player=gCSession::GetInstance().GetPlayer();
        if(reinterpret_cast<uintptr_t>(player)!=actor||!diagnostic::Readable(player,sizeof(eCDynamicEntity)))return false;
        auto* control=static_cast<gCCharacterControl_PS*>(player->GetPropertySet(eEPropertySetType_CharacterControl));
        if(!diagnostic::Readable(control,sizeof(*control)))return false;
        int result=reinterpret_cast<ActionCallback>(gActionHook.original)(control,*reinterpret_cast<eCInpShared::eSGameKeyMessage const*>(&message));
        ReportNativeControl(control,gESessionKey_Down);
        Event event=Record(Kind::Message);sprintf_s(event.text,"vr_native_sneak_action key=14 pressed=%d before=%d duration=%u result=%d; logical_Down; native_script_chained",message.flags&1,(message.flags>>1)&1,message.duration,result);Push(event,true);
        return true; // Native refusal (skills/state) remains native behavior.
    }__except(EXCEPTION_EXECUTE_HANDLER){gNativeSneakFault=true;Message("vr_native_sneak_action_fault; native_keyboard_preserved",GetExceptionCode(),true);return false;}
}
void ApplyArmedSneak(bool requested){
    if(GetCurrentThreadId()!=gApplicationThread.load()||!gApplication||!gApplication->IsGameRunning()){gArmedSneak.Abandon();return;}
    __try {gArmedSneak.Apply(gCSession::GetInstance().GetPlayer(),requested);}
    __except(EXCEPTION_EXECUTE_HANDLER){gArmedSneak.Abandon();Message("vr_armed_sneak_stance_refused",GetExceptionCode(),true);}
}
void ReleaseNativeSneak(){gNativeSneakAction.Release(GetTickCount64(),SendNativeSneakAction);ApplyArmedSneak(false);}
bool NativeSneakOwned(gCCharacterControl_PS* self){
    if(GetCurrentThreadId()!=gApplicationThread.load()||gNativeSneakFault||!gApplication||!gApplication->IsGameRunning()||gWorldLifecycle.Blocked())return false;
    __try {auto* player=gCSession::GetInstance().GetPlayer();return diagnostic::Readable(player,sizeof(eCDynamicEntity))&&
        player->GetPropertySet(eEPropertySetType_CharacterControl)==self&&gNativeSneakAction.Owns(reinterpret_cast<uintptr_t>(player),GetTickCount64());}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
GEBool GE_STDCALL ArmedSneakState(bTObjStack<gScriptRunTimeSingleState>& stack,gCScriptProcessingUnit* unit){
    bool owned=false;
    __try {if(GetCurrentThreadId()==gApplicationThread.load()&&!gWorldLifecycle.Blocked()&&!gStereo.shuttingDown&&diagnostic::Readable(unit,sizeof(*unit))&&gApplication&&gApplication->IsGameRunning()){
        auto* player=unit->GetSelfEntity();owned=player==gCSession::GetInstance().GetPlayer()&&diagnostic::Readable(player,sizeof(eCDynamicEntity))&&
            gNativeSneakAction.Owns(reinterpret_cast<uintptr_t>(player),GetTickCount64())&&!MenuPanelActive()&&!diagnostic::NativeActivity(player)&&!diagnostic::NativeRecovery(player)&&!diagnostic::NativeAnimalForm(player);
    }}__except(EXCEPTION_EXECUTE_HANDLER){owned=false;}
    if(owned){Message("vr_armed_sneak_enter; native_Normal_Sneak; melee_loop_preserved",0,true);return gArmedSneak.Enter(stack,unit);}
    return reinterpret_cast<gFScriptAIState>(gArmedSneak.stateHook.original)(stack,unit);
}
void EnsureArmedSneak(){
    static uint64_t next=0;auto now=GetTickCount64();if(gArmedSneak.stateHook.installed||!gActionHook.installed||!gApplication||!gApplication->IsGameRunning()||gWorldLifecycle.Blocked()||now<next)return;next=now+5000;
    __try {Message(gArmedSneak.Install(reinterpret_cast<void*>(&ArmedSneakState))?"vr_armed_sneak_hook_installed; SDK_state_callback; native_input_and_skills":"vr_armed_sneak_hook_refused; native_state_preserved",0,true);}
    __except(EXCEPTION_EXECUTE_HANDLER){Message("vr_armed_sneak_profile_refused",GetExceptionCode(),true);}
}
void __cdecl ActionOriginal(void* context)
{
    auto& action=*static_cast<ActionContext*>(context);
    action.result=reinterpret_cast<ActionCallback>(gActionHook.original)(action.self,*action.message);
    ReportNativeControl(action.self,action.key);
    if(!Tracing() && !gSample.load(std::memory_order_relaxed))return;
    __try {ControlRecord("action_direction",&gHeadControl.appliedPose,gHeadControl.camera,0,action.self,action.key);}
    __except(EXCEPTION_EXECUTE_HANDLER) {Message("action_direction_probe_fault",GetExceptionCode());}
}
bool MovementMessage(eCInpShared::eSGameKeyMessage const& message,uint16_t& key)
{
    __try {return diagnostic::Readable(&message,sizeof(key)) && diagnostic::HorizontalMovementMessage(&message,key);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
int __fastcall ProcessPlayerAction(gCCharacterControl_PS* self,void*,eCInpShared::eSGameKeyMessage const& message)
{
    uint16_t key=0;
    bool movement=MovementMessage(message,key);
    // The engine can poll released keyboard bindings after our logical batch.
    // While the joystick owns movement, those polls must neither overwrite its
    // timers nor release Sprint. Keyboard movement resumes at joystick neutral.
    if(GetCurrentThreadId()==gApplicationThread.load()&&(movement||key==gESessionKey_Walk)&&gApplication&&gApplication->IsGameRunning()&&!gWorldLifecycle.Blocked()){
        __try {auto* player=gCSession::GetInstance().GetPlayer();
            if(diagnostic::Readable(player,sizeof(eCDynamicEntity))&&player->GetPropertySet(eEPropertySetType_CharacterControl)==self&&
               !gCSession::GetInstance().IsPaused()&&!MenuPanelActive()&&!diagnostic::NativeDirectionalActivity(player)&&
               gNativeMovement.OwnsPoll(reinterpret_cast<uintptr_t>(player),GetTickCount64(),key,gProcess.load(std::memory_order_relaxed)))return 1;
        }__except(EXCEPTION_EXECUTE_HANDLER){gNativeMovement.Reset();}
    }
    // Physical Down polls cannot cancel an owned controller hold. Our logical
    // press/release bypasses this hook through the verified original handler.
    if(key==gESessionKey_Down&&NativeSneakOwned(self))return 1;
    if(GetCurrentThreadId()==gApplicationThread.load()&&key==gESessionKey_WalkToggle&&NativeControllerOwns(self))return 1;
    if(GetCurrentThreadId()==gApplicationThread.load()&&BlockPhysicalSwordClick(self,key))return 1;
    if(GetCurrentThreadId()==gApplicationThread.load()&&key==gESessionKey_Use2&&gHeadControl.enabled&&gConfig.stereoEnabled&&gVrSettings.trackedArms&&!MenuPanelActive()){
        __try {auto* player=gCSession::GetInstance().GetPlayer();alignas(4) unsigned char release[16];
            if(diagnostic::Readable(player,sizeof(eCDynamicEntity))&&player->GetPropertySet(eEPropertySetType_CharacterControl)==self&&AllowNativeParry(player)&&
               !gMagic.Selected(player)&&!diagnostic::NativeActivity(player)&&!diagnostic::NativeRecovery(player)&&diagnostic::Readable(&message,13)&&
               diagnostic::ParryReleaseMessage(&message,release))
                return reinterpret_cast<ActionCallback>(gActionHook.original)(self,*reinterpret_cast<eCInpShared::eSGameKeyMessage const*>(release));
        } __except(EXCEPTION_EXECUTE_HANDLER) {Message("vr_parry_release_guard_fault",GetExceptionCode(),true);}
    }
    // Hardware movement keeps the native binding path. Controller directions
    // are batched at ProcessMovements instead of asynchronous Windows keys.
    if(GetCurrentThreadId()==gApplicationThread.load() && movement && BeginMovementSteering(self,true))
    {
        ActionContext action{self,&message,key,0};
        diagnostic::CallWithRestoration(&ActionOriginal,&MovementCleanup,&action);
        return action.result;
    }
    int result=reinterpret_cast<ActionCallback>(gActionHook.original)(self,message);ReportNativeControl(self,key);return result;
}
void FollowRoomScale(gCCharacterMovement_PS* self)
{
    if(gCinemaScope.Active(GetCurrentThreadId()))return;
    if(gRoomScaleFault||!gVrSettings.roomScaleWalking||!gVrSettings.stableHorizon||!gConfig.stereoEnabled||
       !gHeadControl.enabled||!gHeadControl.centered||gHeadControl.recenterPending||!gStereoRuntime.Connected()||
       gStereo.shuttingDown||gStereo.fault||!gApplication||!gApplication->IsGameRunning())return;
    DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);if(foreground!=GetCurrentProcessId())return;
    auto& session=gCSession::GetInstance();auto* player=session.GetPlayer();
    if(session.IsPaused()||!diagnostic::Readable(player,sizeof(eCDynamicEntity))||
       player->GetPropertySet(eEPropertySetType_CharacterMovement)!=self||!diagnostic::Readable(self,sizeof(*self))||
       !diagnostic::RoomWalkingMode(int(self->GetMovementMode()))||diagnostic::NativeRecovery(player)||diagnostic::NativeActivity(player)||diagnostic::NativeAnimalForm(player)||self->IsMounted()||
       self->IsCombatMovementEnabledFromSPU())return;
    LARGE_INTEGER now;QueryPerformanceCounter(&now);
    float elapsedSeconds=gRoomScalePreviousQpc&&gFrequency.QuadPart>0?
        float(double(now.QuadPart-gRoomScalePreviousQpc)/double(gFrequency.QuadPart)):0.f;
    gRoomScalePreviousQpc=now.QuadPart;
    auto* roomGui=session.GetGUIManager();if(!roomGui||!diagnostic::Readable(roomGui,sizeof(void*))||roomGui->IsMenuOpen()||roomGui->IsAnyPageOpen())return;
    Event cameraMode;CaptureCameraMode(cameraMode);if(!cameraMode.cameraModeValid||!cameraMode.firstPerson||cameraMode.dialogCamera)return;
    diagnostic::HeadPose head;
    if(!TryAcquireSRWLockShared(&gHeadPoseLock))return;head=gHeadPose;ReleaseSRWLockShared(&gHeadPoseLock);
    if(head.session!=gHeadControl.centerSession||!diagnostic::RoomPositionTracked(head,now.QuadPart,gFrequency.QuadPart))return;
    auto const& cameraRotation=gApplication->GetCurrentCamera().GetRotation();
    float angles[3]{cameraRotation.m_fX,cameraRotation.m_fY,cameraRotation.m_fZ};
    if(gHeadControl.active)memcpy(angles,gHeadControl.nativeRotation,sizeof(angles));
    auto rotation=diagnostic::UprightCamera(diagnostic::NativeCameraRotation(angles));
    float offset[3]{},request[3]{};
    if(!diagnostic::RoomNeckOffset(gHeadControl.center,rotation,head,gStereo.centerPosition,gConfig.unitsPerMeter,float(gVrSettings.numbers[6])*.01f*gConfig.unitsPerMeter,offset)||
       !diagnostic::RoomStep(offset,gConfig.unitsPerMeter,request,elapsedSeconds))return;
    auto& sensor=self->GetSensor();if(!diagnostic::Readable(&sensor,sizeof(sensor)))return;
    // Verified CP sensor guard at ProcessPlayerMovements +0x9c and binding at
    // +0x36. The function otherwise falls back to copying an unchecked goal.
    // Read-only inspection; the parent movement hook verifies Game.dll SHA256.
    auto* sensorBytes=reinterpret_cast<unsigned char const*>(&sensor);gCCharacterMovement_PS* bound=nullptr;
    memcpy(&bound,sensorBytes+0x3c,sizeof(bound));if(sensorBytes[0x38]!=1||bound!=self)return;
    auto* body=static_cast<eCRigidBody_PS*>(player->GetPropertySet(eEPropertySetType_RigidBody));
    if(!diagnostic::Readable(body,sizeof(*body)))return;
    auto before=player->GetWorldPosition();auto bodyBefore=body->GetPosition();auto velocityBefore=body->GetLinearVelocity();
    diagnostic::RoomNativeState state;state.Save(&sensor,self);bCVector moved(0,0,0);float accepted[3]{};
    // Probe a separate room step without replacing the joystick's goal, its
    // goal-changed flags, smoothed positions or rotation. Restore on faults too.
    __try {
        float target[3]{before.m_fX+request[0],before.m_fY,before.m_fZ+request[2]};
        diagnostic::RoomNativeState::Request(&sensor,target);
        static_cast<CharacterSensorAccess*>(&sensor)->ProcessPlayerMovements();
        auto result=sensor.GetUnsmoothedPosition();accepted[0]=result.m_fX-before.m_fX;accepted[1]=result.m_fY-before.m_fY;accepted[2]=result.m_fZ-before.m_fZ;
    } __finally {state.RestoreSensor(&sensor);}
    if(!diagnostic::RoomAccepted(request,accepted,gConfig.unitsPerMeter,self->GetStepHeight())||std::hypot(accepted[0],accepted[2])<=.001f*gConfig.unitsPerMeter)return;
    __try {
        diagnostic::RoomNativeState::BeginTransfer(self);
        gRoomMotionScope.Begin(self,GetCurrentThreadId());
        // Match native controlled movement's guarded body/entity transfer.
        // The unguarded MoveWorld callback resets running velocity to zero.
        body->SetPosition(bCVector(bodyBefore.m_fX+accepted[0],bodyBefore.m_fY+accepted[1],bodyBefore.m_fZ+accepted[2]));
        // Absolute target also avoids double displacement if the body setter
        // has already notified the owning entity about its new position.
        player->SetWorldPosition(bCVector(before.m_fX+accepted[0],before.m_fY+accepted[1],before.m_fZ+accepted[2]));
    } __finally {
      __try {
        state.RestoreGuard(self);
        auto after=player->GetWorldPosition();moved=bCVector(after.m_fX-before.m_fX,after.m_fY-before.m_fY,after.m_fZ-before.m_fZ);
        float actual[3]{moved.m_fX,moved.m_fY,moved.m_fZ};state.Commit(&sensor,self,actual);
        body->SetPosition(bCVector(bodyBefore.m_fX+moved.m_fX,bodyBefore.m_fY+moved.m_fY,bodyBefore.m_fZ+moved.m_fZ));
        // Native controlled transfers synchronize both fixed-step positions
        // (Engine.dll 0x179460), not just the actor. Otherwise PhysX blending
        // can pull the entity back toward the pre-transfer pose after the
        // shared tracking origin has already consumed the physical step.
        body->SetFixedStepTranslation(player->GetWorldPosition());
        // Translation preserves velocity, including vertical terrain physics.
        // Restore through the SDK even if a native notification changed it.
        body->SetLinearVelocity(velocityBefore);
        if(!diagnostic::ConsumeRoomStep(gHeadControl.center,rotation,actual,gConfig.unitsPerMeter,gStereo.centerPosition))gRoomScaleFault=true;
        else gStereo.cameraStabilizer.RebaseRoomTransfer(reinterpret_cast<uintptr_t>(player),actual);
      } __finally {gRoomMotionScope.End();}
    }
    ++gRoomScaleMoves;
    if(GetTickCount64()>=gRoomScaleNextLog){gRoomScaleNextLog=GetTickCount64()+500;
        Event event=Record(Kind::Message);sprintf_s(event.text,"vr_room_scale request_cm=%.2f,%.2f accepted_cm=%.2f,%.2f,%.2f moves=%llu mode=%d world_callbacks=%llu velocity=%.1f,%.1f,%.1f dt_ms=%.2f post_native=1 fixed_step=1 history=1",request[0],request[2],moved.m_fX,moved.m_fY,moved.m_fZ,gRoomScaleMoves,int(self->GetMovementMode()),gRoomMotionScope.suppressed,velocityBefore.m_fX,velocityBefore.m_fY,velocityBefore.m_fZ,elapsedSeconds*1000.f);Push(event,true);}

}
void __fastcall RoomWorldMatrixUpdated(gCCharacterMovement_PS* self,void*)
{
    // CP OnUpdatedWorldMatrix forces a living actor into Standing (mode 1)
    // after StopMovement, even with +0x264 set. Bypass only our already
    // collision-checked, explicitly synchronized room transfer notification.
    if(gRoomMotionScope.Suppress(self,GetCurrentThreadId()))return;
    reinterpret_cast<void(__thiscall*)(gCCharacterMovement_PS*)>(gRoomWorldHook.original)(self);
}
void ReportNativeMotion(gCCharacterMovement_PS* self){
    if(!gSample.load(std::memory_order_relaxed)||!gApplication||!gApplication->IsGameRunning()||gWorldLifecycle.Blocked()||!gVrSettings.controllers)return;
    auto* player=gCSession::GetInstance().GetPlayer();
    if(!diagnostic::Readable(player,sizeof(eCDynamicEntity))||player->GetPropertySet(eEPropertySetType_CharacterMovement)!=self)return;
    static uintptr_t previousPlayer=0;static uint64_t previousTime=0;static bCVector previousPosition(0,0,0);
    auto now=GetTickCount64();auto actor=reinterpret_cast<uintptr_t>(player);if(actor==previousPlayer&&now>=previousTime&&now-previousTime<1000)return;
    auto position=player->GetWorldPosition();float displacement=0;
    if(actor==previousPlayer&&now>previousTime&&now-previousTime<5000){float x=position.m_fX-previousPosition.m_fX,z=position.m_fZ-previousPosition.m_fZ;displacement=std::hypot(x,z);}
    previousPlayer=actor;previousTime=now;previousPosition=position;
    auto* control=static_cast<gCCharacterControl_PS*>(player->GetPropertySet(eEPropertySetType_CharacterControl));
    int wish=-1;bCVector direction(0,0,0);if(diagnostic::Readable(control,sizeof(*control))){memcpy(&wish,reinterpret_cast<unsigned char const*>(control)+0xac,sizeof(wish));direction=control->GetWishedDirection();}
    Event event=Record(Kind::Message);
    _snprintf_s(event.text,_TRUNCATE,"vr_native_motion mode=%d wish=%d enabled=%d combat=%d contact=%d recovery=%d owned=%d dir=%.2f,%.2f delta_cm=%.2f",
        int(self->GetMovementMode()),wish,self->IsMovementEnabledFromSPU(),self->IsCombatMovementEnabledFromSPU(),self->GetCombatContact(),diagnostic::NativeRecovery(player),gControllerModes.Owns(actor,now,gProcess.load(std::memory_order_relaxed)),direction.m_fX,direction.m_fZ,displacement);Push(event,true);
    auto sensor=self->GetSensor().GetUnsmoothedPosition();auto goal=self->GetGoalPosition();
    event=Record(Kind::Message);_snprintf_s(event.text,_TRUNCATE,"vr_native_motion_pose p=%.1f,%.1f,%.1f sensor_delta=%.2f,%.2f,%.2f goal_delta=%.2f,%.2f,%.2f",
        position.m_fX,position.m_fY,position.m_fZ,sensor.m_fX-position.m_fX,sensor.m_fY-position.m_fY,sensor.m_fZ-position.m_fZ,goal.m_fX-position.m_fX,goal.m_fY-position.m_fY,goal.m_fZ-position.m_fZ);Push(event,true);
    if(gMelee.ready){auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
        if(diagnostic::Readable(routine,sizeof(*routine))){auto& spu=routine->GetSPU();if(diagnostic::Readable(&spu,sizeof(spu))){int action=-1;memcpy(&action,reinterpret_cast<unsigned char const*>(&spu)+0x154,sizeof(action));
            event=Record(Kind::Message);_snprintf_s(event.text,_TRUNCATE,"vr_native_combat_owner callback=%08lx instruction_action=%d play_s=%.3f duration_s=%.3f rotation=%d",
                DWORD(reinterpret_cast<uintptr_t>(spu.m_pfInstrCallback)),action,spu.m_fInstrPlayTime,spu.m_fInstrDuration,self->IsCombatRotationEnabledFromSPU());Push(event,true);}}}
}
void __fastcall ProcessPlayerPhysics(gCCharacterMovement_PS* self,void*)
{
    float originalTurnSpeed=0;bool turnSpeedLeased=false;
    // ICS Classic Camera changes the HERO's TurnSpeedMax from200 to1000.
    // Borrow that turn responsiveness only while VR owns human ground input;
    // leave translation speed, root motion, collision and progression native.
    if(GetCurrentThreadId()==gApplicationThread.load()&&gConfig.stereoEnabled&&gHeadControl.enabled&&!gStereo.shuttingDown&&!gWorldLifecycle.Blocked()&&
       gApplication&&gApplication->IsGameRunning()&&!MenuPanelActive()&&!gRadialOpen&&!gCSession::GetInstance().IsPaused()){
        __try {auto* player=gCSession::GetInstance().GetPlayer();
            if(diagnostic::Readable(player,sizeof(eCDynamicEntity))&&player->GetPropertySet(eEPropertySetType_CharacterMovement)==self&&
               gControllerModes.Owns(reinterpret_cast<uintptr_t>(player),GetTickCount64(),gProcess.load(std::memory_order_relaxed))&&
               diagnostic::RoomWalkingMode(int(self->GetMovementMode()))&&!diagnostic::NativeRecovery(player)&&!diagnostic::NativeActivity(player)&&!diagnostic::NativeAnimalForm(player)&&!self->IsMounted()){
                originalTurnSpeed=self->GetTurnSpeedMax();if(std::isfinite(originalTurnSpeed)&&originalTurnSpeed>0&&originalTurnSpeed<1000.f){self->SetTurnSpeedMax(1000.f);turnSpeedLeased=true;}
            }
        }__except(EXCEPTION_EXECUTE_HANDLER){turnSpeedLeased=false;}
    }
    // All joystick translation, velocity, acceleration and animation goals are
    // native. The accepted collision-checked room-scale transfer runs afterward.
    if(gMovementTrace.Armed()&&GetCurrentThreadId()==gApplicationThread.load())__try{TraceControllerPhysics(self,"before_native_physics");}__except(EXCEPTION_EXECUTE_HANDLER){gMovementTrace.Reset();}
    __try {reinterpret_cast<void(__thiscall*)(gCCharacterMovement_PS*)>(gPhysicsProcessHook.original)(self);}
    __finally {if(turnSpeedLeased&&self->GetTurnSpeedMax()==1000.f)self->SetTurnSpeedMax(originalTurnSpeed);}
    if(GetCurrentThreadId()==gApplicationThread.load()){
        if(gMovementTrace.Armed())__try{TraceControllerPhysics(self,"after_native_physics");}__except(EXCEPTION_EXECUTE_HANDLER){gMovementTrace.Reset();}
        __try{RestoreBodyFacing(self);}__except(EXCEPTION_EXECUTE_HANDLER){gBodyFacingLease.Forget();gBodyFacingFault=true;Message("vr_head_body_alignment_restore_refused",GetExceptionCode(),true);}
        __try{FollowRoomScale(self);}__except(EXCEPTION_EXECUTE_HANDLER){gRoomScaleFault=true;Message("vr_room_scale_fault; body_follow_disabled",GetExceptionCode(),true);}
        __try{ReportNativeMotion(self);}__except(EXCEPTION_EXECUTE_HANDLER){static bool reported=false;if(!reported){reported=true;Message("vr_native_motion_probe_refused; no_movement_changes",GetExceptionCode(),true);}}
    }
}
bool Pressed(int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; }
void __fastcall ProcessInstantMouseX(gCCameraAI_PS* self,void*,float input){
    if(GetCurrentThreadId()==gApplicationThread.load()&&self==gInstantTurnCamera&&std::isfinite(input)&&!gWorldLifecycle.Blocked()&&!gStereo.shuttingDown&&!MenuPanelActive()&&!gCinemaScope.Active(GetCurrentThreadId())){
        gLastControllerTurnCamera=self;
        // GetCurrentWishedTranslation is a preview, also called from inside
        // GetCurrentWishedRotation. A preview must not consume the update's
        // controller turn or replace it with zero before rotation uses it.
        *reinterpret_cast<float*>(reinterpret_cast<unsigned char*>(self)+diagnostic::MouseFilteredXOffset)=gTurnEvaluation.Filter(reinterpret_cast<uintptr_t>(self),input);
        return;
    }
    if(GetCurrentThreadId()==gApplicationThread.load()&&self==gLastControllerTurnCamera){
        *reinterpret_cast<float*>(reinterpret_cast<unsigned char*>(self)+diagnostic::MouseFilteredXOffset)=0;
        gLastControllerTurnCamera=nullptr;gControllerYaw.Reset();gTurnOutput.Reset();
    }
    reinterpret_cast<void(__thiscall*)(gCCameraAI_PS*,float)>(gMouseFilterXHook.original)(self,input);
}
// MSVC x86 class-by-value ABI: ECX=this, stack=caller-owned matrix result,
// EAX=result pointer, callee pops four bytes. Do not construct a second matrix
// or change the native return value/exception behavior.
using WishedRotationCallback=bCMatrix*(__thiscall*)(gCCameraAI_PS*,bCMatrix*);
bCMatrix* EvaluateInstantRotation(gCCameraAI_PS* self,bCMatrix* result,WishedRotationCallback original){
    diagnostic::NativeTurnInterpolation interpolation;
    if(gNativeTurnInterpolationReady&&gApplication&&diagnostic::Readable(self,sizeof(*self)))
        interpolation.Begin(self,sizeof(*self),gApplication->GetFrameTimeInSeconds());
    __try {return original(self,result);}
    __finally {interpolation.End();}
}
bCMatrix* __fastcall ProcessControllerWishedRotation(gCCameraAI_PS* self,void*,bCMatrix* result){
    using Original=bCMatrix*(__thiscall*)(gCCameraAI_PS*,bCMatrix*);
    auto original=reinterpret_cast<Original>(gWishedRotationHook.original);
    auto identity=reinterpret_cast<uintptr_t>(self);
    if(GetCurrentThreadId()!=gApplicationThread.load()||self!=gInstantTurnCamera||!gMouseFilterXHook.original||
       gTurnEvaluation.Owns(identity)||gWorldLifecycle.Blocked()||gStereo.shuttingDown||MenuPanelActive()||gCinemaScope.Active(GetCurrentThreadId()))return original(self,result);
    auto update=gProcess.load(std::memory_order_relaxed);
    if(gTurnOutput.Matches(identity,update)){
        auto* returned=EvaluateInstantRotation(self,result,original);if(returned)gTurnOutput.Apply(identity,update,reinterpret_cast<float*>(returned));return returned;
    }
    auto* bytes=reinterpret_cast<unsigned char*>(self);void const* preset=nullptr;float zoom=0,mode=0,sensitivity=0,native=0;
    memcpy(&preset,bytes+0x1e4,sizeof(preset));
    if(diagnostic::Readable(preset,16))memcpy(&zoom,static_cast<unsigned char const*>(preset)+12,sizeof(zoom));
    memcpy(&mode,bytes+0x358,sizeof(mode));memcpy(&sensitivity,bytes+0x548,sizeof(sensitivity));
    memcpy(&native,bytes+diagnostic::MouseFilteredXOffset,sizeof(native));
    auto now=GetTickCount64();float pixels=gControllerYaw.Consume(gProcess.load(std::memory_order_relaxed),now);
    float angle=diagnostic::NativeTurnAngle(pixels,self->GetInvertXAxis(),zoom,mode,sensitivity);
    if(!gTurnEvaluation.Begin(identity,native,angle))return original(self,result);
    gLastControllerTurnCamera=self;
    *reinterpret_cast<float*>(bytes+diagnostic::MouseFilteredXOffset)=gTurnEvaluation.Filter(identity,native);
    bCMatrix* returned=nullptr;
    __try {returned=EvaluateInstantRotation(self,result,original);}
    __finally {*reinterpret_cast<float*>(bytes+diagnostic::MouseFilteredXOffset)=gTurnEvaluation.End(identity);}
    if(returned&&angle!=0)gTurnOutput.Store(identity,update,reinterpret_cast<float const*>(returned));
    static uint64_t next=0;if(pixels&&now>=next){next=now+1000;Event event=Record(Kind::Message);
        sprintf_s(event.text,"vr_native_controller_turn pixels=%.3f delta_deg=%.3f gain=%.6f; rotation_evaluation; once_per_update",pixels,angle*57.2957795f,angle/pixels);Push(event,true);}
    return returned;
}
bool gMapperLocatorReady=false;
diagnostic::mapping::Engine gControllerMapper;
diagnostic::mapping::Link gControllerMapperLink;
SRWLOCK gControllerMapperLock=SRWLOCK_INIT;
diagnostic::ControllerFrame MappedControllers(){
    auto raw=gStereoRuntime.Controllers();auto now=GetTickCount64();
    diagnostic::ControllerFrame frame;AcquireSRWLockExclusive(&gControllerMapperLock);
    __try {
    static bool linked=false;static uint64_t published=0;static bool publishedFocus=false;
    if(!linked&&gIni[0])linked=gControllerMapperLink.Open(gIni,true);
    auto buttons=gControllerMapper.Buttons(raw);
    if(raw.timeMs!=published||raw.focused!=publishedFocus){gControllerMapperLink.Publish(raw,buttons);published=raw.timeMs;publishedFocus=raw.focused;}
    bool capture=gControllerMapperLink.Capturing(now);
    frame=gControllerMapper.Step(raw,gVrSettings.controllerBindings,now,capture,gMapperLocatorReady?~0u:~(1u<<diagnostic::mapping::Locator));
    } __finally {ReleaseSRWLockExclusive(&gControllerMapperLock);}
    return frame;
}

void BeginInstantControllerTurn(){
    gInstantTurnCamera=nullptr;
    if(gCinemaScope.Active(GetCurrentThreadId())||!gVrSettings.instantControllerTurn||!gMouseFilterXHook.original||!gWishedRotationHook.original||!gVrSettings.controllers||!gHeadControl.enabled||!gConfig.stereoEnabled||gStereo.fault||gStereo.shuttingDown||!gApplication||!gApplication->IsGameRunning()||gRadialOpen||MenuPanelActive())return;
    DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);auto frame=MappedControllers();auto now=GetTickCount64();
    if(foreground!=GetCurrentProcessId()||!frame.focused||!frame.hands[1].active||now<frame.timeMs||now-frame.timeMs>100)return;
    auto& session=gCSession::GetInstance();auto* player=session.GetPlayer();auto* camera=session.GetCamera_PS();
    if(gCameraReturn.Blocks(reinterpret_cast<uintptr_t>(player))||session.IsPaused()||!diagnostic::Readable(player,sizeof(eCDynamicEntity))||diagnostic::NativeAnimalForm(player)||diagnostic::NativeRecovery(player)||CameraThirdPersonActivity(player)||gFinisher.Busy(player)||
       gAutoFirstPersonProcess==gProcess.load(std::memory_order_relaxed)||!diagnostic::Readable(camera,sizeof(*camera))||!camera->IsInFirstPerson()||camera->IsDialogCameraActive())return;
    gInstantTurnCamera=camera;
}
void StopStereo()
{
    gThirdPersonOrbit.Reset();
    gLocatorProjection.Reset();
    gTrackedArms.ResetBodyPosture();
#ifdef G3VR_NAVIGATION_BUDGET_RUNTIME
    gNavigationBudget.Native();
#endif
    gCombatMovementRecovery.Reset();
    RestoreCurrentBodyFacing();gHeadBodyHeading.Reset();
    gKnockoutCamera.Reset();gActivityCameraGate.Reset();gCameraReturn.Reset();
    gChestVisual.Reset();gChestView.Reset();gLecternView.Reset();gDialogCamera.Reset();gStereo.dialog=gStereo.chest=gStereo.lectern=false;gStereo.dialogPlayer=0;
    RestoreCurrentHeroVoice();
    gFlameTargets.Reset();gTorchParticles.Reset();gParticleWorld.End();
    ReleaseNativeSneak();
    ReleaseNativeControllerActions();gControllerModes.Reset();gPhysicalCrouchWalk.Reset();gSneakToggle.Reset();gGuardPose.Reset();gQuickHeal.Reset();gInstantTurnCamera=nullptr;gLastControllerTurnCamera=nullptr;gControllerYaw.Reset();gTurnOutput.Reset();gActivityCameraGate.Reset();
    gMelee.Reset();if(gWorldLifecycle.Blocked()||!gApplication||!gApplication->IsGameRunning())gBow.Abandon();else gBow.Cancel();gMagic.Cancel();gTrackedArms.ResetCombatSample();gRadialOpen=false;
    gControllerInput.Release();
    gWorldInteraction.selected=0;gControllerTranslator.ResetPointer();
    gStereo.framePending=false;gStereo.uiRendering=false;
    gStereoRuntime.Cancel();gStereoRuntime.Disconnect();
    gStereo.cameraMode=-1;
    gStereo.heightOffsetUnits=0;
    gStereo.heightAnchor.Reset();
    gStereo.cameraStabilizer.Reset();gStereo.basePrepared=false;gStereo.playerHeadingValid=false;
    diagnostic::HeadPose invalid;LARGE_INTEGER qpc;QueryPerformanceCounter(&qpc);invalid.qpc=qpc.QuadPart;PublishHeadPose(invalid);
}
void __fastcall StereoDestroyEngine(eCApplication* self,void*)
{
#ifdef G3VR_SCENE_COST_TRIAL
    FlushSceneCost(true);
#endif
    // Engine::DestroyEngine invokes this virtual callback before shutting down
    // its modules, including the native D3D9 device. Never do GPU cleanup from
    // DllMain: the device and OpenXR runtime must still be alive here.
    gWorldLifecycle.BeginShutdown();gStereo.shuttingDown=true;
    gHeadControl.enabled=false;gHeadControl.centered=false;
    Message("stereo_engine_shutdown_begin",0,true);
    gLocatorNativeInput.Restore();
    StopStereo();
    Message("stereo_resources_released_before_engine_shutdown",0,true);
    using Destroy=void(__thiscall*)(eCApplication*);
    reinterpret_cast<Destroy>(gDestroyEngineHook.original)(self);
}
void StereoFailure(char const* reason)
{
    gHeadControl.enabled=false;gHeadControl.centered=false;gStereo.fault=true;
    Event event=Record(Kind::Message);sprintf_s(event.text,"stereo_disabled: %.160s",reason);Push(event,true);StopStereo();
}
void __fastcall StereoProjectionUpdate(eCCameraBase* camera,void*,bool calculate)
{
    using Update=void(__thiscall*)(eCCameraBase*,bool);
    if(gStereo.eyeActive && camera==gStereo.camera && GetCurrentThreadId()==gApplicationThread.load())
    {
        // RenderLowPoly changes ClipDepth before calling UpdateProjection(true)
        // and restores it afterwards. A frozen depth range clips the distant
        // world at the detailed-world boundary, regardless of native/EDD LOD.
        auto const& clip=camera->GetClipDepth();
        if(diagnostic::StereoProjectionDepth(clip.GetX(),clip.GetY(),gStereo.projection)&&
           clip.GetY()>gStereo.detailFar&&clip.GetY()!=gStereo.lastDistantFar){
            gStereo.lastDistantFar=clip.GetY();char text[180];
            sprintf_s(text,"vr_distant_projection_ready near_cm=%.1f far_cm=%.1f detail_far_cm=%.1f; native_pass_depth; XR_lens_preserved",clip.GetX(),clip.GetY(),gStereo.detailFar);Message(text,0,true);
        }
        bCMatrix matrix;for(int row=0;row<4;++row)for(int column=0;column<4;++column)matrix(row,column)=gStereo.projection[row*4+column];
        camera->SetProjectionMatrix(matrix);
        reinterpret_cast<Update>(gProjectionHook.original)(camera,false);
    }
    else reinterpret_cast<Update>(gProjectionHook.original)(camera,calculate);
}
struct CameraSnapshot
{
    float position[3]{},rotation[3]{},view[16]{},projection[16]{},clipDepth[2]{};
};
void SaveStereoCamera(eCCameraBase& camera,CameraSnapshot& saved)
{
    auto const& clip=camera.GetClipDepth();saved.clipDepth[0]=clip.GetX();saved.clipDepth[1]=clip.GetY();
    auto const& p=camera.GetPosition();saved.position[0]=p.m_fX;saved.position[1]=p.m_fY;saved.position[2]=p.m_fZ;
    auto const& r=camera.GetRotation();saved.rotation[0]=r.m_fX;saved.rotation[1]=r.m_fY;saved.rotation[2]=r.m_fZ;
    for(int row=0;row<4;++row)for(int col=0;col<4;++col){saved.view[row*4+col]=camera.GetViewMatrix()(row,col);saved.projection[row*4+col]=camera.GetProjectionMatrix()(row,col);}
}
void RestoreStereoCamera(eCCameraBase& camera,CameraSnapshot const& saved)
{
    gStereo.eyeActive=false;
    auto const& clip=camera.GetClipDepth();
    if(clip.GetX()!=saved.clipDepth[0]||clip.GetY()!=saved.clipDepth[1])
        camera.SetClipDepth(bCVector2(saved.clipDepth[0],saved.clipDepth[1]));
    camera.SetPosition(bCVector(saved.position[0],saved.position[1],saved.position[2]));camera.SetRotation(bCVector(saved.rotation[0],saved.rotation[1],saved.rotation[2]));
    camera.UpdateViewport(GETrue);bCMatrix view,projection;
    for(int row=0;row<4;++row)for(int col=0;col<4;++col){view(row,col)=saved.view[row*4+col];projection(row,col)=saved.projection[row*4+col];}
    camera.SetViewMatrix(view);camera.UpdateViewport(GEFalse);camera.SetProjectionMatrix(projection);
    using Update=void(__thiscall*)(eCCameraBase*,bool);reinterpret_cast<Update>(gProjectionHook.original)(&camera,false);camera.ExecuteGfxCamera();++gStereo.restores;
}
void ReloadFirstPersonHeight(bool report)
{
    auto settings=diagnostic::LoadVrSettings(gIni);
    static bool particleSettingsLogged=false;
    if(!particleSettingsLogged||settings.particleQuality!=gVrSettings.particleQuality||report){
        Event event=Record(Kind::Message);sprintf_s(event.text,"vr_particle_quality profile=%d; native_simulation_and_skills_preserved",settings.particleQuality);Push(event,true);particleSettingsLogged=true;
    }
    static bool settingsLogged=false;
    if(gLogging.load()&&(!settingsLogged||report||settings.numbers[7]!=gVrSettings.numbers[7]||settings.numbers[6]!=gVrSettings.numbers[6]||settings.numbers[0]!=gVrSettings.numbers[0]||settings.stableHorizon!=gVrSettings.stableHorizon||settings.smoothCamera!=gVrSettings.smoothCamera||settings.cameraSmoothingMs!=gVrSettings.cameraSmoothingMs||settings.cameraVerticalSmoothingMs!=gVrSettings.cameraVerticalSmoothingMs)){
        Event event=Record(Kind::Message);sprintf_s(event.text,"vr_camera_settings height=%d forward=%d turn=%d horizon=%d smooth=%d smoothing_ms=%d,%d",settings.numbers[0],settings.numbers[6],settings.numbers[7],settings.stableHorizon,settings.smoothCamera,settings.cameraSmoothingMs,settings.cameraVerticalSmoothingMs);Push(event,true);
        settingsLogged=true;
    }
    if(settings.instantControllerTurn!=gVrSettings.instantControllerTurn||report){
        ReleaseNativeControllerActions();gControllerModes.Reset();gPhysicalCrouchWalk.Reset();gSneakToggle.Reset();gStereo.cameraStabilizer.Reset();
        Event event=Record(Kind::Message);sprintf_s(event.text,"vr_motion_settings native_locomotion=1 instant_turn=%d",settings.instantControllerTurn);Push(event,true);
    }
    if(settings.stableHorizon!=gVrSettings.stableHorizon){gHeadControl.recenterPending=true;gStereo.heightAnchor.Reset();}
    if(settings.stableHorizon!=gVrSettings.stableHorizon||settings.smoothCamera!=gVrSettings.smoothCamera||settings.cameraSmoothingMs!=gVrSettings.cameraSmoothingMs||settings.cameraVerticalSmoothingMs!=gVrSettings.cameraVerticalSmoothingMs)gStereo.cameraStabilizer.Reset();
    if(memcmp(settings.numbers+8,gVrSettings.numbers+8,(51-8)*sizeof(int))||settings.twoHandedMelee!=gVrSettings.twoHandedMelee||settings.heavyTwoHand!=gVrSettings.heavyTwoHand){
        gMelee.Reset();gBow.Cancel();gMagic.Cancel();gTrackedArms.ResetCombatSample();
        Event event=Record(Kind::Message);sprintf_s(event.text,"vr_hand_calibration left=%d,%d,%d/%d,%d,%d right=%d,%d,%d/%d,%d,%d",settings.numbers[8],settings.numbers[9],settings.numbers[10],settings.numbers[11],settings.numbers[12],settings.numbers[13],settings.numbers[14],settings.numbers[15],settings.numbers[16],settings.numbers[17],settings.numbers[18],settings.numbers[19]);Push(event,true);
        sprintf_s(event.text,"vr_sword_calibration pitch=%d yaw=%d roll=%d; automatic_forward_grip",settings.numbers[20],settings.numbers[21],settings.numbers[22]);Push(event,true);
    }
    if(memcmp(settings.numbers+58,gVrSettings.numbers+58,6*sizeof(int))){gTrackedArms.ResetCombatSample();Message("vr_shield_calibration_updated; native_block_rules",0,true);}
    if(!settingsLogged||report||settings.bowReticle!=gVrSettings.bowReticle){Event event=Record(Kind::Message);sprintf_s(event.text,"vr_bow_reticle_setting enabled=%d",settings.bowReticle);Push(event,true);}
    gStereoRuntime.SetFoveation(settings.ffrMode,settings.ffrDiameterPercent,settings.ffrTransitionPercent);
    bool fogChanged=settings.numbers[85]!=gVrSettings.numbers[85];
    gVrSettings=settings;
    if(fogChanged&&gResetNativeFog)gResetNativeFog();
    gConfig.hideFirstPersonHead=gVrSettings.hideHead&&!gHeadVisibilityFault;
    gConfig.keepFirstPerson=gVrSettings.keepFirstPerson&&!gFirstPersonPermissionFault;
    gConfig.firstPersonHeightOffsetCm=float(gVrSettings.numbers[0]);
    if(report)
    {
        Event event=Record(Kind::Message);sprintf_s(event.text,"first_person_height_offset_cm=%.0f; world_vertical; dialog_unchanged",gConfig.firstPersonHeightOffsetCm);Push(event,true);
    }
}
struct HeadBodyPartVisibility
{
    eCVisualAnimationFactory* factory=nullptr;
    bCString const* name=nullptr;
};
struct HeadVisibilitySnapshot
{
    diagnostic::RenderVisibilitySnapshot visibility;
    HeadBodyPartVisibility parts[4]{};
    unsigned partCount=0,entityCount=0;
};
void __cdecl SetHeadBodyPartVisibility(void* object,bool visible)
{
    auto& part=*static_cast<HeadBodyPartVisibility*>(object);
    part.factory->SetBodyPartVisible(*part.name,visible);
}
void __cdecl SetHeadEntityVisibility(void* object,bool visible)
{
    static_cast<eCEntity*>(object)->EnableRendering(visible);
}
void __cdecl SetPlayerRenderAlpha(void* object,float value){static_cast<eCEntity*>(object)->SetRenderAlphaValue(value,GEFalse);}
bool CollectPlayerRenderAlpha(eCEntity* entity,diagnostic::RenderAlphaSnapshot& snapshot,unsigned depth=0){
    if(!diagnostic::Readable(entity,sizeof(eCEntity))||depth>16)return false;
    for(unsigned i=0;i<snapshot.count;++i)if(snapshot.entries[i].object==entity)return true;
    if(!snapshot.Remember(entity,entity->GetRenderAlphaValue(),SetPlayerRenderAlpha))return false;
    int count=entity->GetChildrenCount();if(count<0||count>128)return false;
    for(int i=0;i<count;++i)if(!CollectPlayerRenderAlpha(entity->GetChildAt(i),snapshot,depth+1))return false;
    return true;
}
void BeginPlayerRenderAlpha(diagnostic::RenderAlphaSnapshot& snapshot){
    if(gStereo.chest&&!gChestVisual.Ready()){
        auto* player=gCSession::GetInstance().GetPlayer();
        if(CollectPlayerRenderAlpha(player,snapshot))snapshot.Hide();else snapshot.count=0;
        return;
    }
    if(!gStereo.firstPerson||!gVrSettings.trackedArms)return;
    auto* player=gCSession::GetInstance().GetPlayer();
    if(!diagnostic::Readable(player,sizeof(eCDynamicEntity))||diagnostic::NativeAnimalForm(player)||
       diagnostic::NativeActivity(player)||gFinisher.Busy(player))return;
    if(!CollectPlayerRenderAlpha(player,snapshot)){snapshot.count=0;Message("vr_player_opacity_collection_refused; native_alpha_preserved",0,true);return;}
    static uint64_t next=0;auto now=GetTickCount64();
    if(diagnostic::probes::Enabled&&now>=next){next=now+1000;Event event=Record(Kind::Message);
        auto* movement=static_cast<gCCharacterMovement_PS*>(player->GetPropertySet(eEPropertySetType_CharacterMovement));
        sprintf_s(event.text,"vr_first_person_opacity native_alpha=%.3f entities=%u sprint=%d jump=%d; scoped_both_eyes; hidden_head_preserved",snapshot.entries[0].alpha,snapshot.count,
            diagnostic::Readable(movement,sizeof(*movement))&&movement->IsSprinting(),diagnostic::Readable(movement,sizeof(*movement))&&movement->IsJumping());Push(event,true);}
    snapshot.Opaque();
}
bool CollectHeadEntity(eCEntity* entity,eCEntity* player,HeadVisibilitySnapshot& snapshot,unsigned depth=0)
{
    if(!entity)return true;
    if(entity==player || depth>16 || !diagnostic::Readable(entity,sizeof(eCEntity)))return false;
    for(unsigned i=0;i<snapshot.visibility.count;++i)
        if(snapshot.visibility.entries[i].object==entity && snapshot.visibility.entries[i].set==SetHeadEntityVisibility)return true;
    if(!snapshot.visibility.Remember(entity,entity->IsRenderingEnabled()!=GEFalse,SetHeadEntityVisibility))return false;
    ++snapshot.entityCount;
    auto children=entity->GetChildrenCount();if(children<0||children>64)return false;
    for(int i=0;i<children;++i)
        if(!CollectHeadEntity(entity->GetChildAt(i),player,snapshot,depth+1))return false;
    return true;
}
bool CollectPlayerHeadVisibility(HeadVisibilitySnapshot& snapshot)
{
    auto* player=gCSession::GetInstance().GetPlayer();if(!player)return true;
    auto* animation=static_cast<eCVisualAnimation_PS*>(player->GetPropertySet(eEPropertySetType_Animation));
    if(animation)
    {
        auto& factory=animation->GetAnimationFactory();auto const& parts=factory.GetAdditionalBodyParts();
        if(parts.GetCount()<0 || parts.GetCount()>64)return false;
        if(!gStereo.headVisibilityReported)
        {
            Event event=Record(Kind::Message);sprintf_s(event.text,"player_visual main=%.60s facial=%.60s parts=%d",factory.GetMainActorFileName().GetText(),factory.GetFacialActorFileName().GetText(),parts.GetCount());Push(event,true);
        }
        for(int i=0;i<parts.GetCount();++i)
        {
            auto const& native=parts[i];if(!diagnostic::IsHeadBodyPart(native.m_strName.GetText()))continue;
            if(snapshot.partCount==4)return false;
            auto& part=snapshot.parts[snapshot.partCount++];part.factory=&factory;part.name=&native.m_strName;
            if(!snapshot.visibility.Remember(&part,native.m_bVisible!=GEFalse,SetHeadBodyPartVisibility))return false;
            if(!gStereo.headVisibilityReported)
            {
                Event event=Record(Kind::Message);sprintf_s(event.text,"player_head_part=%.48s native_visible=%d",native.m_strName.GetText(),int(native.m_bVisible));Push(event,true);
            }
        }
    }
    auto* inventory=static_cast<gCInventory_PS*>(player->GetPropertySet(eEPropertySetType_Inventory));
    if(inventory)
    {
        // Innate head/hair and equipped helmet/beard can be separate entities.
        // Include their children, but never touch the player or body equipment.
        constexpr gESlot slots[]={gESlot_Head,gESlot_Hair,gESlot_Beard,gESlot_Helmet};
        for(auto slot:slots)
        {
            auto* native=inventory->GetDefaultSlot(slot);
            if(native && !CollectHeadEntity(native->AccessItem().GetEntity(),player,snapshot))return false;
            if(!CollectHeadEntity(inventory->GetStackFromSlot(slot),player,snapshot))return false;
        }
    }
    return true;
}
void BeginPlayerHeadVisibility(HeadVisibilitySnapshot& snapshot)
{
    if(!gStereo.hidePlayerHead)return;
    bool collected=false;
    __try {collected=CollectPlayerHeadVisibility(snapshot);}
    __except(EXCEPTION_EXECUTE_HANDLER)
    {Message("first_person_head_visibility_collection_fault; hiding_disabled",GetExceptionCode(),true);}
    if(!collected){gHeadVisibilityFault=true;gConfig.hideFirstPersonHead=false;Message("first_person_head_visibility_refused; native_visibility_preserved",0,true);return;}
    if(!gStereo.headVisibilityReported)
    {
        Event event=Record(Kind::Message);sprintf_s(event.text,"first_person_head_visibility; body_parts=%u head_entities=%u; facial_LOD_included; per_render_restore",snapshot.partCount,snapshot.entityCount);Push(event,true);
        gStereo.headVisibilityReported=true;
    }
    snapshot.visibility.Hide();
}
bool MenuPanelActive();
void RequestCameraReset(char const* source);
void ApplyThirdPersonOrbit(diagnostic::Rotation3& rotation,float (&position)[3]);
void StereoCameraBase(CameraSnapshot const& saved,diagnostic::Rotation3& native,float (&basePosition)[3])
{
    if(gStereo.rendering&&gStereo.basePrepared){native=gStereo.frameBaseRotation;memcpy(basePosition,gStereo.frameBasePosition,sizeof(basePosition));return;}
    memcpy(basePosition,saved.position,sizeof(basePosition));native=diagnostic::NativeCameraRotation(saved.rotation);
    if((gStereo.dialog||gStereo.chest||gStereo.lectern)&&gDialogCamera.ApplyPlayer(gStereo.dialogPlayer,gStereo.playerPosition,native,basePosition))return;
    if(!gStereo.firstPerson){ApplyThirdPersonOrbit(native,basePosition);return;}
    if(gStereo.firstPerson){
        if(gVrSettings.stableHorizon){
            native=gStereo.holdPlayerHeading&&gStereo.playerHeadingValid?gStereo.lastPlayerRotation:diagnostic::UprightCamera(native);
            gStereo.heightAnchor.Position(gStereo.player,gStereo.playerPosition,basePosition,gTrackedArms.PostureNativeHeight());
            basePosition[1]+=gTrackedArms.PostureCameraHeight();
            float stable[3]{};
            if(gVrSettings.smoothCamera&&gStereo.cameraStabilizer.Apply(gStereo.player,gStereo.playerPosition,stable))
                for(unsigned i=0;i<3;++i)basePosition[i]+=stable[i]-gStereo.playerPosition[i];
        }
        diagnostic::ForwardCameraOffset(native,float(gVrSettings.numbers[6])*.01f*gConfig.unitsPerMeter,basePosition);
    }
}
float DialogCameraClearance(float const (&pivot)[3],float const (&eye)[3],eCEntity* player)
{
    auto& physics=eCPhysicsScene::GetInstance();
    float dx=eye[0]-pivot[0],dy=eye[1]-pivot[1],dz=eye[2]-pivot[2],distance=std::sqrt(dx*dx+dy*dy+dz*dz);
    if(!std::isfinite(distance)||distance<=0)return 0;
    float result=1,radius=.12f*gConfig.unitsPerMeter;
    // Five parallel rays reserve space for the head and both stereo eyes.
    // Static geometry only: nearby NPC motion must not reposition the shot.
    float offsets[5][3]{{},{-dz/distance*radius,0,dx/distance*radius},{dz/distance*radius,0,-dx/distance*radius},{0,radius,0},{0,-radius,0}};
    constexpr unsigned hints=eETraceRayHints_TestStaticShapes|eETraceRayHints_Distance;
    for(auto const& offset:offsets){
        if(!physics.TraceRayNearestHit(bCVector(pivot[0]+offset[0],pivot[1]+offset[1],pivot[2]+offset[2]),bCVector(dx,dy,dz),player,hints))continue;
        float hit=physics.GetQueryReport().m_NearestDistance;
        if(!std::isfinite(hit)||hit<0)return 0;
        result=std::min(result,std::max(0.f,(hit-.18f*gConfig.unitsPerMeter)/distance));
    }
    return result;
}
void ApplyThirdPersonOrbit(diagnostic::Rotation3& rotation,float (&position)[3]){
    if(gStereo.dialog||gStereo.chest||gStereo.lectern||!gVrSettings.thirdPersonOrbit||!gApplication||!gApplication->IsGameRunning())return;
    auto* player=gCSession::GetInstance().GetPlayer();if(!diagnostic::Readable(player,sizeof(eCDynamicEntity))||!gThirdPersonOrbit.Requested(reinterpret_cast<uintptr_t>(player)))return;
    // Free third-person locomotion must retain the native camera/heading link.
    // Only stationary activities and recovery own a render-only orbit.
    if(diagnostic::NativeAnimalForm(player)||!(CameraThirdPersonActivity(player)||diagnostic::NativeRecovery(player))){gThirdPersonOrbit.Reset();return;}
    auto root=player->GetWorldPosition();float pivot[3]{root.m_fX,root.m_fY+1.25f*gConfig.unitsPerMeter,root.m_fZ};
    if(std::hypot(position[0]-pivot[0],position[2]-pivot[2])<.65f*gConfig.unitsPerMeter){
        float origin[3];memcpy(origin,position,sizeof(origin));float actor[3]{root.m_fX,root.m_fY,root.m_fZ},headOffset[3]{};
        diagnostic::FrameDialogCamera(origin,actor,headOffset,rotation,gConfig.unitsPerMeter,[player](float const (&p)[3],float const (&e)[3]){return DialogCameraClearance(p,e,player);},position);
    }
    gThirdPersonOrbit.Apply(reinterpret_cast<uintptr_t>(player),pivot,rotation,position,gConfig.unitsPerMeter,
        [player](float const (&p)[3],float const (&e)[3]){return DialogCameraClearance(p,e,player);});
}
void PrepareComfortCameraBase(CameraSnapshot const& saved,diagnostic::HeadPose const&)
{
    bool wasDialogLocked=gDialogCamera.Active();
    if((gStereo.dialog||gStereo.chest||gStereo.lectern)&&gStereo.dialogPlayer){
        auto* player=reinterpret_cast<eCDynamicEntity*>(gStereo.dialogPlayer);
        diagnostic::Rotation3 facing=gStereo.lastPlayerRotation;
        if(!gStereo.playerHeadingValid){auto const& world=player->GetWorldMatrix();
            diagnostic::NativeCameraReturn::Facing(world(2,0),world(2,2),facing);}
        float height=1.65f*gConfig.unitsPerMeter;
        if(gStereo.heightAnchor.Calibrated(gStereo.dialogPlayer))gStereo.heightAnchor.Snapshot(height);
        gDialogCamera.CapturePlayer(gStereo.dialogPlayer,gStereo.playerPosition,facing,height,
            float(gVrSettings.numbers[6])*.01f*gConfig.unitsPerMeter,gConfig.unitsPerMeter);
    }else gDialogCamera.Release();
    if(!wasDialogLocked&&gDialogCamera.Active())Message(gStereo.lectern?"vr_lectern_camera_player_view; live_head_tracking; reading_logic_native":gStereo.chest?"vr_chest_camera_player_view; live_head_tracking; chest_logic_native":"vr_dialog_camera_player_view; native_shots_ignored; live_head_tracking; dialogue_logic_native",0,true);
    bool stabilized=gStereo.firstPerson&&gStereo.player&&gVrSettings.stableHorizon;
    if(stabilized&&gVrSettings.smoothCamera){
        LARGE_INTEGER now;QueryPerformanceCounter(&now);
        double seconds=gFrequency.QuadPart>0?double(now.QuadPart)/double(gFrequency.QuadPart):0;
        gStereo.cameraStabilizer.Update(gStereo.player,gStereo.playerPosition,seconds,gConfig.unitsPerMeter,gVrSettings.cameraSmoothingMs,gVrSettings.cameraVerticalSmoothingMs);
    }else gStereo.cameraStabilizer.Reset();
    gStereo.basePrepared=false;StereoCameraBase(saved,gStereo.frameBaseRotation,gStereo.frameBasePosition);gStereo.basePrepared=true;
    if(!gStereo.dialog&&!gStereo.chest&&!gStereo.lectern){
        if(gStereo.firstPerson&&gStereo.player)gDialogCamera.RememberPlayer(gStereo.player,gStereo.frameBaseRotation,gStereo.frameBasePosition,gStereo.playerPosition,gConfig.unitsPerMeter);
        else gDialogCamera.Reset();
    }
    if(stabilized&&!gStereo.holdPlayerHeading){gStereo.lastPlayerRotation=gStereo.frameBaseRotation;gStereo.playerHeadingValid=true;}
    if(stabilized&&GetTickCount64()>=gStereo.comfortNextLog){gStereo.comfortNextLog=GetTickCount64()+500;
        float stable[3]{};memcpy(stable,gStereo.playerPosition,sizeof(stable));gStereo.cameraStabilizer.Apply(gStereo.player,gStereo.playerPosition,stable);
        Event event=Record(Kind::Message);sprintf_s(event.text,"vr_camera_comfort correction_cm=%.2f,%.2f,%.2f smooth=%d hold_heading=%d same_eye_hand_base=1",stable[0]-gStereo.playerPosition[0],stable[1]-gStereo.playerPosition[1],stable[2]-gStereo.playerPosition[2],gVrSettings.smoothCamera,gStereo.holdPlayerHeading);Push(event,true);
    }
}
bool BuildControllerWorldAim(CameraSnapshot const& saved,diagnostic::WorldAim& aim,bool interaction=true,diagnostic::ControllerFocusQuery const* query=nullptr)
{
    DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    bool allowed=!gWorldInteraction.fault&&gFindFocusHook.original&&gFocusAnglesHook.original&&gVrSettings.controllers&&
        foreground==GetCurrentProcessId()&&GetCurrentThreadId()==gApplicationThread.load()&&gHeadControl.enabled&&gHeadControl.centered&&
        gConfig.stereoEnabled&&gStereoRuntime.Connected()&&!gStereo.shuttingDown&&!gStereo.fault&&gStereo.firstPerson&&gApplication&&gApplication->IsGameRunning();
    if(!allowed)return false;
    bool bowAim=false;
    if(!interaction){
        if(query)bowAim=query->weaponAim;
        else {auto* player=gCSession::GetInstance().GetPlayer();bowAim=gBow.Weapon(player)!=nullptr||gMagic.Selected(player)!=nullptr;}
    }
    auto frame=MappedControllers();auto now=GetTickCount64();
    if(bowAim){
        // This sample already contains the calibrated world-space weapon ray.
        // Reuse the caller's weapon classification and do not rebuild the
        // hero/camera anchor just to discard it for a bow/crossbow/spell.
        auto sample=gMagic.aim.sample.valid?gMagic.aim.sample:gTrackedArms.BowSample();unsigned hand=sample.aimHand;
        if(hand>1||now<sample.time||now-sample.time>100||MenuPanelActive()||gRadialOpen||
           !diagnostic::arms::FreshGrip(frame,hand,now)||!frame.hands[hand].aim.valid)return false;
        return diagnostic::bow::FocusAim(sample,aim);
    }
    if(!diagnostic::WorldAimReady(frame,now,allowed,MenuPanelActive()))return false;
    // Native movement temporarily rotates the camera towards the headset.
    // Interaction must use the original base, so headset yaw is not added twice.
    CameraSnapshot base=saved;
    if(gStereo.rendering){memcpy(base.position,gStereo.nativePosition,sizeof(base.position));memcpy(base.rotation,gStereo.nativeRotation,sizeof(base.rotation));}
    else if(gHeadControl.active)memcpy(base.rotation,gHeadControl.nativeRotation,sizeof(base.rotation));
    if(gVrSettings.stableHorizon){auto* player=gCSession::GetInstance().GetPlayer();if(!diagnostic::Readable(player,sizeof(eCDynamicEntity)))return false;gStereo.player=reinterpret_cast<uintptr_t>(player);auto root=player->GetWorldPosition();gStereo.playerPosition[0]=root.m_fX;gStereo.playerPosition[1]=root.m_fY;gStereo.playerPosition[2]=root.m_fZ;}
    float position[3];diagnostic::Rotation3 native;StereoCameraBase(base,native,position);
    position[1]+=gStereo.heightOffsetUnits;
    return diagnostic::ControllerWorldAim(native,position,gHeadControl.center,gStereo.centerPosition,frame.hands[1].aim,gConfig.unitsPerMeter,aim)&&
        (!interaction||diagnostic::WorldAimElevationBias(aim,gVrSettings.interactionAimUpPercent));
}
bool PlayerFocus(gCFocus_PS const* focus)
{
    if(GetCurrentThreadId()!=gApplicationThread.load()||!gApplication||!gApplication->IsGameRunning())return false;
    auto* player=gCSession::GetInstance().GetPlayer();
    return gPlayerFocusDispatch.Matches(reinterpret_cast<uintptr_t>(focus),reinterpret_cast<uintptr_t>(player),gProcess.load(std::memory_order_relaxed),[&](){
        return diagnostic::Readable(player,sizeof(eCDynamicEntity))?reinterpret_cast<uintptr_t>(player->GetPropertySet(eEPropertySetType_Focus)):uintptr_t(0);});
}
void __fastcall ControllerFocusAngles(gCFocus_PS const* self,void*,bCVector const& point,float& azi,float& elev)
{
    using Angles=void(__thiscall*)(gCFocus_PS const*,bCVector const&,float&,float&);
    bool overridden=false;
    __try {
        diagnostic::WorldAim aim;
        bool ready=gWorldInteraction.scope&&gWorldInteraction.focus==self;
        if(ready)aim=gWorldInteraction.aim;
        else if(!gWorldInteraction.fault&&PlayerFocus(self)){CameraSnapshot saved;SaveStereoCamera(gApplication->GetCurrentCamera(),saved);ready=BuildControllerWorldAim(saved,aim,false);}
        float target[3]={point.m_fX,point.m_fY,point.m_fZ};
        overridden=ready&&diagnostic::WorldAimAngles(aim,target,azi,elev);if(overridden)++gWorldInteraction.angles;
    } __except(EXCEPTION_EXECUTE_HANDLER){gWorldInteraction.fault=true;Message("controller_focus_angles_fault; native_focus_restored",GetExceptionCode(),true);}
    if(!overridden)reinterpret_cast<Angles>(gFocusAnglesHook.original)(self,point,azi,elev);
}
class FocusAccess : public gCFocus_PS {
public: using gCFocus_PS::GetCurrentPriority;
};
bool ControllerTargetRating(eCEntity* entity,diagnostic::WorldAim const& aim,float& rating,bool ranked=false)
{
    if(!diagnostic::Readable(entity,sizeof(eCEntity)))return false;
    auto const& bounds=entity->GetWorldNodeBoundary();
    float lo[3]{bounds.m_Min.m_fX,bounds.m_Min.m_fY,bounds.m_Min.m_fZ},hi[3]{bounds.m_Max.m_fX,bounds.m_Max.m_fY,bounds.m_Max.m_fZ};
    bool pickup=entity->HasPropertySet(eEPropertySetType_Item);
    float reach=float(pickup?gVrSettings.pickupReachCm:gVrSettings.interactionReachCm)*.01f*gConfig.unitsPerMeter;
    auto const& origin=entity->GetWorldPosition();float root[3]{origin.m_fX,origin.m_fY,origin.m_fZ};
    bool hit=ranked?diagnostic::RankedInteractionBox(aim,int(gCEntity::GetUseType(entity)),root,gConfig.unitsPerMeter,lo,hi,reach,rating,pickup):
        diagnostic::InteractionAimBox(aim,int(gCEntity::GetUseType(entity)),root,gConfig.unitsPerMeter,lo,hi,reach,rating,pickup);
    if(hit&&pickup&&reinterpret_cast<uintptr_t>(entity)==gWorldInteraction.selected)rating=std::max(0.f,rating-(ranked?.015f:.06f));
    return hit;
}
void RememberRangedFocusLabel(eCEntity* selected,bool ranged,diagnostic::WorldAim const& aim){
    if(gRangedLabelFault){gRangedFocusLabel.Reset();return;}
    if(!ranged||!selected){gRangedFocusLabel.InteractionFocus(reinterpret_cast<uintptr_t>(selected));return;}
    __try {
        if(!diagnostic::Readable(selected,sizeof(eCEntity))||!selected->HasPropertySet(eEPropertySetType_NPC)){gRangedFocusLabel.Reset();return;}
        auto const& box=selected->GetWorldNodeBoundary();float lo[3]{box.m_Min.m_fX,box.m_Min.m_fY,box.m_Min.m_fZ},hi[3]{box.m_Max.m_fX,box.m_Max.m_fY,box.m_Max.m_fZ};
        gRangedFocusLabel.Capture(reinterpret_cast<uintptr_t>(selected),gProcess.load(std::memory_order_relaxed),GetTickCount64(),true,true,aim.origin,lo,hi,gConfig.unitsPerMeter,float(gVrSettings.interactionReachCm)*.01f*gConfig.unitsPerMeter);
    }__except(EXCEPTION_EXECUTE_HANDLER){gRangedLabelFault=true;gRangedFocusLabel.Reset();Message("vr_ranged_label_size_disabled; native_font_preserved",GetExceptionCode(),true);}
}
bool __fastcall ControllerFocusCheck(gCFocus_PS* self,void*,eCEntity* entity,eCEntity* previous,float best,float& rating)
{
    using Check=bool(__thiscall*)(gCFocus_PS*,eCEntity*,eCEntity*,float,float&);
    if(!gWorldInteraction.scope||gWorldInteraction.crossHair||gWorldInteraction.focus!=self||gWorldInteraction.fault)
        return reinterpret_cast<Check>(gFocusCheckHook.original)(self,entity,previous,best,rating);
    // Keep the complete pre-0.23.72 path (rating, priority, LOS and native
    // comparison) for characters. Independently rank only object candidates.
    __try {
        ++gWorldInteraction.surfaces;
        bool pickup=entity&&entity->HasPropertySet(eEPropertySetType_Item);if(pickup)++gWorldInteraction.pickups;
        if(!ControllerTargetRating(entity,gWorldInteraction.aim,rating))return false;
        if(pickup)++gWorldInteraction.pickupHits;
        if(rating<.05f)++gWorldInteraction.contacts;
        auto* focus=static_cast<FocusAccess*>(self);float priority=focus->GetCurrentPriority(entity);
        if(priority<0)return false;
        bool legacyBetter=!(previous&&priority==focus->GetCurrentPriority(previous)&&rating>=std::fabs(best));
        float objectRating=0;
        bool objectBetter=!gCEntity::IsCharacter(entity)&&ControllerTargetRating(entity,gWorldInteraction.aim,objectRating,true)&&gWorldInteraction.ranking.Better(objectRating);
        if(!legacyBetter&&!objectBetter)return false;
        auto* player=gCSession::GetInstance().GetPlayer();
        bool visible=diagnostic::Readable(player,sizeof(eCDynamicEntity))&&entity->IsFreeLOS(player,GEFalse);
        if(pickup){if(!visible)++gWorldInteraction.pickupBlocked;
            if(GetTickCount64()>=gWorldInteraction.pickupNextLog){gWorldInteraction.pickupNextLog=GetTickCount64()+750;Event event=Record(Kind::Message);
                sprintf_s(event.text,"vr_pickup_focus entity=%08lx reach_cm=%d visible=%d candidates=%llu hits=%llu blocked=%llu",DWORD(reinterpret_cast<uintptr_t>(entity)),gVrSettings.pickupReachCm,visible,gWorldInteraction.pickups,gWorldInteraction.pickupHits,gWorldInteraction.pickupBlocked);Push(event,true);}}
        if(visible&&objectBetter)gWorldInteraction.ranking.Accept(reinterpret_cast<uintptr_t>(entity),objectRating);
        return visible&&legacyBetter;
    } __except(EXCEPTION_EXECUTE_HANDLER){gWorldInteraction.fault=true;Message("controller_contact_focus_fault; native_focus_restored",GetExceptionCode(),true);}
    return reinterpret_cast<Check>(gFocusCheckHook.original)(self,entity,previous,best,rating);
}
eCEntity* __fastcall ControllerFindFocus(gCFocus_PS* self,void*,bool crossHair)
{
    using Find=eCEntity*(__thiscall*)(gCFocus_PS*,bool);auto original=reinterpret_cast<Find>(gFindFocusHook.original);
    if(gWorldInteraction.scope||gWorldInteraction.fault)return original(self,crossHair);
    CameraSnapshot saved;diagnostic::WorldAim aim;bool ready=false;auto query=diagnostic::PlanControllerFocus(crossHair,false,false);
    __try {if(PlayerFocus(self)){auto* player=gCSession::GetInstance().GetPlayer();bool ranged=gBow.Weapon(player)!=nullptr;query=diagnostic::PlanControllerFocus(crossHair,ranged,!ranged&&gMagic.Selected(player)!=nullptr);SaveStereoCamera(gApplication->GetCurrentCamera(),saved);ready=BuildControllerWorldAim(saved,aim,!query.rangedAim,&query);}}
    __except(EXCEPTION_EXECUTE_HANDLER){gWorldInteraction.fault=true;Message("controller_focus_prepare_fault; native_focus_preserved",GetExceptionCode(),true);}
    if(!ready)return original(self,crossHair);
    bool rangedAim=query.rangedAim;
    uint64_t focusStarted=diagnostic::probes::Now();
    auto& camera=gApplication->GetCurrentCamera();eCEntity* selected=nullptr;
    gWorldInteraction.scope=true;gWorldInteraction.crossHair=rangedAim;gWorldInteraction.focus=self;gWorldInteraction.aim=aim;
    gWorldInteraction.ranking.Reset();
    bool rangeRating=self->GetEnableRangeRating();
    auto focusMode=self->GetCurrentMode().GetValue();
    // FindFocusEntity's ranged/crosshair branch reads this exact camera's
    // position and view inverse. Only this native query sees the controller.
    __try {
        camera.SetPosition(bCVector(aim.origin[0],aim.origin[1],aim.origin[2]));camera.SetRotation(bCVector(aim.rotation[0],aim.rotation[1],aim.rotation[2]));camera.UpdateViewport(GETrue);
        if(!rangedAim)self->SetEnableRangeRating(GEFalse);
        // Keep automatic ranged focus on the native candidate path as well.
        // Native Shape/crosshair tracing can return hits without distances;
        // FindFocusEntity then dereferences that absent report at RVA 6E02F.
        if((!rangedAim||query.candidateQuery)&&focusMode==gEFocus_Shape)self->AccessCurrentMode().SetValue(gEFocus_Hard);
        ++gWorldInteraction.calls;selected=original(self,query.nativeCrossHair);
        if(!rangedAim&&!gWorldInteraction.fault&&gWorldInteraction.ranking.Selected()){
            bool character=selected&&gCEntity::IsCharacter(selected);
            selected=reinterpret_cast<eCEntity*>(diagnostic::FinalInteractionFocus(reinterpret_cast<uintptr_t>(selected),character,gWorldInteraction.ranking.Selected()));self->SetFocusEntity(selected);
        }
        // Locked-focus and ranged-mode branches bypass FocusCheck. Apply the
        // same reach gate to interaction, without shortening bow/spell aim.
        if(!rangedAim&&selected){float rating=0;if(!ControllerTargetRating(selected,aim,rating)){selected=nullptr;self->SetFocusEntity(nullptr);}}
    } __finally {
        self->SetEnableRangeRating(rangeRating);
        self->AccessCurrentMode().SetValue(focusMode);
        camera.SetPosition(bCVector(saved.position[0],saved.position[1],saved.position[2]));camera.SetRotation(bCVector(saved.rotation[0],saved.rotation[1],saved.rotation[2]));camera.UpdateViewport(GETrue);
        bCMatrix view;for(int row=0;row<4;++row)for(int col=0;col<4;++col)view(row,col)=saved.view[row*4+col];camera.SetViewMatrix(view);camera.UpdateViewport(GEFalse);
        gWorldInteraction.scope=false;gWorldInteraction.focus=nullptr;++gWorldInteraction.restores;
    }
    if(!rangedAim)gWorldInteraction.selected=reinterpret_cast<uintptr_t>(selected);
    if(!crossHair)RememberRangedFocusLabel(selected,query.candidateQuery,aim);
    gStereoRuntime.NoteCpuUpdate(diagnostic::performance::UpdateStage::Focus,focusStarted);
    if(gSceneProfileEnabled&&GetCurrentThreadId()==gApplicationThread.load())gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::Focus,focusStarted,MenuPanelActive()?2:1);
    if(diagnostic::probes::Enabled&&GetTickCount64()>=gWorldInteraction.nextLog){gWorldInteraction.nextLog=GetTickCount64()+1000;Event event=Record(Kind::Message);sprintf_s(event.text,"controller_world_focus target=%08lx ray=%d surfaces=%llu reach_cm=%d up_percent=%d calls=%llu restores=%llu",DWORD(gWorldInteraction.selected),crossHair,gWorldInteraction.surfaces,gVrSettings.interactionReachCm,rangedAim?0:gVrSettings.interactionAimUpPercent,gWorldInteraction.calls,gWorldInteraction.restores);Push(event,true);}
    return selected;
}
bool __fastcall RangedFocusName(gCGUIEntityRenderer* self,void*,eCEntity const* entity,float alpha){
    using Call=bool(__thiscall*)(gCGUIEntityRenderer*,eCEntity const*,float);auto original=reinterpret_cast<Call>(gFocusNameSizeHook.original);
    bool allowed=!gRangedLabelFault&&!gRangedLabelDrawing&&GetCurrentThreadId()==gApplicationThread.load()&&
        !gWorldLifecycle.Blocked()&&!gStereo.shuttingDown&&gConfig.stereoEnabled&&gHeadControl.enabled;
    if(!allowed||gRadialOpen)return original(self,entity,alpha);
    int percent=gVrSettings.numbers[86];
    if(gStereo.firstPerson&&gRangedFocusLabel.IsTarget(reinterpret_cast<uintptr_t>(entity))&&
       gRangedFocusLabel.Applies(reinterpret_cast<uintptr_t>(entity),gProcess.load(std::memory_order_relaxed),GetTickCount64(),true))percent=(percent+1)/2;
    if(percent==100)return original(self,entity,alpha);
    auto saved=self->m_FontIndex;int smaller=-1;
    __try {
        auto epoch=gProcess.load(std::memory_order_relaxed)+gRender.load(std::memory_order_relaxed);
        smaller=gWorldLabelFrameCache.Get(epoch,saved,percent,[](){return !MenuPanelActive();},
            [](unsigned font,int scale){return gRangedLabelFont.Resolve(font,scale);});
    }
    __except(EXCEPTION_EXECUTE_HANDLER){gRangedLabelFault=true;Message("vr_ranged_label_font_unavailable; native_font_preserved",GetExceptionCode(),true);}
    if(smaller<0)return original(self,entity,alpha);
    __try {gRangedLabelDrawing=true;self->m_FontIndex=GEU32(smaller);return original(self,entity,alpha);}
    __finally {self->m_FontIndex=saved;gRangedLabelDrawing=false;}
}
bool __fastcall SignpostNameFocus(gCGUIEntityRenderer const* self,void*,eCEntity const* entity)
{
    using Call=bool(__thiscall*)(gCGUIEntityRenderer const*,eCEntity const*);
    if(reinterpret_cast<Call>(gSignpostFocusHook.original)(self,entity))return true;
    if(!entity||GetCurrentThreadId()!=gApplicationThread.load()||gWorldLifecycle.Blocked()||gStereo.shuttingDown||!gHeadControl.enabled||!gConfig.stereoEnabled||
       !gStereo.firstPerson||!gHeadControl.centered||!gApplication)return false;
    __try {
        // DrawFocusName owns a live native entity for this synchronous call.
        // Do not VirtualQuery it, its string and the hero's menu/routine for
        // every NPC/item. Keep the native predicate above and SEH protection;
        // only actual Sign_ entities need the extension's expensive checks.
        return diagnostic::ExtraSignpostFocus(entity->GetName().GetText(),[&](){
        if(!gApplication->IsGameRunning()||MenuPanelActive())return false;
        auto* sign=const_cast<eCEntity*>(entity);
        if(!diagnostic::Readable(sign,sizeof(eCEntity)))return false;
        diagnostic::HeadPose head;if(!TryAcquireSRWLockShared(&gHeadPoseLock))return false;head=gHeadPose;ReleaseSRWLockShared(&gHeadPoseLock);
        LARGE_INTEGER ticks;QueryPerformanceCounter(&ticks);if(!diagnostic::FreshOrientation(head,ticks.QuadPart,gFrequency.QuadPart,100))return false;
        diagnostic::StereoEyePose pose{};memcpy(pose.position,head.position,sizeof(pose.position));memcpy(pose.quaternion,head.quaternion,sizeof(pose.quaternion));
        float origin[3]{},angles[3]{};if(!diagnostic::GameEyeTransform(gStereo.frameBaseRotation,gStereo.frameBasePosition,gHeadControl.center,gStereo.centerPosition,pose,gConfig.unitsPerMeter,origin,angles))return false;
        origin[1]+=gStereo.heightOffsetUnits;auto rotation=diagnostic::NativeCameraRotation(angles);float forward[3]{rotation.m[2],rotation.m[5],rotation.m[8]};
        auto const& box=sign->GetWorldNodeBoundary();float point[3]{(box.m_Min.m_fX+box.m_Max.m_fX)*.5f,(box.m_Min.m_fY+box.m_Max.m_fY)*.5f,(box.m_Min.m_fZ+box.m_Max.m_fZ)*.5f};
        return diagnostic::SignpostInView(origin,forward,point,gConfig.unitsPerMeter);
        });
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
int __fastcall NativeHudWindowProc(CFFGFCWnd* self,void*,unsigned message,unsigned w,unsigned l)
{
    bool hidden=false;
    if(diagnostic::HudPaintMessage(message)&&gConfig.stereoEnabled&&gHeadControl.enabled&&
       gStereoRuntime.Connected()&&!gStereo.shuttingDown&&!gStereo.fault){
        __try {
            static diagnostic::HudInventoryFrameCache inventory;
            auto epoch=gProcess.load(std::memory_order_relaxed)+gRender.load(std::memory_order_relaxed);
            bool inventoryOpen=inventory.Get(epoch,[](){return NativeUiState()==int(gEGamePage_Inventory);});
            auto show=diagnostic::EffectiveHudVisibility({!gVrSettings.hideCompass,gVrSettings.showSlotBar,gVrSettings.showVitalsPanel},inventoryOpen);
            hidden=gNativeHudVisibility.Hidden(self,show);
        }
        __except(EXCEPTION_EXECUTE_HANDLER){hidden=false;}
    }
    return hidden?0:gNativeHudVisibility.Dispatch(self,message,w,l);
}
bool InstallControllerFocus()
{
    auto game=GetModuleHandleW(L"Game.dll");wchar_t path[MAX_PATH];unsigned char digest[32]{};
    constexpr unsigned char expected[32]={0x47,0x28,0xa4,0x7b,0x24,0x4a,0xdc,0xb0,0xea,0x2e,0xc3,0x50,0x1d,0xb0,0x49,0x94,0x90,0x9d,0xcf,0xcf,0xf3,0x38,0xc7,0xb8,0x07,0x5f,0x2b,0x68,0xdd,0x8d,0x6e,0x00};
    if(!game||!GetModuleFileNameW(game,path,MAX_PATH)||!diagnostic::FileSha256(path,digest)||memcmp(digest,expected,32))return false;
    auto* drawName=GetProcAddress(game,"?DrawFocusName@gCGUIEntityRenderer@@IAE_NPBVeCEntity@@M@Z");
    if(reinterpret_cast<unsigned char*>(drawName)==reinterpret_cast<unsigned char*>(game)+0x79cb0&&
        gFocusNameSizeHook.Install(drawName,reinterpret_cast<void*>(&RangedFocusName),diagnostic::FocusHook::FindPrefix,6))Message("vr_world_label_scale_ready; context_and_fonts_cached_per_frame; native_name_filter_preserved",0,true);
    else Message("vr_ranged_focus_name_size_unavailable; native_font_preserved",0,true);
    auto* signFocus=GetProcAddress(game,"?IsFocus@gCGUIEntityRenderer@@IBE_NPBVeCEntity@@@Z");
    unsigned char signPrefix[]{0x56,0x8b,0x74,0x24,0x08};
    if(reinterpret_cast<unsigned char*>(signFocus)==reinterpret_cast<unsigned char*>(game)+0x7a040&&
       gSignpostFocusHook.Install(signFocus,reinterpret_cast<void*>(&SignpostNameFocus),signPrefix,sizeof(signPrefix)))Message("vr_native_signpost_labels_ready; native_localization; head_view_cone",0,true);
    else Message("vr_native_signpost_labels_unavailable; native_predicate_preserved",0,true);
    auto find=GetProcAddress(game,"?FindFocusEntity@gCFocus_PS@@QAEPAVeCEntity@@_N@Z"),angles=GetProcAddress(game,"?GetAngles@gCFocus_PS@@IBEXABVbCVector@@AAM1@Z");
    auto check=GetProcAddress(game,"?FocusCheck@gCFocus_PS@@IAE_NPAVeCEntity@@0MAAM@Z");
    if(!diagnostic::Readable(find,6)||memcmp(find,diagnostic::FocusHook::FindPrefix,6)||!diagnostic::Readable(angles,5)||memcmp(angles,diagnostic::FocusHook::AnglesPrefix,5)||!diagnostic::Readable(check,6)||memcmp(check,diagnostic::FocusHook::FindPrefix,6))return false;
    if(!gFocusCheckHook.Install(check,reinterpret_cast<void*>(&ControllerFocusCheck),diagnostic::FocusHook::FindPrefix,6)||
        !gFocusAnglesHook.Install(angles,reinterpret_cast<void*>(&ControllerFocusAngles),diagnostic::FocusHook::AnglesPrefix,5)||
        !gFindFocusHook.Install(find,reinterpret_cast<void*>(&ControllerFindFocus),diagnostic::FocusHook::FindPrefix,6))return false;
    Message("controller_world_focus_hooks_installed; hand_surface_reach; close_contact; native_filters_LOS; ranged_aim_preserved",0,true);return true;
}
bool SetStereoEye(eCCameraBase& camera,CameraSnapshot const& saved,diagnostic::StereoEyePose const& eye,unsigned index)
{
    float position[3]{},rotation[3]{},basePosition[3];diagnostic::Rotation3 native;StereoCameraBase(saved,native,basePosition);
    if(!diagnostic::GameEyeTransform(native,basePosition,gHeadControl.center,gStereo.centerPosition,eye,gConfig.unitsPerMeter,position,rotation))return false;
    // Apply an equal world-Y shift to both eyes, after the tracked pose. Camera
    // pitch must not turn this height adjustment into a forward displacement.
    position[1]+=gStereo.heightOffsetUnits;
    auto const& clip=camera.GetClipDepth();float nearZ=clip.GetX(),farZ=clip.GetY();if(!std::isfinite(nearZ)||!std::isfinite(farZ)||nearZ<=0||farZ<=nearZ)return false;
    // The native first-person camera forces its own near plane. Override
    // ClipDepth for both VR eyes so projection updates use the same range.
    // RenderLowPoly still supplies its own depth range in the projection hook.
    if(gStereo.firstPerson){
        float firstPersonNear=gConfig.unitsPerMeter*.01f*gVrSettings.numbers[diagnostic::FirstPersonNearClipIndex];
        if(firstPersonNear<farZ)nearZ=firstPersonNear;
    }
    diagnostic::StereoProjection(eye.fov,nearZ,farZ,gStereo.projection);
    if(!gStereoRuntime.ApplyTemporalProjection(index,gStereo.projection))return false;
    gStereo.detailFar=farZ;
    if(clip.GetX()!=nearZ)camera.SetClipDepth(bCVector2(nearZ,farZ));
    camera.SetPosition(bCVector(position[0],position[1],position[2]));camera.SetRotation(bCVector(rotation[0],rotation[1],rotation[2]));camera.UpdateViewport(GETrue);
    StereoProjectionUpdate(&camera,nullptr,true);camera.ExecuteGfxCamera();
    static float reportedNear=0;
    float effectiveNear=camera.GetClipDepth().GetX();
    if(index==0&&std::fabs(effectiveNear-reportedNear)>.001f&&gApplication&&gApplication->IsGameRunning()){
        reportedNear=effectiveNear;char text[160];
        sprintf_s(text,"vr_detail_projection_depth near_cm=%.3f native_near_cm=%.3f far_cm=%.1f",100.f*effectiveNear/gConfig.unitsPerMeter,100.f*saved.clipDepth[0]/gConfig.unitsPerMeter,100.f*farZ/gConfig.unitsPerMeter);
        Message(text,0,true);
    }
    for(int row=0;row<4;++row)for(int col=0;col<4;++col)if(std::fabs(camera.GetProjectionMatrix()(row,col)-gStereo.projection[row*4+col])>.0001f)return false;
    return diagnostic::RotationError(diagnostic::NativeCameraRotation(rotation),CameraViewRotation(camera))<.001f;
}
bool KeepFirstPersonPermission(bCString const& name,eCEntity* actor)
{
    if(!gConfig.keepFirstPerson||!gConfig.stereoEnabled||!gHeadControl.enabled||!gStereoRuntime.Connected()||
        gStereo.shuttingDown||gStereo.fault||GetCurrentThreadId()!=gApplicationThread.load())return false;
    __try
    {
        if(!diagnostic::IsFirstPersonPermission(name.GetText())||!actor||!gApplication||!gApplication->IsGameRunning()||
            actor!=gCSession::GetInstance().GetPlayer()||CameraThirdPersonActivity(actor)||diagnostic::NativeAnimalForm(actor))return false;
        ++gCameraPermissionOverrides;
        if(GetTickCount64()>=gCameraPermissionNextLog)
        {
            gCameraPermissionNextLog=GetTickCount64()+1000;
            Event event=Record(Kind::Message);sprintf_s(event.text,"vr_first_person_permission_retained; native_refusals=%llu",gCameraPermissionOverrides);Push(event,true);
        }
        return true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        gFirstPersonPermissionFault=true;gConfig.keepFirstPerson=false;Message("first_person_permission_check_fault; native_permission_restored",GetExceptionCode(),true);return false;
    }
}
GEInt __fastcall CameraPermissionCallScript(gCScriptAdmin* admin,void*,bCString const& name,eCEntity* actor,eCEntity* other,GEInt args)
{
    using Call=GEInt(__thiscall*)(gCScriptAdmin*,bCString const&,eCEntity*,eCEntity*,GEInt);
    return diagnostic::CallCameraScript(reinterpret_cast<Call>(gCameraPermissionHook.original),admin,name,actor,other,args,KeepFirstPersonPermission);
}
bool InstallFirstPersonLock()
{
    if(!gConfig.keepFirstPerson||gCameraPermissionHook.installed)return true;
    auto game=GetModuleHandleW(L"Game.dll");wchar_t path[MAX_PATH];unsigned char digest[32]{};
    constexpr unsigned char expectedHash[32]={
        0x47,0x28,0xa4,0x7b,0x24,0x4a,0xdc,0xb0,0xea,0x2e,0xc3,0x50,0x1d,0xb0,0x49,0x94,
        0x90,0x9d,0xcf,0xcf,0xf3,0x38,0xc7,0xb8,0x07,0x5f,0x2b,0x68,0xdd,0x8d,0x6e,0x00};
    if(!game||!GetModuleFileNameW(game,path,MAX_PATH)||!diagnostic::FileSha256(path,digest)||memcmp(digest,expectedHash,32))return false;
    // Install on the actual loaded script admin after a save is loaded. This
    // preserves an existing dispatcher detour by chaining the original slot.
    auto* admin=&GetScriptAdmin();if(!diagnostic::Readable(admin,sizeof(gCScriptAdmin)))return false;
    auto table=*reinterpret_cast<void***>(admin);size_t slot=0;
    auto expected=GetProcAddress(game,"?CallScript@gCScriptAdmin@@UAEHABVbCString@@PAVeCEntity@@1H@Z");
    if(!expected||!diagnostic::VirtualSlot(&gCScriptAdmin::CallScript,slot)||slot!=47||!diagnostic::Readable(table+slot,sizeof(void*))||
        diagnostic::ResolveJumpChain(table[slot])!=diagnostic::ResolveJumpChain(expected)||
        !gCameraPermissionHook.Prepare(table,slot,reinterpret_cast<void*>(&CameraPermissionCallScript))||!gCameraPermissionHook.Install())return false;
    Message("first_person_camera_permission_hook_installed; player_only; VR_only; native_script_chained",0,true);return true;
}
bool ReturnFirstPersonFacing(gCCameraAI_PS* camera,eCEntity* player)
{
    if(!gCameraPermissionHook.installed||!diagnostic::Readable(camera,sizeof(*camera))||camera->IsDialogCameraActive()||
       !diagnostic::Readable(player,sizeof(eCDynamicEntity)))return false;
    size_t targetSlot=0,modeSlot=0;auto** table=*reinterpret_cast<void***>(camera);
    auto game=GetModuleHandleW(L"Game.dll");
    auto target=GetProcAddress(game,"?SetTarget@gCCameraAI_PS@@UAEXPAVeCEntity@@@Z");
    auto mode=GetProcAddress(game,"?SetMode@gCCameraAI_PS@@UAEXW4gECameraMode@@_N1@Z");
    if(!target||!mode||!diagnostic::VirtualSlot(&gCCameraAI_PS::SetTarget,targetSlot)||targetSlot!=85||
       !diagnostic::VirtualSlot(&gCCameraAI_PS::SetMode,modeSlot)||modeSlot!=88||
       !diagnostic::Readable(table+modeSlot,sizeof(void*))||
       diagnostic::ResolveJumpChain(table[targetSlot])!=diagnostic::ResolveJumpChain(target)||
       diagnostic::ResolveJumpChain(table[modeSlot])!=diagnostic::ResolveJumpChain(mode))return false;
    RestoreCurrentBodyFacing();
    auto const& actor=player->GetWorldMatrix();diagnostic::Rotation3 facing;
    if(!diagnostic::NativeCameraReturn::Facing(actor(2,0),actor(2,2),facing))return false;
    if(!diagnostic::AnimalCamera::Release(camera,sizeof(*camera)))return false;
    camera->SetMode(static_cast<gECameraMode>(0),GETrue,GEFalse);
    camera->SetTarget(player);
    camera->SetMode(static_cast<gECameraMode>(11),GETrue,GEFalse);
    if(!camera->IsInFirstPerson()||!diagnostic::NativeCameraReturn::Seed(camera,sizeof(*camera),facing))return false;
    float yaw=0;diagnostic::HorizontalHeading(facing,yaw);
    auto& view=gApplication->GetCurrentCamera();view.SetRotation(bCVector(0,yaw,0));view.UpdateViewport(GETrue);
    gStereo.lastPlayerRotation=facing;gStereo.playerHeadingValid=true;gStereo.basePrepared=false;
    gInstantTurnCamera=gLastControllerTurnCamera=nullptr;
    RequestCameraReset("native_third_person_return_actor_facing");
    gAutoFirstPersonProcess=gProcess.load(std::memory_order_relaxed);gAutoFirstPersonNext=GetTickCount64()+250;
    Event event=Record(Kind::Message);
    sprintf_s(event.text,"vr_native_return_actor_facing; camera_history_reseeded; actor_forward=%.3f,%.3f view_forward=%.3f,%.3f; actor_unchanged",actor(2,0),actor(2,2),facing.m[2],facing.m[8]);Push(event,true);
    return true;
}
void AutoFirstPerson()
{
    if(gCinemaScope.Active(GetCurrentThreadId()))return;
    if(!gConfig.keepFirstPerson||!gConfig.stereoEnabled||!gHeadControl.enabled||gStereo.shuttingDown||gStereo.fault||
       !gStereoRuntime.Connected()||!gApplication||!gApplication->IsGameRunning()){gCameraReturn.Reset();return;}
    if(!gFirstPersonLockAttempted){gFirstPersonLockAttempted=true;if(!InstallFirstPersonLock()){
        gFirstPersonPermissionFault=true;gConfig.keepFirstPerson=false;Message("auto_first_person_profile_refused",0,true);return;}}
    if(!gCameraPermissionHook.installed)return; // This also verifies the exact Game.dll profile.
    auto* camera=gCSession::GetInstance().GetCamera_PS();
    auto* player=gCSession::GetInstance().GetPlayer();bool animal=diagnostic::NativeAnimalForm(player);
    bool activity=!animal&&CameraThirdPersonActivity(player);
    bool dialog=diagnostic::Readable(camera,sizeof(*camera))&&camera->IsDialogCameraActive();
    bool nativeReturn=gCameraReturn.Observe(animal?0:reinterpret_cast<uintptr_t>(player),reinterpret_cast<uintptr_t>(camera),activity||dialog);
    bool returning=(!animal&&gAnimalCameraActive)||nativeReturn;
    if(activity!=gActivityCameraActive){ReleaseNativeSneak();gActivityCameraActive=activity;gAutoFirstPersonNext=0;ReleaseNativeControllerActions();gControllerModes.Reset();gPhysicalCrouchWalk.Reset();gSneakToggle.Reset();gStereo.cameraStabilizer.Reset();
        gMelee.Reset();gBow.Cancel();gMagic.Cancel();gTrackedArms.ResetCombatSample();
        Message(activity?"vr_activity_camera_enter; native_third_person":"vr_activity_camera_exit; calibrated_first_person_return",0,true);}
    if(MenuPanelActive()&&!animal&&!activity&&!returning)return;
    if(animal!=gAnimalCameraActive){
        if(animal)gHumanHeightSaved=gStereo.heightAnchor.Snapshot(gHumanHeight);
        gAnimalCameraActive=animal;gAutoFirstPersonNext=0;gStereo.heightAnchor.Reset();gStereo.cameraStabilizer.Reset();
        if(!animal){gHumanHeightRestoreUntil=gHumanHeightSaved?0:GetTickCount64()+300;if(gHumanHeightSaved)gStereo.heightAnchor.Restore(reinterpret_cast<uintptr_t>(player),gHumanHeight);RequestCameraReset("animal_return_human");}
        gHeadControl.movementHeadingValid=false;gStereo.playerHeadingValid=false;
        gMelee.Reset();gBow.Cancel();gMagic.Cancel();gTrackedArms.ResetCombatSample();
        Message(animal?"vr_animal_camera_enter; native_third_person; humanoid_overrides_suspended":"vr_animal_camera_exit; first_person_restore_pending",0,true);
    }
    if(nativeReturn&&!animal&&!activity&&!dialog){
        if(ReturnFirstPersonFacing(camera,player)){gCameraReturn.Complete();return;}
        gCameraReturn.Complete();Message("vr_native_return_facing_refused; native_camera_preserved",0,true);
    }
    if(!diagnostic::Readable(camera,sizeof(*camera))||camera->IsDialogCameraActive()||
       ((animal||activity)?!diagnostic::AnimalCamera::FirstPerson(camera,sizeof(*camera)):camera->IsInFirstPerson()&&!returning)||
       GetTickCount64()<gAutoFirstPersonNext)return;
    size_t modeSlot=0;auto** cameraTable=*reinterpret_cast<void***>(camera);
    auto modeEntry=GetProcAddress(GetModuleHandleW(L"Game.dll"),"?SetMode@gCCameraAI_PS@@UAEXW4gECameraMode@@_N1@Z");
    if(!modeEntry||!diagnostic::VirtualSlot(&gCCameraAI_PS::SetMode,modeSlot)||modeSlot!=88||
       !diagnostic::Readable(cameraTable+modeSlot,sizeof(void*))||
       diagnostic::ResolveJumpChain(cameraTable[modeSlot])!=diagnostic::ResolveJumpChain(modeEntry)){
        gFirstPersonPermissionFault=true;gConfig.keepFirstPerson=false;Message("auto_first_person_native_slot_refused",0,true);return;}
    // SDK leaves gECameraMode unnamed. In the verified CP binary, case 11 uses
    // CamModeFirstPerson/first-person combat presets and sets the persistent
    // first-person flag. The permission hook remains chained to native scripts.
    // The exact Game.dll profile was verified by InstallFirstPersonLock.
    // SetMode consults the old FP flags before choosing its normal preset;
    // release all three latches first, including temporary native combat FP.
    if((animal||activity)&&!diagnostic::AnimalCamera::Release(camera,sizeof(*camera)))return;
    camera->SetMode(static_cast<gECameraMode>(animal||activity?0:11),GETrue,GEFalse);
    gAutoFirstPersonProcess=animal||activity?UINT64_MAX:gProcess.load(std::memory_order_relaxed);
    gAutoFirstPersonNext=GetTickCount64()+250;
    Message(activity?"vr_activity_third_person_requested; native_mode_0":animal?"vr_animal_third_person_requested; FP_latches_released; native_mode_0":"vr_auto_first_person_requested; native_mode_11; dialogue_preserved",0,true);
}
bool ConnectStereoRuntime()
{
    if(gStereoRuntime.Connected())return true;
    if(gStereo.shuttingDown||gStereo.fault||!gConfig.stereoEnabled||!gHeadControl.enabled||!gCameraRenderHook.original||GetTickCount64()<gStereo.nextRetry)return false;
    auto* wrapper=DeviceAccess::sGetAPIDevice();if(!diagnostic::Readable(wrapper,sizeof(eCDX9Device)))return false;
    auto* device=reinterpret_cast<eCDX9Device*>(wrapper)->m_pDevice;
    if(!diagnostic::Readable(device,sizeof(void*)))return false;
    wchar_t logPath[MAX_PATH];swprintf_s(logPath,L"%s\\stereo-game-%llu-%lu.jsonl",gLogDirectory,GetTickCount64(),GetCurrentProcessId());
    if(!gStereoRuntime.Connect(device,gRuntimeLoader,logPath,gVrSettings)){Event event=Record(Kind::Message);sprintf_s(event.text,"stereo_waiting: %.160s",gStereoRuntime.Error());Push(event,true);gStereo.nextRetry=GetTickCount64()+5000;return false;}
    gHeadControl.centered=false;gHeadControl.recenterPending=true;Message("VDXR_graphical_stereo_connected; headless_worker_disabled",0,true);return true;
}
bool ConnectStereoRuntimeSafely()
{
    __try{return ConnectStereoRuntime();}
    __except(EXCEPTION_EXECUTE_HANDLER){StereoFailure("stereo_connection_native_access_fault");return false;}
}
void RestoreTrackedArms(bool end);
void MoviePresent(IDirect3DDevice9* device){
    auto thread=GetCurrentThreadId();if(device!=gMovieDevice||!gHeadControl.enabled||!gConfig.stereoEnabled||!gCinemaScope.Capture(thread))return;
    __try{
        if(ConnectStereoRuntimeSafely()){
            diagnostic::StereoFrame frame;auto status=gStereoRuntime.Begin(frame);
            if(status==diagnostic::StereoBegin::Ready){
                PublishHeadPose(frame.head);if(frame.originChanged){gHeadControl.centered=false;gHeadControl.recenterPending=true;gHeadControl.movementHeadingValid=false;}gStereoRuntime.SetUiLayout(diagnostic::PanelLayout(gVrSettings,true));
                if(gStereoRuntime.CaptureCinema()&&gStereoRuntime.Submit()){
                    ++gMovieFrames;if(gMovieFrames==1||gMovieFrames%120==0){Event event=Record(Kind::Message);sprintf_s(event.text,"vr_movie_frame submissions=%llu opaque_both_eye_panel=1",gMovieFrames);Push(event,true);}
                }else {gStereoRuntime.Cancel();Message("vr_movie_capture_refused; native_playback_preserved",0,true);}
            }
            auto controllers=MappedControllers();auto now=GetTickCount64();
            bool fresh=controllers.focused&&controllers.hands[0].active&&now>=controllers.timeMs&&now-controllers.timeMs<=100;
            if(fresh&&!controllers.hands[0].menu)gMovieSkipArmed=true;
            diagnostic::ControllerCommands commands;commands.held[diagnostic::Back]=fresh&&gMovieSkipArmed&&controllers.hands[0].menu;
            gControllerInput.Update(commands,now,fresh);
        }
    }__finally{gCinemaScope.Finish();}
}
void SafeMoviePresent(IDirect3DDevice9* device){__try{MoviePresent(device);}__except(EXCEPTION_EXECUTE_HANDLER){gCinemaScope.Finish();gStereoRuntime.Cancel();Message("vr_movie_native_access_refused; native_playback_preserved",GetExceptionCode(),true);}}
HRESULT STDMETHODCALLTYPE PresentMovie(IDirect3DDevice9* self,RECT const* source,RECT const* destination,HWND window,RGNDATA const* dirty){
    using Present=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,RECT const*,RECT const*,HWND,RGNDATA const*);
    bool outer=!gMoviePresentEntered;if(outer){gMoviePresentEntered=true;SafeMoviePresent(self);}HRESULT result=E_FAIL;
    __try{result=reinterpret_cast<Present>(gMoviePresentHook.original)(self,source,destination,window,dirty);}
    __finally{if(outer)gMoviePresentEntered=false;}return result;
}
HRESULT STDMETHODCALLTYPE PresentExMovie(IDirect3DDevice9Ex* self,RECT const* source,RECT const* destination,HWND window,RGNDATA const* dirty,DWORD flags){
    using Present=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9Ex*,RECT const*,RECT const*,HWND,RGNDATA const*,DWORD);
    bool outer=!gMoviePresentEntered;if(outer){gMoviePresentEntered=true;SafeMoviePresent(self);}HRESULT result=E_FAIL;
    __try{result=reinterpret_cast<Present>(gMoviePresentExHook.original)(self,source,destination,window,dirty,flags);}
    __finally{if(outer)gMoviePresentEntered=false;}return result;
}
bool InstallMoviePresents(){
    auto* wrapper=DeviceAccess::sGetAPIDevice();if(!diagnostic::Readable(wrapper,sizeof(eCDX9Device)))return false;
    auto* device=reinterpret_cast<eCDX9Device*>(wrapper)->m_pDevice;if(!diagnostic::Readable(device,sizeof(void*)))return false;
    auto** table=*reinterpret_cast<void***>(device);
    if(!gMoviePresentHook.installed&&(!diagnostic::Readable(table+17,sizeof(void*))||!diagnostic::Executable(table[17])||!gMoviePresentHook.Prepare(table,17,reinterpret_cast<void*>(&PresentMovie))||!gMoviePresentHook.Install()))return false;
    IDirect3DDevice9Ex* ex=nullptr;
    if(SUCCEEDED(device->QueryInterface(__uuidof(IDirect3DDevice9Ex),reinterpret_cast<void**>(&ex)))){
        auto** extended=*reinterpret_cast<void***>(ex);
        bool ok=gMoviePresentExHook.installed||(diagnostic::Readable(extended+121,sizeof(void*))&&diagnostic::Executable(extended[121])&&gMoviePresentExHook.Prepare(extended,121,reinterpret_cast<void*>(&PresentExMovie))&&gMoviePresentExHook.Install());
        ex->Release();if(!ok)return false;
    }
    gMovieDevice=device;return true;
}
void __fastcall PlayNativeVideo(eCApplication* self,void*,bCString const& file){
    using Play=void(__thiscall*)(eCApplication*,bCString const&);auto thread=GetCurrentThreadId();
    bool scoped=gHeadControl.enabled&&gConfig.stereoEnabled&&!gStereo.shuttingDown&&gCinemaScope.Begin(thread);
    if(scoped){++gMovieEpoch;gMovieSkipArmed=false;gMovieFrames=0;
        RestoreHeadCamera(self);RestoreTrackedArms(true);gStereoRuntime.Cancel();gStereo.framePending=gStereo.uiRendering=false;
        ReleaseNativeSneak();gControllerInput.Release();ReleaseNativeControllerActions();gControllerModes.Reset();gPhysicalCrouchWalk.Reset();gSneakToggle.Reset();gControllerTranslator.ResetPointer();gRadialOpen=false;gMelee.Reset();gMagic.Cancel();gStereoRuntime.SetRadial({});gStereoRuntime.SetWorldStatus(0);
        __try{gBow.Cancel();}__except(EXCEPTION_EXECUTE_HANDLER){gBow.Fail(GetTickCount64());Message("vr_movie_ammo_restore_refused; ranged_retry_scheduled",GetExceptionCode(),true);}
        __try{if(!InstallMoviePresents())Message("vr_movie_present_hooks_refused; native_playback_preserved",0,true);}
        __except(EXCEPTION_EXECUTE_HANDLER){Message("vr_movie_present_setup_refused",GetExceptionCode(),true);}
        Message("vr_movie_begin; native_PlayVideo; decoder_audio_timing_preserved; Y_skip",0,true);
    }
    __try{reinterpret_cast<Play>(gPlayVideoHook.original)(self,file);}
    __finally{if(scoped){gCinemaScope.End(thread);gStereoRuntime.Cancel();gStereo.framePending=gStereo.uiRendering=false;gControllerInput.Release();gStereo.basePrepared=false;Message("vr_movie_end; native_playback_returned",0,true);}}
}
bool InstallMovieHook(){
    auto engine=GetModuleHandleW(L"Engine.dll");wchar_t path[MAX_PATH];unsigned char actual[32],expected[32];
    char const* hash="991cf674a9cf1bc60cddc204be0993e8251336f4d3471543a37846cc6840bf5a";
    for(unsigned i=0;i<32;++i){unsigned n=0;sscanf_s(hash+2*i,"%2x",&n);expected[i]=static_cast<unsigned char>(n);}
    if(!engine||!GetModuleFileNameW(engine,path,MAX_PATH)||!diagnostic::FileSha256(path,actual)||memcmp(actual,expected,32))return false;
    auto* entry=reinterpret_cast<unsigned char*>(GetProcAddress(engine,"?PlayVideo@eCApplication@@QAEXABVbCString@@@Z"));
    return entry==reinterpret_cast<unsigned char*>(engine)+0x14e10&&gPlayVideoHook.Install(entry,reinterpret_cast<void*>(&PlayNativeVideo),diagnostic::FocusHook::FindPrefix,sizeof(diagnostic::FocusHook::FindPrefix));
}
bool MenuPanelActive()
{
    // Use SDK queries, not guessed camera mode values. Dialogues/inventory use
    // the centered menu layout; the persistent HUD has its own lower position.
    __try {
        if(!gApplication||!gApplication->IsGameRunning())return true;
        auto* guiManager=gCSession::GetInstance().GetGUIManager();
        return !guiManager||!diagnostic::Readable(guiManager,sizeof(void*))||guiManager->IsMenuOpen()||guiManager->IsAnyPageOpen()||diagnostic::NativeLockpick(gCSession::GetInstance().GetPlayer());
    } __except(EXCEPTION_EXECUTE_HANDLER){return true;}
}
void __fastcall ProfileNativeSession(gCSession* self,void*)
{
    using Process=void(__thiscall*)(gCSession*);
    bool measure=gStereoRuntime.Connected()&&!gWorldLifecycle.Blocked()&&GetCurrentThreadId()==gApplicationThread.load();
    uint64_t started=measure?diagnostic::probes::Now():0;int context=measure&&MenuPanelActive()?2:1;
    __try {reinterpret_cast<Process>(gSessionProcessHook.original)(self);}
    __finally {if(measure)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::SessionProcess,started,context);}
}
void InstallNativeSessionProfiler()
{
    static bool attempted=false;if(!gSceneProfileEnabled||attempted||!gApplication||!gApplication->IsGameRunning())return;attempted=true;
    auto module=GetModuleHandleW(L"Game.dll");wchar_t path[MAX_PATH];unsigned char actual[32],expected[32];
    char const* hash="4728a47b244adcb0ea2ec3501db04994909dcfcff338c7b8075f2b68dd8d6e00";
    for(unsigned i=0;i<32;++i){unsigned byte=0;sscanf_s(hash+2*i,"%2x",&byte);expected[i]=static_cast<unsigned char>(byte);}
    if(!module||!GetModuleFileNameW(module,path,MAX_PATH)||!diagnostic::FileSha256(path,actual)||memcmp(actual,expected,32)){
        Message("native_session_profiler_unavailable; unknown_Game_profile; gameplay_unchanged",0,true);return;}
    auto* session=&gCSession::GetInstance();
    if(!diagnostic::Readable(session,sizeof(gCSession))){Message("native_session_profiler_unavailable; invalid_session; gameplay_unchanged",0,true);return;}
    auto** table=*reinterpret_cast<void***>(session);size_t slot=0;
    auto target=GetProcAddress(module,"?Process@gCSession@@UAEXXZ");
    auto expectedTarget=target?diagnostic::ResolveJumpChain(target):nullptr;
    if(!expectedTarget||!diagnostic::VirtualSlot(&gCSession::Process,slot)||!diagnostic::Readable(table+slot,sizeof(void*))||
       diagnostic::ResolveJumpChain(table[slot])!=expectedTarget||
       !gSessionProcessHook.Prepare(table,slot,reinterpret_cast<void*>(&ProfileNativeSession))||!gSessionProcessHook.Install()){
        Message("native_session_profiler_unavailable; virtual_target_not_verified; gameplay_unchanged",0,true);return;}
    Message("native_session_process_profiler_ready; inclusive_native_Process; separate_from_application_callback",DWORD(slot),true);
}
bool PrepareStereoFrame(eCCameraBase* camera,diagnostic::StereoFrame& frame)
{
    if(gWorldLifecycle.Blocked())return false;
    if(gStereo.shuttingDown || !gConfig.stereoEnabled || !gHeadControl.enabled || gStereo.fault || !gLogging.load() || !gApplication || GetCurrentThreadId()!=gApplicationThread.load() || camera!=&gApplication->GetCurrentCamera())return false;
    if(!diagnostic::Readable(camera,sizeof(eCCameraBase))||!camera->GetPerspective())return false;
    auto* renderDevice=DeviceAccess::sGetAPIDevice();
    if(!diagnostic::Readable(renderDevice,sizeof(eCDX9Device)))return false;
    auto worldPass=diagnostic::InspectStereoWorldPass(reinterpret_cast<eCDX9Device*>(renderDevice)->m_pDevice);
    if(!worldPass.IsWorld()){
        // Never start an XR frame, mutate the camera pose or double-render a
        // native auxiliary pass. The caller executes it once, unchanged.
        static unsigned reported=0;unsigned key=worldPass.kind==diagnostic::StereoWorldPass::Kind::AuxiliaryViewport?(worldPass.viewport.Width==512?1u:2u):worldPass.kind==diagnostic::StereoWorldPass::Kind::AuxiliaryTarget?4u:0u;
        if(key&&!(reported&key)){reported|=key;Event event=Record(Kind::Message);sprintf_s(event.text,"vr_auxiliary_camera_pass_native; viewport=%ux%u at %u,%u backbuffer=%ux%u; no_XR_frame_no_camera_override",worldPass.viewport.Width,worldPass.viewport.Height,worldPass.viewport.X,worldPass.viewport.Y,worldPass.backWidth,worldPass.backHeight);Push(event,true);}
        return false;
    }
    Event mode;CaptureCameraMode(mode);
    diagnostic::StereoCameraState cameraState{gApplication->IsGameRunning()!=GEFalse,mode.cameraModeValid,mode.firstPerson,mode.tempFirstPerson,mode.dialogCamera};
    if(cameraState.gameRunning){auto* player=gCSession::GetInstance().GetPlayer();cameraState.animal=diagnostic::NativeAnimalForm(player);cameraState.activity=!cameraState.animal&&CameraThirdPersonActivity(player);}
    gStereo.dialog=gVrSettings.firstPersonDialogue&&cameraState.gameRunning&&cameraState.modeAvailable&&(cameraState.dialog||NativeUiState()==int(gEGamePage_Dialog))&&!cameraState.animal&&!cameraState.activity;
    auto* chestPlayer=cameraState.gameRunning?gCSession::GetInstance().GetPlayer():nullptr;
    bool interactionViewAllowed=cameraState.gameRunning&&!cameraState.animal&&!gStereo.dialog&&!diagnostic::NativeRecovery(chestPlayer);
    bool chestAllowed=interactionViewAllowed&&gVrSettings.firstPersonChest;
    bool chestRequested=chestAllowed&&diagnostic::NativeChestInteraction(chestPlayer,gStereo.chest);
    bool chestCameraReady=cameraState.modeAvailable&&(cameraState.firstPerson||cameraState.tempFirstPerson)&&!cameraState.activity&&!cameraState.dialog&&
        gAutoFirstPersonProcess!=gProcess.load(std::memory_order_relaxed)&&GetTickCount64()>=gHumanHeightRestoreUntil;
    gStereo.chest=gChestView.Step(reinterpret_cast<uintptr_t>(chestPlayer),chestAllowed,chestRequested,chestCameraReady,GetTickCount64());
    bool lecternAllowed=interactionViewAllowed&&gVrSettings.firstPersonLectern&&!gStereo.chest;
    bool lecternRequested=lecternAllowed&&diagnostic::NativeLecternInteraction(chestPlayer,gStereo.lectern);
    gStereo.lectern=gLecternView.Step(reinterpret_cast<uintptr_t>(chestPlayer),lecternAllowed,lecternRequested,chestCameraReady,GetTickCount64());
    gStereo.dialogPlayer=0;
    if(gStereo.dialog||gStereo.chest||gStereo.lectern){auto* player=gCSession::GetInstance().GetPlayer();if(diagnostic::Readable(player,sizeof(eCDynamicEntity)))gStereo.dialogPlayer=reinterpret_cast<uintptr_t>(player);}
    else if(gDialogCamera.Active()){gDialogCamera.Release();Message("vr_dialog_camera_released; normal_camera_policy_resumed",0,true);}
    if(!diagnostic::StereoCameraAllowed(cameraState,gStereoRuntime.Connected())&&!gVrSettings.autoStart)return false;
    if(cameraState.gameRunning&&gConfig.keepFirstPerson&&!gFirstPersonLockAttempted)
    {
        gFirstPersonLockAttempted=true;
        if(!InstallFirstPersonLock())
        {gFirstPersonPermissionFault=true;gConfig.keepFirstPerson=false;Message("first_person_camera_permission_hook_refused; stereo_still_available",0,true);}
    }
    if(!ConnectStereoRuntime())return false;
    // The camera AI recomputes its position on the following update after a
    // late mode switch. Do not calibrate eye height from its old third-person
    // position during that transition frame.
    if(!cameraState.gameRunning){gHumanHeightSaved=false;gAnimalCameraActive=gActivityCameraActive=false;gHumanHeightRestoreUntil=0;}
    gStereo.heightAnchor.Context(cameraState.gameRunning,gVrSettings.stableHorizon);
    gStereo.player=0;if(cameraState.gameRunning&&(gVrSettings.stableHorizon||gStereo.dialog||gStereo.chest||gStereo.lectern)){auto* player=gCSession::GetInstance().GetPlayer();if(diagnostic::Readable(player,sizeof(eCDynamicEntity))){gStereo.player=reinterpret_cast<uintptr_t>(player);auto root=player->GetWorldPosition();gStereo.playerPosition[0]=root.m_fX;gStereo.playerPosition[1]=root.m_fY;gStereo.playerPosition[2]=root.m_fZ;}}
    bool switching=gAutoFirstPersonProcess==gProcess.load(std::memory_order_relaxed)||GetTickCount64()<gHumanHeightRestoreUntil;
    gStereo.firstPerson=diagnostic::ComfortFirstPerson(cameraState,gConfig.keepFirstPerson,gStereo.heightAnchor.Calibrated(gStereo.player),switching);
    gStereo.holdPlayerHeading=gStereo.firstPerson&&(!(cameraState.firstPerson||cameraState.tempFirstPerson)||switching||gFinisher.Busy(reinterpret_cast<eCEntity*>(gStereo.player)));
    gStereo.heightOffsetUnits=(gStereo.firstPerson||gStereo.dialog||gStereo.chest||gStereo.lectern)?gConfig.firstPersonHeightOffsetCm*.01f*gConfig.unitsPerMeter:0.f;
    if(!gStereo.firstPerson||!gVrSettings.stableHorizon||!gVrSettings.smoothCamera)gStereo.cameraStabilizer.Reset();
    if(!cameraState.gameRunning||!gVrSettings.stableHorizon)gStereo.playerHeadingValid=false;
    gStereo.hidePlayerHead=gConfig.hideFirstPersonHead&&(gStereo.firstPerson||gStereo.dialog||gStereo.chest||gStereo.lectern);
    auto currentMode=diagnostic::CameraMode(cameraState);
    if(gStereo.cameraMode!=int(currentMode))
    {
        gStereo.cameraMode=int(currentMode);Event event=Record(Kind::Message);
        sprintf_s(event.text,"stereo_camera_mode=%s; continuous_native_camera_render",diagnostic::CameraModeName(currentMode));Push(event,true);
    }
    gStereo.framePending=false;
    auto status=gStereoRuntime.Begin(frame);
    if(status==diagnostic::StereoBegin::Failed){StereoFailure(gStereoRuntime.Error());return false;}
    // A skipped render frame is not a changed tracking origin. Recentring here
    // continuously erases physical translation, especially at high resolution.
    if(status!=diagnostic::StereoBegin::Ready)return false;
    PublishHeadPose(frame.head);
    if(frame.originChanged)RequestCameraReset("runtime_local_origin_changed");
    if(gHeadControl.recenterPending || !gHeadControl.centered)
    {
        diagnostic::Orientation head{frame.head.quaternion[0],frame.head.quaternion[1],frame.head.quaternion[2],frame.head.quaternion[3]};
        if(!diagnostic::RecenterTracking(head,frame.head.position,gVrSettings.stableHorizon,gHeadControl.center,gStereo.centerPosition)){gStereoRuntime.Cancel();StereoFailure("recenter_pose_invalid");return false;}
        gHeadControl.centerSession=frame.head.session;strcpy_s(gHeadControl.origin,"local");
        gHeadControl.centered=true;gHeadControl.recenterPending=false;gHeadControl.movementHeadingValid=false;ControlRecord("stereo_centered",&frame.head);
        Event event=Record(Kind::Message);sprintf_s(event.text,"vr_camera_reset_applied center_m=%.3f,%.3f,%.3f; player_height_preserved",gStereo.centerPosition[0],gStereo.centerPosition[1],gStereo.centerPosition[2]);Push(event,true);
    }
    return true;
}
bool SkipOwnedFlameTail(eCParticle_PS* particle){
    // Hot callbacks must never query the entity/SDK or VirtualQuery memory.
    // Worker threads do not inspect the application-thread cache at all.
    return GetCurrentThreadId()==gApplicationThread.load()&&gFlameTargets.Contains(reinterpret_cast<uintptr_t>(particle));
}
void PrepareFlameTargets(diagnostic::ControllerFrame const& controllers){
    gFlameTargets.Reset();if(!gFlameHooksReady||gFlameHooksFault||gTrackedArms.fault)return;
    __try{
        if(!diagnostic::visual::CaptureFlameTargets(gCSession::GetInstance().GetPlayer(),controllers,GetTickCount64(),gFlameTargets)){
            gFlameTargets.Reset();Message("vr_flame_target_collection_refused; native_effects_preserved",0,true);}
    }__except(EXCEPTION_EXECUTE_HANDLER){gFlameTargets.Reset();gFlameHooksFault=true;Message("vr_flame_target_collection_fault; native_effects_preserved",GetExceptionCode(),true);}
}
bool ParticlePerformanceActive(){
    return GetCurrentThreadId()==gApplicationThread.load()&&gParticlePerformanceReady&&gVrSettings.particleQuality>0&&
        gConfig.stereoEnabled&&gStereo.rendering&&gStereo.eyeActive&&!gStereo.shuttingDown&&!gWorldLifecycle.Blocked();
}
char const* ParticleEffectName(eCParticle_PS* particle){
    // Native callback owns a live Particle_PS and its entity here. No scene
    // traversal, heap allocation, persistent entity cache or VirtualQuery.
    auto* owner=particle->GetEntity();return owner?owner->GetName().GetText():nullptr;
}
GEBool __fastcall RenderBudgetParticleVertices(eCParticle_PS* self,void*){
    using Call=GEBool(__thiscall*)(eCParticle_PS*);auto original=reinterpret_cast<Call>(gParticleBudgetHook.original);
    if(!ParticlePerformanceActive()||!gApplication||gParticleRenderLease.Active())return original(self);
    uint32_t frame=0;memcpy(&frame,reinterpret_cast<unsigned char*>(gApplication)+diagnostic::particles::ApplicationFrameOffset,4);
    // Preserve the native once-per-frame GPU buffer cache, including instanced
    // master buffers shared by both eyes. Never build twice to change quality.
    if(self->__UNK_0348==frame)return original(self);
    int count=self->m_arrParticles.GetCount();if(count<=12)return original(self);
    char const* name=ParticleEffectName(self);
    if(!diagnostic::particles::Effect(name)&&!diagnostic::particles::Effect(self->GetTexture().GetText()))return original(self);
    bool leased=gParticleRenderLease.Begin(self,sizeof(*self),gVrSettings.particleQuality);GEBool result=GEFalse;
    __try {result=original(self);}
    __finally {if(leased)gParticleRenderLease.End();}
    if(leased&&result&&!gParticleBudgetReported[gVrSettings.particleQuality]){
        gParticleBudgetReported[gVrSettings.particleQuality]=true;Event event=Record(Kind::Message);
        sprintf_s(event.text,"vr_particle_budget_applied profile=%d native_count=%d draw_count=%d; simulation_array_restored; native_stereo_buffer_cache",gVrSettings.particleQuality,count,diagnostic::particles::DrawCount(count,gVrSettings.particleQuality));Push(event,true);
    }
    return result;
}
GEBool __fastcall ParticleRefraction(eCParticle_PS const* self,void*){
    if(ParticlePerformanceActive()&&diagnostic::particles::HeatOnly(ParticleEffectName(const_cast<eCParticle_PS*>(self))))return GEFalse;
    return reinterpret_cast<GEBool(__thiscall*)(eCParticle_PS const*)>(gParticleRefractionHook.original)(self);
}
void __fastcall ProcessOwnedFlame(eCParticle_PS* self,void*){
    if(SkipOwnedFlameTail(self)){++gFlameProcessSkipped;return;}
#ifdef G3VR_ENTITY_COST_TRIAL
    auto started=EntityCostParticleStarted();
    __try{reinterpret_cast<void(__thiscall*)(eCParticle_PS*)>(gFlameProcessHook.original)(self);}
    __finally{EntityCostParticleFinished(started);}
#else
    reinterpret_cast<void(__thiscall*)(eCParticle_PS*)>(gFlameProcessHook.original)(self);
#endif
}
GEBool __fastcall RenderAttachedParticleVertices(eCParticle_PS* self,void*,GEFloat alpha,eCParticle_PS* owner)
{
    bool scoped=false;
    if(GetCurrentThreadId()==gApplicationThread.load()&&gTorchHooksReady&&gStereo.rendering&&gStereo.eyeActive&&gTorchParticles.Contains(reinterpret_cast<uintptr_t>(self))&&(!owner||owner==self)){
        __try {auto* entity=reinterpret_cast<eCEntity*>(gTorchParticles.Anchor(reinterpret_cast<uintptr_t>(self)));diagnostic::arms::Mat delta;
            if(diagnostic::Readable(entity,sizeof(eCEntity))&&gTorchParticles.Delta(reinterpret_cast<uintptr_t>(self),diagnostic::visual::ParticleMatrix(entity->GetWorldMatrix()),delta))scoped=gParticleWorld.Begin(delta);
        }__except(EXCEPTION_EXECUTE_HANDLER){scoped=false;}
    }
    using Call=GEBool(__thiscall*)(eCParticle_PS*,GEFloat,eCParticle_PS*);GEBool result=GEFalse;
    __try {result=reinterpret_cast<Call>(gParticleVerticesHook.original)(self,alpha,owner);}
    __finally {if(scoped)gParticleWorld.End();}
    return result;
}
bEResult __fastcall AttachedParticleWorldMatrix(eCGfxMixerAdmin* self,void*,bCMatrix const& matrix)
{
    using Call=bEResult(__thiscall*)(eCGfxMixerAdmin*,bCMatrix const&);
    diagnostic::arms::Mat changed;
    auto* scope=diagnostic::visual::ParticleWorldScope::Current();
    if(scope&&scope->Rewrite(diagnostic::visual::ParticleMatrix(matrix),changed)){
        bCMatrix value;memcpy(&value,&changed,sizeof(value));return reinterpret_cast<Call>(gParticleWorldHook.original)(self,value);}
    return reinterpret_cast<Call>(gParticleWorldHook.original)(self,matrix);
}
void __fastcall RenderOwnedFlameAlpha(eCParticle_PS* self,void*,GEI16 pass){
    if(SkipOwnedFlameTail(self)){++gFlameDrawSkipped;return;}
    if(ParticlePerformanceActive()&&diagnostic::particles::HeatOnly(ParticleEffectName(self))){
        if(!gParticleHeatReported[gVrSettings.particleQuality]){gParticleHeatReported[gVrSettings.particleQuality]=true;Message("vr_particle_heat_haze_bypassed; fire_and_light_retained; native_effect_lifetime",0,true);}return;
    }
    reinterpret_cast<void(__thiscall*)(eCParticle_PS*,GEI16)>(gFlameAlphaHook.original)(self,pass);
}
void BeginMenuStaffVisibility(diagnostic::visual::MenuWeaponScope& scope){
    if(!gStereo.firstPerson||!gVrSettings.controllers||!gVrSettings.trackedArms||gWorldLifecycle.Blocked())return;
    __try{
        if(!diagnostic::visual::CaptureMenuStaff(gCSession::GetInstance().GetPlayer(),gStereo.firstPerson,MenuPanelActive(),gRadialOpen,scope)){
            scope.Abandon();Message("vr_menu_staff_collection_refused; native_visibility_preserved",0,true);return;}
        if(scope.Count())scope.Hide();
    }__except(EXCEPTION_EXECUTE_HANDLER){scope.Restore();Message("vr_menu_staff_visibility_refused; native_visibility_preserved",GetExceptionCode(),true);}
}
void BeginStowedWeaponVisibility(diagnostic::visual::MenuWeaponScope& scope){
    if(!(gStereo.firstPerson||gStereo.chest)||gWorldLifecycle.Blocked())return;
    __try{
        if(!diagnostic::visual::CapturePlayerStowedWeapons(gCSession::GetInstance().GetPlayer(),true,scope)){
            scope.Abandon();Message("vr_stowed_weapon_collection_refused; native_visibility_preserved",0,true);}
    }__except(EXCEPTION_EXECUTE_HANDLER){scope.Abandon();Message("vr_stowed_weapon_collection_fault; native_visibility_preserved",GetExceptionCode(),true);}
}
void __cdecl TrackedArmsRecord(char const* text){Message(text,0,true);}
void RestoreTrackedArms(bool end=false)
{
    __try {if(end)gChestVisual.End();else gChestVisual.Restore();}
    __except(EXCEPTION_EXECUTE_HANDLER){gChestVisual.Abandon();Message("vr_chest_visual_restore_refused",GetExceptionCode(),true);}
    __try {if(end)gMagicHandEffects.Reset();else gMagicHandEffects.Restore();}
    __except(EXCEPTION_EXECUTE_HANDLER){gMagicHandLayoutReady=false;Message("vr_magic_hand_effect_restore_refused",GetExceptionCode(),true);}
    __try {if(end)gTrackedArms.End();else gTrackedArms.RestoreEye();}
    __except(EXCEPTION_EXECUTE_HANDLER){gTrackedArms.fault=true;Message("vr_arms_restore_fault; tracked_arms_disabled",GetExceptionCode(),true);}
    __try {if(end)gTrackedBow.End();else gTrackedBow.RestoreEye();}
    __except(EXCEPTION_EXECUTE_HANDLER){gTrackedBow.Abandon();gTrackedBow.Fail("restore");Message("vr_bow_palette_restore_fault; ranged_binding_retry_scheduled",GetExceptionCode(),true);}
}
void ReleaseWorldBindings(bool restoreAmmo)
{
    gHandHud.Reset();
    gThirdPersonOrbit.Reset();
    gLocatorNativeInput.pulse.Cancel();
    gLocatorProjection.Reset();
    gTrackedArms.ResetBodyPosture();
    RestoreCurrentBodyFacing();gHeadBodyHeading.Reset();
    gKnockoutCamera.Reset();gActivityCameraGate.Reset();gCameraReturn.Reset();
    gChestVisual.Reset();gChestView.Reset();gLecternView.Reset();gDialogCamera.Reset();gStereo.dialog=gStereo.chest=gStereo.lectern=false;gStereo.dialogPlayer=0;
    RestoreCurrentHeroVoice();
    gFlameTargets.Reset();
    // Invoked before native Stop clears the player and destroys the old world.
    ReleaseNativeSneak();
    RestoreHeadCamera(gApplication);RestoreTrackedArms(true);
    __try {if(restoreAmmo)gBow.Cancel();}__except(EXCEPTION_EXECUTE_HANDLER){Message("vr_world_stop_ammo_restore_refused; ownership_forgotten",GetExceptionCode(),true);}
    gTorchParticles.Reset();gParticleWorld.End();
    gBow.Abandon();gMagic.Cancel();gMelee.Reset();gFinisher.Reset();gFinisherSearch.Reset();gPlayerFocusDispatch.Reset();gStereoRuntime.SetWorldStatus(0);gTrackedArms.ResetCombatSample();
    ReleaseNativeControllerActions();gControllerModes.Reset();gPhysicalCrouchWalk.Reset();gSneakToggle.Reset();gGuardPose.Reset();gQuickHeal.Reset();gInstantTurnCamera=nullptr;gLastControllerTurnCamera=nullptr;gControllerYaw.Reset();gTurnOutput.Reset();gActivityCameraGate.Reset();
    gControllerInput.Release();gWorldInteraction.selected=0;gControllerTranslator.ResetPointer();gRadialOpen=false;
    gStereo.player=0;gStereo.firstPerson=false;gStereo.basePrepared=false;gStereo.heightAnchor.Reset();gStereo.cameraStabilizer.Reset();
}
void __fastcall StopNativeSession(gCSession* self,void*)
{
    gWorldLifecycle.BeginStop();Message("vr_world_stop_begin; bindings_restored_before_native_Stop",0,true);
    __try {ReleaseWorldBindings(true);reinterpret_cast<void(__thiscall*)(gCSession*)>(gSessionStopHook.original)(self);}
    __finally {gBow.Abandon();gWorldLifecycle.EndStop();Message("vr_world_stop_complete; old_entity_ownership_forgotten",0,true);}
}
void __fastcall StereoDestroyWorkspace(eCApplication* self,void*)
{
    gWorldLifecycle.BeginShutdown();gStereo.shuttingDown=true;
    Message("stereo_workspace_shutdown_begin; before_native_world_release",0,true);
    ReleaseWorldBindings(true);gHeadControl.enabled=false;gHeadControl.centered=false;StopStereo();
    reinterpret_cast<void(__thiscall*)(eCApplication*)>(gDestroyWorkspaceHook.original)(self);
}
GEBool __fastcall UpdateTrackedArmPalette(eCVisualAnimationFactory* self,void*,GEU32 frame,bCMatrix const& worldMatrix)
{
    using Update=GEBool(__thiscall*)(eCVisualAnimationFactory*,GEU32,bCMatrix const&);
    bool ours=gTrackedRenderThread&&gTrackedArms.Matches(self);
    bool bow=gTrackedRenderThread&&gTrackedBow.Matches(self);
    if(ours){__try {gTrackedArms.BeforePalette();}__except(EXCEPTION_EXECUTE_HANDLER){gTrackedArms.fault=true;}}
    if(bow){__try {gTrackedBow.RestorePalette();}__except(EXCEPTION_EXECUTE_HANDLER){gTrackedBow.Abandon();gTrackedBow.Fail("before_palette");}}
    __try {if(gChestVisual.Matches(self))gChestVisual.Restore();}
    __except(EXCEPTION_EXECUTE_HANDLER){gChestVisual.Abandon();}
    GEBool result=reinterpret_cast<Update>(gArmPaletteHook.original)(self,frame,worldMatrix);
    if(diagnostic::ranged::ApplyTrackedFrame(ours,result,gTrackedArms.CrossbowFrame()||gTrackedArms.FrozenFrame())){__try {gTrackedArms.Apply(self,worldMatrix);}__except(EXCEPTION_EXECUTE_HANDLER){gTrackedArms.fault=true;RestoreTrackedArms();Message("vr_arms_pose_fault; native_arms_restored",GetExceptionCode(),true);}}
    if(diagnostic::ranged::ApplyTrackedFrame(bow,result,gTrackedBow.CrossbowFrame())){__try {gTrackedBow.Apply(self,frame,gArmPaletteHook.original);}__except(EXCEPTION_EXECUTE_HANDLER){RestoreTrackedArms();gTrackedBow.Fail("pose");Message("vr_bow_palette_pose_fault; ranged_binding_retry_scheduled",GetExceptionCode(),true);}}
    if(gTrackedRenderThread){__try {gChestVisual.Updated(self,worldMatrix);}
        __except(EXCEPTION_EXECUTE_HANDLER){gChestVisual.Abandon();Message("vr_chest_pose_freeze_refused",GetExceptionCode(),true);}}
    return result;
}
void BeginTrackedArms(CameraSnapshot const& saved,diagnostic::HeadPose const& head)
{
    gFlameTargets.Reset();gTorchParticles.Reset();gMagic.aim.sample={};
    // A save load/camera transition may skip Begin below. Never expose last
    // frame's player/weapon pointers to the independent ranged render binder.
    gTrackedArms.DiscardFrameSamples();
    __try {
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        if(gFinisher.Busy(reinterpret_cast<eCEntity*>(gStereo.player))){RestoreTrackedArms(true);return;}
        if(!gArmPaletteHook.original||!gVrSettings.controllers||!gVrSettings.trackedArms||!gStereo.firstPerson||!gHeadControl.centered||foreground!=GetCurrentProcessId())return;
        float base[3];diagnostic::Rotation3 native;StereoCameraBase(saved,native,base);
        float stable[3]{},correction[3]{};
        if(gVrSettings.stableHorizon&&gVrSettings.smoothCamera&&gStereo.cameraStabilizer.Apply(gStereo.player,gStereo.playerPosition,stable))
            for(unsigned i=0;i<3;++i)correction[i]=stable[i]-gStereo.playerPosition[i];
        auto controllers=MappedControllers();gTrackedArms.SetCombatTracking(controllers,head);
        gTrackedArms.SetMagicGripOffsets(gVrSettings.numbers+48);
        gTrackedArms.SetHandCalibration(gVrSettings.numbers+8,gVrSettings.numbers+14);
        gTrackedArms.SetSwordCalibration(gVrSettings.numbers+20);
        gTrackedArms.SetHeavyTwoHand(gVrSettings.heavyTwoHand);
        gTrackedArms.SetMeleeHandling(gVrSettings.numbers+73);
        gTrackedArms.SetTwoHandSettings(gVrSettings.twoHandedMelee,gVrSettings.numbers+32,gVrSettings.numbers[35],!gRadialOpen&&!MenuPanelActive()&&!gCSession::GetInstance().IsPaused(),gVrSettings.toggleSupportGrip,gVrSettings.numbers[82]);
        gTrackedArms.SetShieldSettings(gVrSettings.numbers+58);
        gTrackedArms.SetStaffSettings(gVrSettings.numbers+36,gVrSettings.numbers[39],gVrSettings.numbers[40],gVrSettings.numbers[41],gVrSettings.numbers[83]);
        gTrackedArms.SetBowState(gBow.draw.pulling,gVrSettings.numbers+23,gVrSettings.numbers[28],gVrSettings.numbers[81]);
        gTrackedArms.SetBowGripOffsets(gVrSettings.numbers+29);
        gTrackedArms.SetCrossbowSettings(gVrSettings.numbers+42,gVrSettings.numbers[84]);
        diagnostic::Rotation3 bodyHeading;LARGE_INTEGER now;QueryPerformanceCounter(&now);
        bool locomotion=false;auto* bodyPlayer=gCSession::GetInstance().GetPlayer();
        auto* bodyControl=static_cast<gCCharacterControl_PS*>(bodyPlayer->GetPropertySet(eEPropertySetType_CharacterControl));
        if(diagnostic::Readable(bodyControl,sizeof(*bodyControl))){auto wished=bodyControl->GetWishedDirection();locomotion=diagnostic::BodyLocomotionIntent(wished.m_fX,wished.m_fZ);}
        bool bodyTracked=head.session==gHeadControl.centerSession&&gHeadBodyHeading.Step(gStereo.player,native,gHeadControl.center,head,now.QuadPart,gFrequency.QuadPart,bodyHeading,locomotion);
        if(bodyTracked&&gVrSettings.stableBody&&gVrSettings.stableHorizon&&diagnostic::RoomPositionTracked(head,now.QuadPart,gFrequency.QuadPart)){
            float neck[3]{};float forward=float(gVrSettings.numbers[6])*.01f*gConfig.unitsPerMeter;
            if(diagnostic::RoomNeckOffset(gHeadControl.center,native,head,gStereo.centerPosition,gConfig.unitsPerMeter,forward,neck)){
                correction[0]+=neck[0];correction[2]+=neck[2]; // Y stays on the native ground/height path.
                static uint64_t next=0;if(diagnostic::probes::Enabled&&GetTickCount64()>=next){next=GetTickCount64()+1000;Event event=Record(Kind::Message);
                    sprintf_s(event.text,"vr_body_neck_anchor requested_cm=%.2f,%.2f moving=%d forward_cm=%d; same_eye_hand_frame; native_ground_Y",neck[0],neck[2],locomotion,gVrSettings.numbers[6]);Push(event,true);}
            }
        }
        gTrackedArms.Begin(gCSession::GetInstance().GetPlayer(),controllers,native,base,gHeadControl.center,gStereo.centerPosition,gConfig.unitsPerMeter,gStereo.heightOffsetUnits,gVrSettings.stableBody,TrackedArmsRecord,{correction[0],correction[1],correction[2]},bodyTracked?&bodyHeading:nullptr);
        PrepareFlameTargets(controllers);
        if(gTorchHooksReady&&!diagnostic::visual::CaptureHeldParticles(gCSession::GetInstance().GetPlayer(),controllers,GetTickCount64(),gTorchParticles))gTorchParticles.Reset();
        auto* magicPlayer=gCSession::GetInstance().GetPlayer();auto* spell=gMagic.Selected(magicPlayer);
        if(spell&&!gRadialOpen&&!gCSession::GetInstance().IsPaused())gMagic.aim.Prepare(reinterpret_cast<uintptr_t>(magicPlayer),reinterpret_cast<uintptr_t>(spell),controllers,head,native,base,gHeadControl.center,gStereo.centerPosition,gConfig.unitsPerMeter,gStereo.heightOffsetUnits,gVrSettings.numbers+14,gVrSettings.numbers[28],GetTickCount64());
    } __except(EXCEPTION_EXECUTE_HANDLER){gFlameTargets.Reset();gTrackedArms.fault=true;RestoreTrackedArms(true);Message("vr_arms_prepare_fault; native_arms_preserved",GetExceptionCode(),true);}
}
uint64_t NativeRenderStarted(){if(GetCurrentThreadId()!=gApplicationThread.load())return 0;return gNativeRenderScope?diagnostic::performance::Now():0;}
diagnostic::performance::SceneWorkState CaptureSceneWorkState()
{
    diagnostic::performance::SceneWorkState state;state.work=diagnostic::performance::CaptureSceneWork();
    state.threading=eCThreadPool::IsThreadingEnabled();state.jobs=eCThreadPool::GetGlobalNumberOfJobsInQueue();state.workers=eCThreadPool::GetGlobalNumberOfThreadsWorking();return state;
}
void FinishSceneWork(char const* name,uint64_t started,int context,diagnostic::performance::SceneWorkState const& before)
{
    if(!started)return;auto ended=diagnostic::probes::Now();
    if(ended>=started&&ended-started>=uint64_t(gFrequency.QuadPart)/50)
        gStereoRuntime.NoteSceneStall(name,started,context,before,CaptureSceneWorkState(),ended);
}
uint64_t NativeSceneUpdateStarted()
{
    if(!gSceneProfileEnabled||GetCurrentThreadId()!=gApplicationThread.load()||gWorldLifecycle.Blocked()||!gStereoRuntime.Connected())return 0;
    return diagnostic::probes::Now();
}
void __fastcall StereoPupOptionalWork(uint32_t nativeStarted,bool forced)
{
    using Call=void(__fastcall*)(uint32_t,bool);auto original=reinterpret_cast<Call>(gPupWorkHook.original);
    auto started=NativeRenderStarted();
    if(!started){original(nativeStarted,forced);return;}
    ++gPupStats.calls;
    if(gNativeRenderContext==1){if(forced)++gPupStats.worldForced;else ++gPupStats.worldOptional;}
    bool controlled=!forced&&gNativeRenderContext==1&&!gWorldLifecycle.Blocked()&&!gStereo.shuttingDown;
    if(!controlled){++gPupStats.bypass;
        __try {original(nativeStarted,forced);}
        __finally {if(gSceneProfileEnabled)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::PupWork,started,gNativeRenderContext);}
        return;
    }
    if(!gPupBudget.Admit(gPupStereoEpoch,started,gPupStereoEye==1)){++gPupStats.budget;return;}
    // The SDK wraps a recursive Windows critical section. Retain one extra
    // acquisition through the original call; it enters/leaves recursively.
    // Busy optional work stays wholly owned by PUP until a later frame.
    if(!gPupQueueLock->TryAcquire()){++gPupStats.busy;return;}
    auto adjusted=gPupBudget.Start(started,nativeStarted);++gPupStats.run;
    __try {original(adjusted,forced);}
    __finally {gPupQueueLock->Release();auto ended=diagnostic::performance::Now();gPupBudget.Finish(ended);
        if(ended>=started)gPupStats.maximum=(std::max)(gPupStats.maximum,ended-started);
        if(gSceneProfileEnabled)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::PupWork,started,gNativeRenderContext);}
}
void InstallPupStereoCompatibility()
{
    wchar_t mode[8]{};if(GetEnvironmentVariableW(L"G3VR_PUP_COMPAT",mode,8)&&wcscmp(mode,L"0")==0){Message("pup_vr_prefetch_compat_disabled_by_environment",0,true);return;}
    auto module=GetModuleHandleW(L"paru_s2.dll");wchar_t path[MAX_PATH];unsigned char digest[32]{};
    if(!module||!GetModuleFileNameW(module,path,MAX_PATH)||!diagnostic::FileSha256(path,digest)||memcmp(digest,diagnostic::pup::FileHash,32)){
        Message("pup_vr_prefetch_compat_unavailable; native_patch_unchanged",0,true);return;}
    auto* base=reinterpret_cast<unsigned char*>(module);auto* lock=reinterpret_cast<bCCriticalSectionWin32*>(base+diagnostic::pup::QueueLockRva);
    static_assert(sizeof(bCCriticalSectionWin32)==sizeof(CRITICAL_SECTION));
    HMODULE pinned=nullptr;
    if(!diagnostic::Readable(lock,sizeof(*lock))||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(base),&pinned)||
       !gPupWorkHook.Install(base+diagnostic::pup::WorkRva,reinterpret_cast<void*>(&StereoPupOptionalWork),diagnostic::pup::WorkPrefix,sizeof(diagnostic::pup::WorkPrefix))){
        Message("pup_vr_prefetch_compat_refused; original_patch_unchanged",0,true);return;}
    gPupQueueLock=lock;
    unsigned char lowPolyBody[sizeof(diagnostic::pup::LowPolyBody)]{};
    if(gSceneProfileEnabled&&diagnostic::pup::LowPolyBodyAtBase(uint32_t(reinterpret_cast<uintptr_t>(base)),lowPolyBody)&&
       diagnostic::Readable(base+diagnostic::pup::LowPolyWrapperRva,sizeof(lowPolyBody))&&
       !memcmp(base+diagnostic::pup::LowPolyWrapperRva,lowPolyBody,sizeof(lowPolyBody))&&diagnostic::Readable(base+diagnostic::pup::LowPolyFlagRva,1)){
        gPupLowPolyFlag=base+diagnostic::pup::LowPolyFlagRva;
        Message("pup_low_poly_flag_telemetry_ready; read_only; existing_renderer_hook; PUP_code_unchanged",1,true);
    }else Message("pup_low_poly_flag_telemetry_unavailable; no_unverified_state_read",0,true);
    Message("pup_vr_prefetch_compat_ready; shared_stereo_budget_ms=2 nonblocking_optional_queue=1 forced_loading_preserved=1 native_camera_threading=1 right_eye_optional_publish_deferred=1",0,true);
}
template<unsigned Index,diagnostic::performance::NativePhase Phase=static_cast<diagnostic::performance::NativePhase>(unsigned(diagnostic::performance::NativePhase::FrustumUpdate)+Index)> void __fastcall ProfileNativeFrustum(void* self,void*)
{
    auto started=NativeRenderStarted();using Call=void(__thiscall*)(void*);
    bool detail=Index==2&&gSceneProfileEnabled&&started&&gNativeRenderContext==1;
    auto work=detail?CaptureSceneWorkState():diagnostic::performance::SceneWorkState{};
    __try {reinterpret_cast<Call>(gNativeRenderHooks[Index].original)(self);}
    __finally {if(detail)FinishSceneWork(diagnostic::performance::NativePhaseNames[unsigned(Phase)],started,gNativeRenderContext,work);if(started)gStereoRuntime.NoteNativePhase(Phase,started,gNativeRenderContext);}
}
int SamplePupLowPolyClass()noexcept
{
    // Only called from a sampled main-thread native renderer invocation.
    __try {return gPupLowPolyFlag?diagnostic::pup::LowPolyClass(*gPupLowPolyFlag):-1;}
    __except(EXCEPTION_EXECUTE_HANDLER){return -1;}
}
void __fastcall ProfileNativeRenderer(eCRenderSystemAdmin* self,void*,eCFrustumDatabase& database)
{
    auto started=NativeRenderStarted();using Call=void(__thiscall*)(eCRenderSystemAdmin*,eCFrustumDatabase&);
    int view=started&&gSceneExecutionSample?SamplePupLowPolyClass():-1;
    __try {reinterpret_cast<Call>(gNativeRenderHooks[4].original)(self,database);}
    __finally {
        if(started&&view>=0)gStereoRuntime.NoteNativePhase(view?diagnostic::performance::NativePhase::SampledLowPolyRenderer:diagnostic::performance::NativePhase::SampledNonLowPolyRenderer,started,gNativeRenderContext);
        if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::RendererRender,started,gNativeRenderContext);
    }
}
GEBool __fastcall ProfileNativePvsProcess(eCPVSPrefetcher3* self,void*,eCFrustumDatabase& database)
{
    auto started=NativeRenderStarted();using Call=GEBool(__thiscall*)(eCPVSPrefetcher3*,eCFrustumDatabase&);
    bool detail=gSceneProfileEnabled&&started&&gNativeRenderContext==1;
    auto work=detail?CaptureSceneWorkState():diagnostic::performance::SceneWorkState{};
    __try {return reinterpret_cast<Call>(gNativeRenderHooks[6].original)(self,database);}
    __finally {if(detail)FinishSceneWork("pvs_prefetch_process_inclusive",started,gNativeRenderContext,work);if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::PvsProcess,started,gNativeRenderContext);}
}
void __fastcall ProfileNativePvsExecute(eCPVSPrefetcher3* self,void*,eCFrustumDatabase& database,bCSphere const& sphere)
{
    auto started=NativeRenderStarted();using Call=void(__thiscall*)(eCPVSPrefetcher3*,eCFrustumDatabase&,bCSphere const&);
    __try {reinterpret_cast<Call>(gNativeRenderHooks[7].original)(self,database,sphere);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::PvsExecute,started,gNativeRenderContext);}
}
GEBool __fastcall ProfileNativePvsGrid(eCPVSPrefetcher3* self,void*,eCFrustumDatabase& database,bCSphere const& sphere,bTSmallArray<eCPVSCellItem*>& cells)
{
    auto started=NativeRenderStarted();using Call=GEBool(__thiscall*)(eCPVSPrefetcher3*,eCFrustumDatabase&,bCSphere const&,bTSmallArray<eCPVSCellItem*>&);
    __try {return reinterpret_cast<Call>(gNativeRenderHooks[9].original)(self,database,sphere,cells);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::PvsGrid,started,gNativeRenderContext);}
}
void __fastcall ProfileNativePvsCells(eCPVSPrefetcher3* self,void*,eCFrustumDatabase& database,bTSmallArray<eCPVSCellItem*>& cells)
{
    auto started=NativeRenderStarted();using Call=void(__thiscall*)(eCPVSPrefetcher3*,eCFrustumDatabase&,bTSmallArray<eCPVSCellItem*>&);
    __try {reinterpret_cast<Call>(gNativeRenderHooks[10].original)(self,database,cells);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::PvsCells,started,gNativeRenderContext);}
}
template<unsigned Index,diagnostic::performance::NativePhase Phase> void __fastcall ProfileNativeSceneVoid(void* self,void*)
{
    auto started=NativeRenderStarted();using Call=void(__thiscall*)(void*);
    __try {reinterpret_cast<Call>(gNativeRenderHooks[Index].original)(self);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(Phase,started,gNativeRenderContext);}
}
template<unsigned Index,diagnostic::performance::NativePhase Phase,class Result> Result __fastcall ProfileNativeSceneResult(void* self,void*)
{
    // Pool waits can block NPC/physics updates before either eye begins.
    auto started=Index>=15?NativeSceneUpdateStarted():NativeRenderStarted();
    int context=started?(gNativeRenderScope?gNativeRenderContext:gSceneUpdateContext):0;using Call=Result(__thiscall*)(void*);
    __try {return reinterpret_cast<Call>(gNativeRenderHooks[Index].original)(self);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(Phase,started,context);}
}
template<unsigned Index,diagnostic::performance::NativePhase Phase,bool Game=false> void __fastcall ProfileNativeSceneUpdate(void* self,void*)
{
    auto started=NativeSceneUpdateStarted();int context=started?(gNativeRenderScope?gNativeRenderContext:gSceneUpdateContext):0;
    using Call=void(__thiscall*)(void*);void* original=nullptr;
    if constexpr(Game)original=gGameSceneHooks[Index].original;else original=gNativeRenderHooks[Index].original;
    __try {reinterpret_cast<Call>(original)(self);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(Phase,started,context);}
}
uint64_t SceneExecutionStarted()noexcept;
bool ReadStaticTrialCamera(float* view,float* projection)noexcept {
    __try{auto* camera=gStereo.camera;if(!camera||!diagnostic::Readable(camera,sizeof(void*)))return false;
        for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){view[r*4+c]=camera->GetViewMatrix()(r,c);projection[r*4+c]=camera->GetProjectionMatrix()(r,c);}return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void __fastcall TemporalEndSolid(void* self,void*)
{
    if(reinterpret_cast<uintptr_t>(_ReturnAddress())==gTemporalSolidCaller&&
       GetCurrentThreadId()==gApplicationThread.load()&&gNativeRenderScope&&gStereo.rendering&&gStereo.eyeActive)
    {
        gStereoRuntime.CaptureTemporalOpaqueDepth();
        if(gStaticTrialHooksOptIn){float view[16],projection[16];if(ReadStaticTrialCamera(view,projection))gStereoRuntime.StaticTrialSolidBoundary(view,projection);}
    }
    auto started=SceneExecutionStarted();
    __try {reinterpret_cast<void(__thiscall*)(void*)>(gTemporalSolidHook.original)(self);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::SampledSolidEnd,started,gNativeRenderContext);}
}
bool ReadTemporalObject(eCRenderContextAnimation* context,diagnostic::temporal::Object& out)
{
    // The SDK property ID is copied, not retained. No strings, refcounts or
    // animation state changes. An invalid nested context masks the outer one.
    __try {
        if(!context||!diagnostic::Readable(context,sizeof(*context))||!context->m_pEntity||!context->m_VisualAnimationPS)return false;
        auto const& id=context->m_pEntity->GetID();if(!id.IsValid())return false;
        static_assert(sizeof(bCPropertyID::bSCore)==16);
        memcpy(out.id.data(),&id.GetCore(),16);out.id[4]=id.GetCount();
        out.context=reinterpret_cast<uintptr_t>(context);out.visual=reinterpret_cast<uintptr_t>(context->m_VisualAnimationPS);
        memcpy(out.world.data(),&context->m_pEntity->GetWorldMatrix(),sizeof(float)*16);
        return out.Valid();
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
GEBool __fastcall TemporalAnimatedRender(eCRenderContextAnimation* self,void*,eCRenderOp2 const& operation)
{
    using Call=GEBool(__thiscall*)(eCRenderContextAnimation*,eCRenderOp2 const&);
    auto original=reinterpret_cast<Call>(gTemporalAnimationHook.original);unsigned token=UINT_MAX;
    if(!gStereoRuntime.TemporalEyeActive())return original(self,operation);
    if(GetCurrentThreadId()==gApplicationThread.load()&&gNativeRenderScope&&gStereo.rendering&&gStereo.eyeActive&&gStereoRuntime.TemporalEyeActive()){
        diagnostic::temporal::Object object{};if(!ReadTemporalObject(self,object))object={};token=gStereoRuntime.BeginTemporalObject(object);
    }
    __try {return original(self,operation);}
    __finally {if(token!=UINT_MAX)gStereoRuntime.EndTemporalObject(token);}
}
bool ReadTemporalEntity(eCEntity* entity,diagnostic::temporal::InstanceEntity& out)
{
    __try {
        if(!entity||!diagnostic::Readable(entity,sizeof(void*)))return false;
        auto const& id=entity->GetID();if(!id.IsValid())return false;
        memcpy(out.id.value.data(),&id.GetCore(),16);out.id.value[4]=id.GetCount();
        memcpy(out.world.data(),&entity->GetWorldMatrix(),sizeof(float)*16);
        for(float f:out.world)if(!std::isfinite(f))return false;
        return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ReadTemporalContext(eCRenderContext* context,diagnostic::temporal::InstanceEntity& out)
{
    __try {return context&&diagnostic::Readable(context,sizeof(*context))&&ReadTemporalEntity(context->m_pEntity,out);}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
unsigned ReadTemporalInstanceArray(void* pool,diagnostic::temporal::InstanceEntity* output)
{
    using namespace diagnostic::temporal;
    __try {
        if(!diagnostic::Readable(pool,InstanceCountOffset+4))return 0;
        auto* bytes=static_cast<unsigned char*>(pool);uint32_t count=0;unsigned char* rows=nullptr;
        memcpy(&count,bytes+InstanceCountOffset,4);memcpy(&rows,bytes+InstanceArrayOffset,4);
        if(!count||count>1024||!diagnostic::Readable(rows,count*InstanceStride))return 0;
        // Preserve duplicate/invalid records rather than inventing row IDs.
        for(unsigned i=0;i<count;++i){eCEntity* entity=nullptr;memcpy(&entity,rows+i*InstanceStride,4);
            if(!ReadTemporalEntity(entity,output[i]))output[i]={};}
        return count;
    }__except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
bool TemporalGeometryActive(){return GetCurrentThreadId()==gApplicationThread.load()&&gNativeRenderScope&&gStereo.rendering&&gStereo.eyeActive&&gStereoRuntime.TemporalEyeActive();}
template<unsigned Index> GEBool __fastcall TemporalGeometryRender(eCRenderContext* self,void*,eCRenderOp2 const& operation)
{
    using Call=GEBool(__thiscall*)(eCRenderContext*,eCRenderOp2 const&);
    auto original=reinterpret_cast<Call>(gTemporalGeometryHooks[Index].original);unsigned token=UINT_MAX,materialToken=UINT_MAX;
    if constexpr(Index==0){
        auto cookie=reinterpret_cast<uintptr_t>(self);
        if(gStaticTrialHooksOptIn&&GetCurrentThreadId()==gApplicationThread.load()&&gNativeRenderScope&&gStereo.rendering&&gStereo.eyeActive&&gStereoRuntime.StaticTrialWantsContext(cookie)){
            g3vr::dx11::NativeStaticSnapshot snapshot;float view[16],projection[16];
            if(g3vr::dx11::CopyNativeStatic(self,snapshot)&&ReadStaticTrialCamera(view,projection))gStereoRuntime.ObserveStaticTrial(cookie,snapshot,view,projection);
        }
        if(gStaticTrialHooksOptIn&&GetCurrentThreadId()==gApplicationThread.load()&&gNativeRenderScope&&gStereo.rendering&&gStereo.eyeActive){
            char material[1024]{};if(gStereoRuntime.StaticTrialWantsMaterial(cookie))g3vr::dx11::CopyNativeMaterialName(operation,material);
            materialToken=gStereoRuntime.BeginStaticTrialMaterial(cookie,material);
        }
    }
    bool temporal=TemporalGeometryActive();if(!temporal&&materialToken==UINT_MAX)return original(self,operation);
    if(temporal){diagnostic::temporal::InstanceEntity entity{};bool valid=ReadTemporalContext(self,entity);token=gStereoRuntime.BeginTemporalInstances(valid?&entity:nullptr,valid?1:0);}
    __try {return original(self,operation);}
    __finally {gStereoRuntime.EndTemporalInstances(token);if constexpr(Index==0)gStereoRuntime.EndStaticTrialMaterial(materialToken);}
}
void __fastcall TemporalExecuteInstances(void* self,void*)
{
    auto original=reinterpret_cast<void(__thiscall*)(void*)>(gTemporalGeometryHooks[3].original);
    auto started=SceneExecutionStarted();
    __try {
        bool temporal=TemporalGeometryActive();
        bool material=gStaticTrialHooksOptIn&&GetCurrentThreadId()==gApplicationThread.load()&&gNativeRenderScope&&gStereo.rendering&&gStereo.eyeActive&&gStereoRuntime.StaticTrialMaterialObservationEnabled();
        if(!temporal&&!material){original(self);return;}
        // Allocation-free bounded stack copy. The native qsort may reorder these
        // records after entry; actual shader matrices determine each current row.
        diagnostic::temporal::InstanceEntity entities[1024];unsigned count=ReadTemporalInstanceArray(self,entities);
        unsigned materialToken=UINT_MAX;
        if(material){
            diagnostic::temporal::InstanceEntity const* selected=nullptr;
            for(unsigned i=0;i<count;++i)if(gStereoRuntime.StaticTrialWantsMaterialEntity(entities[i])){selected=&entities[i];break;}
            char name[1024]{};if(selected)g3vr::dx11::CopyNativePoolMaterialName(reinterpret_cast<eCVertexbufferPoolItemMain*>(self),name);
            auto lightmap=selected?g3vr::dx11::NativePoolLightmapSource(reinterpret_cast<eCVertexbufferPoolItemMain*>(self),selected->id.value):g3vr::dx11::NativeLightmapSource{};
            materialToken=gStereoRuntime.BeginStaticTrialDeferredMaterial(selected,name,lightmap);
        }
        auto token=temporal?gStereoRuntime.BeginTemporalInstances(entities,count):UINT_MAX;
        __try {original(self);}
        __finally {if(temporal)gStereoRuntime.EndTemporalInstances(token);gStereoRuntime.EndStaticTrialMaterial(materialToken);}
    }
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::SampledInstanceItem,started,gNativeRenderContext);}
}

void InstallTemporalPhaseCapture(HMODULE engine)
{
    if constexpr(!diagnostic::probes::StaticTrial)return;
    wchar_t staticMode[2]{};auto mode=diagnostic::NativeTrialMode(staticMode,GetEnvironmentVariableW(L"G3VR_DX11_STATIC_TRIAL",staticMode,2),diagnostic::probes::Dx11ValidationOnly);
    if constexpr(diagnostic::probes::Dx11ValidationOnly)if(mode!=5)return;
    gStaticTrialHooksOptIn=mode!=0;
    if(mode){char message[96];sprintf_s(message,"vr_dx11_native_hooks_opt_in mode=%u; runtime_and_hooks_share_mode_parser",mode);Message(message,0,true);}
    using namespace diagnostic::temporal;auto* base=reinterpret_cast<unsigned char*>(engine);
    auto* target=reinterpret_cast<void*>(GetProcAddress(engine,EndSolidSymbol));
    if(target!=base+EndSolidRva||!diagnostic::Readable(base+EndSolidCallRva,sizeof(EndSolidCall))||
       memcmp(base+EndSolidCallRva,EndSolidCall,sizeof(EndSolidCall))||
       !gTemporalSolidHook.Install(target,reinterpret_cast<void*>(&TemporalEndSolid),EndSolidPrefix,sizeof(EndSolidPrefix))){
        Message("vr_temporal_opaque_hook_refused; native_rendering_preserved",0,true);return;}
    gTemporalSolidCaller=reinterpret_cast<uintptr_t>(base)+EndSolidReturnRva;
    Message("vr_temporal_opaque_hook_ready; before_EndSolid_callbacks_and_dynamic_occlusion; opt_in_only",0,true);
    if constexpr(diagnostic::probes::Enabled){
    auto* animated=reinterpret_cast<void*>(GetProcAddress(engine,AnimatedRenderSymbol));
    if(animated!=base+AnimatedRenderRva||!gTemporalAnimationHook.Install(animated,reinterpret_cast<void*>(&TemporalAnimatedRender),AnimatedRenderPrefix,sizeof(AnimatedRenderPrefix)))
        Message("vr_temporal_animation_hook_refused; native_rendering_preserved",0,true);
    else Message("vr_temporal_animation_hook_ready; stable_entity_id; per_eye_palette_history; opt_in_only",0,true);
    }
    void* replacements[]={reinterpret_cast<void*>(&TemporalGeometryRender<0>),reinterpret_cast<void*>(&TemporalGeometryRender<1>),reinterpret_cast<void*>(&TemporalGeometryRender<2>),reinterpret_cast<void*>(&TemporalExecuteInstances)};
    for(unsigned i=0;i<4;++i){auto const& entry=GeometryRenderEntries[i];auto* address=reinterpret_cast<void*>(GetProcAddress(engine,entry.symbol));
        if constexpr(diagnostic::probes::Dx11ValidationOnly)if(i!=0&&i!=3)continue;
        if(address!=base+entry.rva||!gTemporalGeometryHooks[i].Install(address,replacements[i],entry.prefix,entry.size))
            Message("vr_temporal_geometry_hook_refused; native_rendering_preserved",i,true);
        else Message("vr_temporal_geometry_hook_ready; copied_entity_ids; opt_in_only",i,true);
    }
}
uint64_t SceneExecutionStarted()noexcept
{
    // Only the application render thread sets this TLS flag. Other threads
    // and unsampled frames forward without reading mutable global state.
    if(!gSceneExecutionSample)return 0;
    return gNativeRenderScope?diagnostic::probes::Now():0;
}
template<unsigned Index,class Result> Result __fastcall ProfileSceneExecutionResult(void* self,void*)
{
    auto started=SceneExecutionStarted();using Call=Result(__thiscall*)(void*);
    __try {return reinterpret_cast<Call>(gExecutionHooks[Index].original)(self);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::ExecutionEntries[Index].phase,started,gNativeRenderContext);}
}
void __fastcall ProfileSceneRenderOperation(void* self,void*,void const* database,bool solid)
{
    auto started=SceneExecutionStarted();using Call=void(__thiscall*)(void*,void const*,bool);
    __try {reinterpret_cast<Call>(gExecutionHooks[0].original)(self,database,solid);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::SampledRenderOperation,started,gNativeRenderContext);}
}
void __stdcall ProfileSceneInstanceBatches()
{
    auto started=SceneExecutionStarted();using Call=void(__stdcall*)();
    __try {reinterpret_cast<Call>(gExecutionHooks[1].original)();}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::SampledInstanceBatches,started,gNativeRenderContext);}
}
void __fastcall ProfileSceneDynamicOccludees(void* self,void*,void* items)
{
    auto started=SceneExecutionStarted();using Call=void(__thiscall*)(void*,void*);
    __try {reinterpret_cast<Call>(gExecutionHooks[7].original)(self,items);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::SampledDynamicOccludees,started,gNativeRenderContext);}
}
GEBool __fastcall ProfileSceneDecals(void* self,void*,void* database)
{
    auto started=SceneExecutionStarted();using Call=GEBool(__thiscall*)(void*,void*);
    __try {return reinterpret_cast<Call>(gExecutionHooks[8].original)(self,database);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::SampledDecals,started,gNativeRenderContext);}
}
template<unsigned Index> void __fastcall ProfileSceneExecutionVoid(void* self,void*)
{
    auto started=SceneExecutionStarted();using Call=void(__thiscall*)(void*);
    __try {reinterpret_cast<Call>(gExecutionHooks[Index].original)(self);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::ExecutionEntries[Index].phase,started,gNativeRenderContext);}
}
GEBool __fastcall ProfileSceneZPassBegin(void* self,void*,void* depth,GEBool pass)
{
    auto started=SceneExecutionStarted();using Call=GEBool(__thiscall*)(void*,void*,GEBool);
    __try {return reinterpret_cast<Call>(gExecutionHooks[11].original)(self,depth,pass);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::SampledZPassBegin,started,gNativeRenderContext);}
}
template<unsigned Index> GEBool __fastcall ProfileSceneZPass(void* self,void*,GEBool pass)
{
    auto started=SceneExecutionStarted();using Call=GEBool(__thiscall*)(void*,GEBool);
    __try {return reinterpret_cast<Call>(gExecutionHooks[Index].original)(self,pass);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::ExecutionEntries[Index].phase,started,gNativeRenderContext);}
}
void __fastcall ProfileSceneZOperation(void* self,void*,void const* database)
{
    auto started=SceneExecutionStarted();using Call=void(__thiscall*)(void*,void const*);
    __try {reinterpret_cast<Call>(gExecutionHooks[15].original)(self,database);}
    __finally {if(started)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::SampledRenderOperationZ,started,gNativeRenderContext);}
}
// Bind each added adapter to the local SDK's native signature at compile time.
static_assert(std::is_same<decltype(&eCShadowMapBase::RenderShadowMap),GEBool(eCShadowMapBase::*)()>::value);
static_assert(std::is_same<decltype(&eCCameraBase::RenderLowPoly),void(eCCameraBase::*)()>::value);
static_assert(std::is_same<decltype(&eCShaderMaterialAdmin::BeginZPass),GEBool(eCShaderMaterialAdmin::*)(eCDepthMap*,GEBool)>::value);
static_assert(std::is_same<decltype(&eCShaderMaterialAdmin::EndZPass),GEBool(eCShaderMaterialAdmin::*)()>::value);
static_assert(std::is_same<decltype(&eCRenderContextAnimation::RenderZPass),GEBool(eCRenderContextAnimation::*)(GEBool)>::value);
static_assert(std::is_same<decltype(&eCRenderContextMesh::RenderZPass),GEBool(eCRenderContextMesh::*)(GEBool)>::value);
static_assert(std::is_same<decltype(&eCRenderOp::ExecuteZPass),void(eCRenderOp::*)(eCFrustumDatabase const&)>::value);
static_assert(std::is_same<decltype(&eCModuleAdmin::DoBeginRenderSolid),void(eCModuleAdmin::*)()>::value);
void InstallSceneExecutionProfiler(HMODULE engine)
{
    using namespace diagnostic::performance;
    // The caller has already validated the entire native Engine.dll.
    void* replacements[]={reinterpret_cast<void*>(&ProfileSceneRenderOperation),reinterpret_cast<void*>(&ProfileSceneInstanceBatches),
        reinterpret_cast<void*>(&ProfileSceneExecutionResult<2,GEBool>),reinterpret_cast<void*>(&ProfileSceneExecutionResult<3,GEBool>),
        reinterpret_cast<void*>(&ProfileSceneExecutionResult<4,bEResult>),reinterpret_cast<void*>(&ProfileSceneExecutionResult<5,bEResult>),
        reinterpret_cast<void*>(&ProfileSceneExecutionResult<6,bEResult>),reinterpret_cast<void*>(&ProfileSceneDynamicOccludees),reinterpret_cast<void*>(&ProfileSceneDecals),
        reinterpret_cast<void*>(&ProfileSceneExecutionResult<9,GEBool>),reinterpret_cast<void*>(&ProfileSceneExecutionVoid<10>),reinterpret_cast<void*>(&ProfileSceneZPassBegin),
        reinterpret_cast<void*>(&ProfileSceneExecutionResult<12,GEBool>),reinterpret_cast<void*>(&ProfileSceneZPass<13>),reinterpret_cast<void*>(&ProfileSceneZPass<14>),
        reinterpret_cast<void*>(&ProfileSceneZOperation),reinterpret_cast<void*>(&ProfileSceneExecutionVoid<16>)};
    static_assert(sizeof(replacements)/sizeof(replacements[0])==ExecutionEntryCount);
    unsigned mask=0;
    for(unsigned i=0;i<ExecutionEntryCount;++i){auto const& entry=ExecutionEntries[i];auto* address=reinterpret_cast<void*>(GetProcAddress(engine,entry.symbol));
        unsigned char prefix[16]{};
        if(ExecutionPrefixAtBase(entry,uint32_t(reinterpret_cast<uintptr_t>(engine)),prefix)&&
           address==reinterpret_cast<unsigned char*>(engine)+entry.rva&&gExecutionHooks[i].Install(address,replacements[i],prefix,entry.bytes))mask|=1u<<i;
    }
    Message("native_scene_execution_profiler_mask; entries=17; period_stereo_frames=32; inclusive_sampled_scopes; no_render_work_skipped",mask,true);
}
void InstallNativeRenderProfiler(HMODULE engine)
{
    if constexpr(!diagnostic::probes::Enabled)return;
    // Caller already validated the entire Engine.dll hash. Every detour also
    // checks its exact symbol, RVA and whole position-independent prefix.
    using Phase=diagnostic::performance::NativePhase;
    gSceneProfileEnabled=diagnostic::performance::ProfilingOptIn(L"G3VR_SCENE_PROFILE");
    // Do not install even the legacy timing detours during ordinary play.
    // PUP scheduling, camera/arm hooks and temporal capture are separate.
    if(!gSceneProfileEnabled){Message("native_scene_profiler_opt_in; disabled; no_scene_or_crowd_timing_detours",0,true);return;}
    void* replacements[]={reinterpret_cast<void*>(&ProfileNativeFrustum<0>),reinterpret_cast<void*>(&ProfileNativeFrustum<1>),reinterpret_cast<void*>(&ProfileNativeFrustum<2>),reinterpret_cast<void*>(&ProfileNativeFrustum<3>),reinterpret_cast<void*>(&ProfileNativeRenderer),
        reinterpret_cast<void*>(&ProfileNativeFrustum<5,diagnostic::performance::NativePhase::FrustumRefresh>),reinterpret_cast<void*>(&ProfileNativePvsProcess),reinterpret_cast<void*>(&ProfileNativePvsExecute),
        reinterpret_cast<void*>(&ProfileNativeFrustum<8,Phase::PvsWait>),reinterpret_cast<void*>(&ProfileNativePvsGrid),reinterpret_cast<void*>(&ProfileNativePvsCells),
        reinterpret_cast<void*>(&ProfileNativeSceneVoid<11,Phase::CellHierarchy>),reinterpret_cast<void*>(&ProfileNativeSceneVoid<12,Phase::GeometryCacheIn>),reinterpret_cast<void*>(&ProfileNativeSceneVoid<13,Phase::GeometryCacheOut>),
        reinterpret_cast<void*>(&ProfileNativeSceneResult<14,Phase::GeometryReadNodes,bEResult>),reinterpret_cast<void*>(&ProfileNativeSceneResult<15,Phase::PoolQueueWait,GEBool>),reinterpret_cast<void*>(&ProfileNativeSceneResult<16,Phase::PoolIdleWait,GEBool>),
        reinterpret_cast<void*>(&ProfileNativeSceneUpdate<17,Phase::ModuleUpdate>),reinterpret_cast<void*>(&ProfileNativeSceneUpdate<18,Phase::EntityUpdate>),reinterpret_cast<void*>(&ProfileNativeSceneUpdate<19,Phase::CharacterAnimation>),
        reinterpret_cast<void*>(&ProfileNativeSceneUpdate<20,Phase::PhysicsSimulate>),reinterpret_cast<void*>(&ProfileNativeSceneUpdate<21,Phase::AnimationPhysics>)};
    static_assert(sizeof(replacements)/sizeof(replacements[0])==diagnostic::performance::RenderEntryCount);
    unsigned mask=0;
    for(unsigned i=0;i<diagnostic::performance::RenderEntryCount;++i){auto const& entry=diagnostic::performance::RenderEntries[i];auto target=reinterpret_cast<void*>(GetProcAddress(engine,entry.symbol));
        if(target==reinterpret_cast<unsigned char*>(engine)+entry.rva&&gNativeRenderHooks[i].Install(target,replacements[i],entry.prefix,entry.bytes))mask|=1u<<i;}
    Message("native_render_phase_profiler_mask; legacy_bits=0..8 scene_bits=9..16 simulation_bits=17..21; unknown_entries_unchanged",mask,true);
    InstallSceneExecutionProfiler(engine);
    HMODULE game=GetModuleHandleW(L"Game.dll");wchar_t path[MAX_PATH];unsigned char hash[32]{};
    if(!game||!GetModuleFileNameW(game,path,MAX_PATH)||!diagnostic::FileSha256(path,hash)||memcmp(hash,diagnostic::performance::GameSceneHash,32)){
        Message("native_crowd_profiler_unavailable; unknown_Game_profile",0,true);return;}
    void* gameReplacements[]={reinterpret_cast<void*>(&ProfileNativeSceneUpdate<0,Phase::AiRoutine,true>),reinterpret_cast<void*>(&ProfileNativeSceneUpdate<1,Phase::Navigation,true>)};
    static_assert(sizeof(gameReplacements)/sizeof(gameReplacements[0])==diagnostic::performance::GameSceneEntryCount);
    unsigned gameMask=0;
    for(unsigned i=0;i<diagnostic::performance::GameSceneEntryCount;++i){auto const& entry=diagnostic::performance::GameSceneEntries[i];auto target=reinterpret_cast<void*>(GetProcAddress(game,entry.symbol));
        if(target==reinterpret_cast<unsigned char*>(game)+entry.rva&&gGameSceneHooks[i].Install(target,gameReplacements[i],entry.prefix,entry.bytes))gameMask|=1u<<i;}
    Message("native_crowd_profiler_mask; bits=ai_routines,navigation; application_thread_only; no_simulation_skipped",gameMask,true);
}
bEResult __fastcall StereoOcclusionInfo(eCDX9Device* self,void*,GELPVoid handle,GEU32& samples,GEBool wait)
{
    using Call=bEResult(__thiscall*)(eCDX9Device*,GELPVoid,GEU32&,GEBool);
    auto original=reinterpret_cast<Call>(gOcclusionInfoHook.original);
    if(self!=gOcclusionDevice||!diagnostic::occlusion::Eligible(gOcclusionWorldEye,wait,
       reinterpret_cast<uintptr_t>(_ReturnAddress()),gOcclusionResolveReturn))return original(self,handle,samples,wait);
    __try {
        auto* query=diagnostic::occlusion::BorrowQuery(self,reinterpret_cast<uintptr_t>(handle));
        if(query){
            DWORD measured=0;auto result=diagnostic::occlusion::Poll(query,measured);
            if(result==diagnostic::occlusion::Result::Ready||result==diagnostic::occlusion::Result::Conservative){
                samples=measured;
                if(result==diagnostic::occlusion::Result::Ready)++gOcclusionStats.ready;else ++gOcclusionStats.conservative;
                // Returning success lets the ORIGINAL ResolveOCQuery recycle
                // this handle exactly once, through its existing native pool.
                return bEResult_Ok;
            }
            if(result==diagnostic::occlusion::Result::Error){++gOcclusionStats.errors;return bEResult_False;}
        }
    }__except(EXCEPTION_EXECUTE_HANDLER){++gOcclusionStats.errors;return bEResult_False;}
    ++gOcclusionStats.refused;return original(self,handle,samples,wait);
}
bool VerifiedPupOcclusionTable(void** table,void* nativeTarget)
{
    // InstallPupStereoCompatibility already checked the entire PUP FileHash,
    // pinned its module, and verified/installed the work hook. No arbitrary
    // import/jump chain or unknown derived device is accepted here.
    auto module=GetModuleHandleW(L"paru_s2.dll");if(!module||!gPupWorkHook.original)return false;
    using namespace diagnostic::pup;auto* base=reinterpret_cast<unsigned char*>(module);unsigned char thunk[6]{};
    if(table!=reinterpret_cast<void**>(base+OcclusionDeviceTableRva)||
       !diagnostic::Readable(table+diagnostic::occlusion::QueryInfoSlot,sizeof(void*))||
       table[diagnostic::occlusion::QueryInfoSlot]!=base+OcclusionThunkRva||
       !OcclusionThunkAtBase(uint32_t(reinterpret_cast<uintptr_t>(base)),thunk)||
       !diagnostic::Readable(base+OcclusionThunkRva,sizeof(thunk))||memcmp(base+OcclusionThunkRva,thunk,sizeof(thunk))||
       !diagnostic::Readable(base+OcclusionIatRva,sizeof(void*)))return false;
    void* imported=nullptr;memcpy(&imported,base+OcclusionIatRva,sizeof(imported));return imported==nativeTarget;
}
void InstallStereoOcclusion(HMODULE engine)
{
    if constexpr(!diagnostic::probes::Enabled)return;
    if(gOcclusionInstallFinished)return;
    wchar_t mode[8]{};DWORD length=GetEnvironmentVariableW(L"G3VR_ASYNC_OCCLUSION",mode,8);
    if(!length||length>=8||!diagnostic::occlusion::PendingVisibleOptIn(mode)){
        gOcclusionInstallFinished=true;
        Message("vr_occlusion_native_results; pending_visible_shortcut_disabled; same_visibility_policy_for_world_and_menus; distances_unchanged",0,true);return;
    }
    Message("vr_occlusion_pending_visible_experimental_opt_in; may_increase_hidden_geometry_cost",0,true);
    // Engine whole-file hash is already verified by InstallStereoHooks.
    using namespace diagnostic::occlusion;auto* base=reinterpret_cast<unsigned char*>(engine);
    auto* target=reinterpret_cast<void*>(GetProcAddress(engine,QueryInfoSymbol));auto* device=DeviceAccess::sGetAPIDevice();size_t slot=0;
    if(!device)return; // A native API device may not exist during script startup.
    gOcclusionInstallFinished=true;
    unsigned char call[]={0xff,0xd2,0x83,0xf8,0x01};
    if(!diagnostic::Readable(device,sizeof(eCDX9Device))||
       target!=base+QueryInfoRva||!diagnostic::Readable(target,sizeof(QueryInfoPrefix))||memcmp(target,QueryInfoPrefix,sizeof(QueryInfoPrefix))||
       !diagnostic::Readable(base+ResolveReturnRva-2,sizeof(call))||memcmp(base+ResolveReturnRva-2,call,sizeof(call))||
       !diagnostic::VirtualSlot(&eCDX9Device::GetOcclusionTestInfo,slot)||slot!=QueryInfoSlot){Message("vr_occlusion_nonblocking_refused; native_profile_or_SDK_slot; native_wait_preserved",1,true);return;}
    auto** table=*reinterpret_cast<void***>(device);
    bool native=table==reinterpret_cast<void**>(base+DeviceTableRva)&&diagnostic::Readable(table+slot,sizeof(void*))&&table[slot]==target;
    bool pup=!native&&VerifiedPupOcclusionTable(table,target);
    if(!native&&!pup){Message("vr_occlusion_nonblocking_refused; unsupported_device_table; native_wait_preserved",2,true);return;}
    if(!gOcclusionInfoHook.Prepare(table,slot,reinterpret_cast<void*>(&StereoOcclusionInfo))||!gOcclusionInfoHook.Install()){
        gOcclusionInfoHook.Restore();Message("vr_occlusion_nonblocking_refused; virtual_slot_install_failed; native_wait_preserved",3,true);return;}
    gOcclusionDevice=reinterpret_cast<eCDX9Device*>(device);gOcclusionResolveReturn=reinterpret_cast<uintptr_t>(base)+ResolveReturnRva;
    Message("vr_occlusion_nonblocking_ready; world_eye_only; ready_results_preserved; pending_visible; native_query_pool; copy_fences_unchanged; pup_device_code",pup?1:0,true);
}
#ifdef G3VR_RENDERER_FEASIBILITY
diagnostic::FocusHook gRendererTrialHooks[12];
thread_local void const* gRendererTrialDatabase=nullptr;
thread_local bool gRendererTrialOpaque=false;
thread_local unsigned gRendererTrialShadow=0;
thread_local unsigned gRendererTrialEye=2;
thread_local unsigned gRendererTrialExecuteDepth=0;
thread_local bool gRendererTrialSolidPhase=false;
bool RendererTrialScope()noexcept {
    return GetCurrentThreadId()==gApplicationThread.load()&&diagnostic::renderer_trial::experiment.Active()&&gNativeRenderScope&&gStereo.rendering&&gStereo.eyeActive&&gNativeRenderContext==1;
}
template<unsigned Index,diagnostic::renderer_trial::Scope Scope> void __fastcall RendererTrialVoid(void* self,void*) {
    bool active=RendererTrialScope();auto started=active?diagnostic::performance::Now():0;
    auto* previous=gRendererTrialDatabase;bool previousSolid=gRendererTrialSolidPhase;
    if constexpr(Index==3)if(active){++gRendererTrialExecuteDepth;if(!previous){gRendererTrialDatabase=self;gRendererTrialSolidPhase=false;}}
    __try {reinterpret_cast<void(__thiscall*)(void*)>(gRendererTrialHooks[Index].original)(self);}
    __finally {
        if constexpr(Index==3)if(active){--gRendererTrialExecuteDepth;gRendererTrialDatabase=previous;gRendererTrialSolidPhase=previousSolid;}
        if(started)diagnostic::renderer_trial::experiment.ScopeTime(Scope,started,diagnostic::performance::Now());
    }
}
void __fastcall RendererTrialSystem(eCRenderSystemAdmin* self,void*,eCFrustumDatabase& database){
    auto started=RendererTrialScope()?diagnostic::performance::Now():0;
    __try{reinterpret_cast<void(__thiscall*)(eCRenderSystemAdmin*,eCFrustumDatabase&)>(gRendererTrialHooks[4].original)(self,database);}
    __finally{if(started)diagnostic::renderer_trial::experiment.ScopeTime(diagnostic::renderer_trial::Scope::RenderSystem,started,diagnostic::performance::Now());}
}
void __fastcall RendererTrialOperation(eCRenderOp2* self,void*,eCFrustumDatabase const& database,bool immediate){
    auto started=RendererTrialScope()?diagnostic::performance::Now():0;bool previous=gRendererTrialOpaque;
    // Native Render's bool is immediate submission, NOT an opaque-pass flag.
    // Main Execute passes false even for opaque objects, which queue instances.
    gRendererTrialOpaque=diagnostic::renderer_trial::EligibleColor(started!=0,gRendererTrialDatabase==&database,gRendererTrialSolidPhase,gRendererTrialShadow,gRendererTrialExecuteDepth);
    // Preserve the original operation's processed flag, Begin/EndMaterial,
    // culling/mip state and cleanup. Only its static mesh callback can omit work.
    __try{reinterpret_cast<void(__thiscall*)(eCRenderOp2*,eCFrustumDatabase const&,bool)>(gRendererTrialHooks[5].original)(self,database,immediate);}
    __finally{gRendererTrialOpaque=previous;if(started)diagnostic::renderer_trial::experiment.ScopeTime(diagnostic::renderer_trial::Scope::Operation,started,diagnostic::performance::Now());}
}
bool RendererTrialStatic(eCRenderContext* self)noexcept{
    // This pointer is supplied to the verified native mesh Render entry.
    // No string lookup, RTTI scan, retained entity pointer or SDK mutation.
    __try{return self&&self->m_pEntity&&!self->m_pEntity->GetEntityFlags().Dynamic;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
GEBool __fastcall RendererTrialMesh(eCRenderContext* self,void*,eCRenderOp2 const& operation){
    using Call=GEBool(__thiscall*)(eCRenderContext*,eCRenderOp2 const&);
    bool active=RendererTrialScope(),target=active&&gRendererTrialOpaque&&!gRendererTrialShadow&&RendererTrialStatic(self);
    auto started=active?diagnostic::performance::Now():0;
    bool omit=target&&diagnostic::renderer_trial::experiment.Omitting();
    if(target)diagnostic::renderer_trial::experiment.Target(gRendererTrialEye,omit);
    __try{
        // CP1.75.14 mesh Render only submits Execute/AddInstance and returns
        // true. Do not suppress ExecuteAllInstances: it MUST drain its queue.
        if(omit)return GETrue;
        return reinterpret_cast<Call>(gRendererTrialHooks[6].original)(self,operation);
    }__finally{if(started){auto ended=diagnostic::performance::Now();diagnostic::renderer_trial::experiment.ScopeTime(diagnostic::renderer_trial::Scope::Mesh,started,ended);
        if(target)diagnostic::renderer_trial::experiment.ScopeTime(diagnostic::renderer_trial::Scope::TargetMesh,started,ended);}}
}
GEBool __fastcall RendererTrialShadow(void* self,void*){
    bool active=RendererTrialScope();auto started=active?diagnostic::performance::Now():0;bool previous=gRendererTrialOpaque;
    ++gRendererTrialShadow;gRendererTrialOpaque=false;
    __try{return reinterpret_cast<GEBool(__thiscall*)(void*)>(gRendererTrialHooks[7].original)(self);}
    __finally{--gRendererTrialShadow;gRendererTrialOpaque=previous;if(started)diagnostic::renderer_trial::experiment.ScopeTime(diagnostic::renderer_trial::Scope::Shadow,started,diagnostic::performance::Now());}
}
void __stdcall RendererTrialInstances(){
    auto started=RendererTrialScope()?diagnostic::performance::Now():0;
    __try{reinterpret_cast<void(__stdcall*)()>(gRendererTrialHooks[8].original)();}
    __finally{if(started)diagnostic::renderer_trial::experiment.ScopeTime(diagnostic::renderer_trial::Scope::Instances,started,diagnostic::performance::Now());}
}
GEBool __fastcall RendererTrialPvs(eCPVSPrefetcher3* self,void*,eCFrustumDatabase& database){
    auto started=RendererTrialScope()?diagnostic::performance::Now():0;
    __try{return reinterpret_cast<GEBool(__thiscall*)(eCPVSPrefetcher3*,eCFrustumDatabase&)>(gRendererTrialHooks[9].original)(self,database);}
    __finally{if(started)diagnostic::renderer_trial::experiment.ScopeTime(diagnostic::renderer_trial::Scope::Pvs,started,diagnostic::performance::Now());}
}
template<unsigned Index> void __fastcall RendererTrialSolidCallback(void* self,void*){
    if(RendererTrialScope()&&gRendererTrialDatabase&&gRendererTrialExecuteDepth==1&&!gRendererTrialShadow)
        gRendererTrialSolidPhase=Index==10;
    reinterpret_cast<void(__thiscall*)(void*)>(gRendererTrialHooks[Index].original)(self);
}
void InstallRendererFeasibility(HMODULE engine){
    wchar_t enabled[4]{};if(GetEnvironmentVariableW(L"G3VR_RENDERER_FEASIBILITY",enabled,4)!=1||enabled[0]!=L'1')return;
    using namespace diagnostic::performance;
    void* replacements[]={reinterpret_cast<void*>(&RendererTrialVoid<0,diagnostic::renderer_trial::Scope::Update>),reinterpret_cast<void*>(&RendererTrialVoid<1,diagnostic::renderer_trial::Scope::Extract>),
        reinterpret_cast<void*>(&RendererTrialVoid<2,diagnostic::renderer_trial::Scope::OcclusionExtract>),reinterpret_cast<void*>(&RendererTrialVoid<3,diagnostic::renderer_trial::Scope::Execute>),
        reinterpret_cast<void*>(&RendererTrialSystem),reinterpret_cast<void*>(&RendererTrialOperation),reinterpret_cast<void*>(&RendererTrialMesh),reinterpret_cast<void*>(&RendererTrialShadow),reinterpret_cast<void*>(&RendererTrialInstances),reinterpret_cast<void*>(&RendererTrialPvs),
        reinterpret_cast<void*>(&RendererTrialSolidCallback<10>),reinterpret_cast<void*>(&RendererTrialSolidCallback<11>)};
    unsigned mask=0;
    for(unsigned i=0;i<12;++i){
        char const* symbol=nullptr;unsigned rva=0,bytes=0;unsigned char prefix[16]{};
        if(i<5||i==9){auto const& e=RenderEntries[i==9?6:i];symbol=e.symbol;rva=e.rva;bytes=e.bytes;memcpy(prefix,e.prefix,bytes);}
        else if(i==6){auto const& e=diagnostic::temporal::GeometryRenderEntries[0];symbol=e.symbol;rva=e.rva;bytes=e.size;memcpy(prefix,e.prefix,bytes);}
        else if(i==11){symbol=diagnostic::temporal::EndSolidSymbol;rva=diagnostic::temporal::EndSolidRva;bytes=sizeof(diagnostic::temporal::EndSolidPrefix);memcpy(prefix,diagnostic::temporal::EndSolidPrefix,bytes);}
        else {auto const& e=ExecutionEntries[i==5?0:i==7?9:i==10?16:1];symbol=e.symbol;rva=e.rva;bytes=e.bytes;if(!ExecutionPrefixAtBase(e,uint32_t(reinterpret_cast<uintptr_t>(engine)),prefix))continue;}
        auto* target=reinterpret_cast<void*>(GetProcAddress(engine,symbol));
        if(target==reinterpret_cast<unsigned char*>(engine)+rva&&gRendererTrialHooks[i].Install(target,replacements[i],prefix,bytes))mask|=1u<<i;
    }
    diagnostic::renderer_trial::experiment.SetHooks(mask);
    Message("renderer_feasibility_hooks; bounded_static_color_omission; native_depth_queues_and_simulation_preserved",mask,true);
}
#endif
#ifdef G3VR_SCENE_COST_TRIAL
#include "SceneCostTrialHooks.inl"
#endif
void __fastcall StereoCameraRender(eCCameraBase* camera,void*,bool allowThreading,bool update)
{
    using Render=void(__thiscall*)(eCCameraBase*,bool,bool);auto original=reinterpret_cast<Render>(gCameraRenderHook.original);
    if(!gOcclusionInstallFinished)InstallStereoOcclusion(GetModuleHandleW(L"Engine.dll"));
    if(gStereo.rendering || gStereo.uiRendering){original(camera,allowThreading,update);return;}
    gFlameTargets.Reset();
    if(!gHeadControl.enabled || !gConfig.stereoEnabled){
#ifdef G3VR_NAVIGATION_BUDGET_RUNTIME
        gNavigationBudget.Native();
#endif
        original(camera,allowThreading,update);return;
    }
    diagnostic::StereoFrame frame;bool ready=false;
    if(gCinemaScope.Active(GetCurrentThreadId())){original(camera,allowThreading,update);return;}
    __try {ready=PrepareStereoFrame(camera,frame);}
    __except(EXCEPTION_EXECUTE_HANDLER){StereoFailure("stereo_prepare_SEH_fault");}
    if(!ready){
#ifdef G3VR_NAVIGATION_BUDGET_RUNTIME
        gNavigationBudget.Native();
#endif
        original(camera,allowThreading,update);return;}
#ifdef G3VR_NAVIGATION_BUDGET_RUNTIME
    gNavigationBudget.Frame(gApplication&&gApplication->IsGameRunning()&&!gWorldLifecycle.Blocked(),GetTickCount64());
#endif
    // The verified native camera writes its FIRST bool to the engine pool.
    // Forcing false here made PVS jobs synchronous and PUP drain unboundedly
    // in both eyes. Preserve the caller's allowance only for the supported PUP
    // profile. Native camera-distance checks can still force synchronous work.
    bool pupThreading=gPupWorkHook.original!=nullptr;
    bool savedPoolThreading=pupThreading&&eCThreadPool::IsThreadingEnabled();
    bool eyeThreading=pupThreading&&allowThreading;
    CameraSnapshot saved;SaveStereoCamera(*camera,saved);memcpy(gStereo.nativePosition,saved.position,sizeof(saved.position));memcpy(gStereo.nativeRotation,saved.rotation,sizeof(saved.rotation));gStereo.camera=camera;gStereo.rendering=true;++gPupStereoEpoch;
#ifdef G3VR_SCENE_COST_TRIAL
    diagnostic::scene_cost::trial.Pose(saved.position,saved.rotation,gConfig.unitsPerMeter);
    auto costStereoStarted=CostStart();
#ifdef G3VR_DRAW_COST_TRIAL
    DrawCostGpuFrame();
#endif
#endif
#ifdef G3VR_RENDERER_FEASIBILITY
    bool trialEligible=gApplication->IsGameRunning()&&gStereo.firstPerson&&!MenuPanelActive()&&!gRadialOpen&&!gWorldLifecycle.Blocked()&&!gCSession::GetInstance().IsPaused();
    diagnostic::renderer_trial::experiment.Begin(diagnostic::performance::Now(),gPupStereoEpoch,trialEligible);
    diagnostic::renderer_trial::experiment.Camera(saved.position,gConfig.unitsPerMeter,saved.rotation,frame.head.position,frame.head.quaternion);
#endif
    bool previousTrackedThread=gTrackedRenderThread;gTrackedRenderThread=true;unsigned previousPupEye=gPupStereoEye;gPupStereoEye=0;
    bool complete=true;HeadVisibilitySnapshot headVisibility;diagnostic::RenderAlphaSnapshot playerAlpha;diagnostic::visual::MenuWeaponScope menuStaff,stowedWeapons;
    uint64_t armsStarted=diagnostic::probes::Now();
    __try
    {
        gChestVisual.Begin(reinterpret_cast<eCEntity*>(gStereo.player),gStereo.chest,gStereo.firstPerson&&!gStereo.dialog);
        gTrackedArms.SetBodyVisibility(gStereo.firstPerson,gVrSettings.handsOnly);
        BeginPlayerRenderAlpha(playerAlpha);
        BeginPlayerHeadVisibility(headVisibility);
        gTrackedArms.PrepareBodyPosture(reinterpret_cast<eCEntity*>(gStereo.player),
            gArmPaletteHook.original&&gVrSettings.controllers&&gVrSettings.trackedArms&&gVrSettings.stableBody&&gVrSettings.stableHorizon&&gStereo.firstPerson,
            GetTickCount64(),gConfig.unitsPerMeter,TrackedArmsRecord);
        gTrackedArms.PreparePhysicalCrouch(reinterpret_cast<eCEntity*>(gStereo.player),
            gVrSettings.physicalCrouching&&gVrSettings.controllers&&gVrSettings.trackedArms&&gVrSettings.stableBody&&gVrSettings.stableHorizon&&gStereo.firstPerson&&gHeadControl.centered&&!gHeadControl.recenterPending,
            frame.head,gStereo.centerPosition[1],GetTickCount64());
        if(gTrackedArms.PhysicalHandsOnly()&&!gStereo.hidePlayerHead){gStereo.hidePlayerHead=true;BeginPlayerHeadVisibility(headVisibility);gStereo.hidePlayerHead=false;}
        PrepareComfortCameraBase(saved,frame.head);
        BeginTrackedArms(saved,frame.head);
        BeginMenuStaffVisibility(menuStaff);
        // Ranged mesh/support geometry must exist before freezing the hero's
        // palette. Otherwise the frozen early return reuses a free left hand
        // for every mesh/eye even though the support grip is still latched.
        __try {if(!gRadialOpen&&!MenuPanelActive()){
            gTrackedBow.Begin(gTrackedArms.BowSample(),gBow.draw.pulling?gBow.draw.power:0.f,TrackedArmsRecord);
            diagnostic::arms::Vec nock,drawn;diagnostic::arms::Mat arrow;if(gTrackedBow.Geometry(nock,arrow,drawn))gTrackedArms.SetBowGeometry(nock,arrow,drawn);
            diagnostic::arms::Mat mesh;diagnostic::arms::Vec contact;if(gTrackedBow.SupportGeometry(mesh,contact))gTrackedArms.SetCrossbowGeometry(mesh,contact);
        }}
        __except(EXCEPTION_EXECUTE_HANDLER){RestoreTrackedArms();gTrackedBow.Fail("begin");Message("vr_bow_palette_prepare_fault; ranged_binding_retry_scheduled",GetExceptionCode(),true);}
        __try{gTrackedArms.PrepareFrozenFrame(gArmPaletteHook.original,eCApplication::GetInstance().GetMasterFrameCounter());}
        __except(EXCEPTION_EXECUTE_HANDLER){RestoreTrackedArms(true);Message("vr_2h_frozen_frame_refused; native_pose_preserved",GetExceptionCode(),true);}
        __try {if(gMagicHandLayoutReady&&!gRadialOpen&&!MenuPanelActive()&&gStereo.firstPerson&&!gTrackedArms.fault){
            if(!diagnostic::visual::CaptureMagicHands(gCSession::GetInstance().GetPlayer(),MappedControllers(),GetTickCount64(),gMagicHandEffects,gTorchParticles)){
                gMagicHandEffects.Reset();gTorchParticles.Reset();Message("vr_magic_hand_effect_capture_refused; native_effects_preserved",0,true);
            }
        }}__except(EXCEPTION_EXECUTE_HANDLER){gMagicHandEffects.Reset();gTorchParticles.Reset();gMagicHandLayoutReady=false;Message("vr_magic_hand_effect_capture_fault; native_effects_preserved",GetExceptionCode(),true);}
        __try {gBowReticle.Prepare(gMagic.aim.sample.valid?gMagic.aim.sample:gTrackedArms.BowSample(),gVrSettings.bowReticle&&gBow.ready&&!gBow.fault&&!gRadialOpen&&!MenuPanelActive());}
        __except(EXCEPTION_EXECUTE_HANDLER){gBowReticle.fault=true;gBowReticle.Reset();Message("vr_bow_reticle_prepare_fault; reticle_disabled",GetExceptionCode(),true);}
        BeginStowedWeaponVisibility(stowedWeapons);
        gStereoRuntime.NoteTiming(diagnostic::performance::Stage::Arms,armsStarted);
        for(unsigned eye=0;eye<2;++eye)
        {
            gPupStereoEye=eye;
#ifdef G3VR_SCENE_COST_TRIAL
            gCostLane=eye+1;
#endif
#ifdef G3VR_RENDERER_FEASIBILITY
            gRendererTrialEye=eye;
#endif
            uint64_t eyePrepareStarted=diagnostic::probes::Now();
            gNativeRenderContext=gApplication->IsGameRunning()?(MenuPanelActive()?2:1):0;
            gStereoRuntime.PrepareTemporalEye(eye,gNativeRenderContext);
            gStereo.eyeActive=true;
            if(!SetStereoEye(*camera,saved,frame.eyes[eye],eye)){complete=false;break;}
            if(gLocatorNativeInput.projectionHook.installed)for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)gLocatorEyeView[eye][r*4+c]=camera->GetViewMatrix()(r,c);
            if(playerAlpha.active){if(gStereo.chest)playerAlpha.Hide();else playerAlpha.Opaque();}
            gTrackedArms.PrepareEye();
            __try {diagnostic::arms::Mat handDelta[2];bool handReady[2]{};gTrackedArms.RenderHandDeltas(handDelta,handReady);gMagicHandEffects.Apply(handDelta,handReady);}
            __except(EXCEPTION_EXECUTE_HANDLER){gMagicHandEffects.Restore();gMagicHandLayoutReady=false;Message("vr_magic_hand_effect_pose_refused; native_effects_preserved",GetExceptionCode(),true);}
            __try {gTrackedBow.PrepareEye();}
            __except(EXCEPTION_EXECUTE_HANDLER){RestoreTrackedArms();gTrackedBow.Fail("prepare_eye");Message("vr_bow_spatial_prepare_fault; ranged_binding_retry_scheduled",GetExceptionCode(),true);}
            // Apply after palette/attachment preparation, for both eyes. Native
            // running/sheathing can push back equipment through the VR camera.
            // Leave drawn weapons and all third-person states visible.
            if(stowedWeapons.Count())stowedWeapons.Hide();
            // Rebuild the frustum for each eye, including when native temporal
            // culling would otherwise reuse the preceding eye's render list.
            gStereoRuntime.NoteTiming(diagnostic::performance::Stage::Arms,eyePrepareStarted);
            uint64_t renderStarted=diagnostic::probes::Now();
            gStereoRuntime.BeginGpuEye(eye,gNativeRenderContext);
#ifdef G3VR_STEREO_FRAME_CAPTURE
            {float view[16],projection[16];for(int row=0;row<4;++row)for(int col=0;col<4;++col){view[row*4+col]=camera->GetViewMatrix()(row,col);projection[row*4+col]=camera->GetProjectionMatrix()(row,col);}gStereoRuntime.CaptureCamera(eye,view,projection);}
#endif
            if(gStereoRuntime.TemporalEyeActive()){
                float temporalView[16],temporalProjection[16];
                for(int row=0;row<4;++row)for(int col=0;col<4;++col){temporalView[row*4+col]=camera->GetViewMatrix()(row,col);temporalProjection[row*4+col]=camera->GetProjectionMatrix()(row,col);}
                gStereoRuntime.RecordTemporalCamera(temporalView,temporalProjection);
            }
            gSceneExecutionSample=gSceneProfileEnabled&&diagnostic::performance::SampleSceneExecution(gPupStereoEpoch,gNativeRenderContext,gStereoRuntime.TemporalDiagnosticActive());
            gNativeRenderScope=true;
            if(pupThreading&&gNativeRenderContext==1){if(eyeThreading)++gPupStats.eyesAllowed;else ++gPupStats.eyesSync;}
            gOcclusionWorldEye=gNativeRenderContext==1&&!gStereoRuntime.TemporalDiagnosticActive();
            __try {original(camera,eyeThreading,true);++gStereo.eyeDraws[eye];
#ifdef G3VR_SCENE_COST_TRIAL
                diagnostic::scene_cost::trial.Eye(eye);
#endif
            }
            __finally {gOcclusionWorldEye=false;gNativeRenderScope=false;gSceneExecutionSample=false;gStereoRuntime.EndGpuEye(eye);}
            gStereoRuntime.NoteTiming(eye?diagnostic::performance::Stage::RightRender:diagnostic::performance::Stage::LeftRender,renderStarted);
            __try {if(gVrSettings.handHudEnabled&&!gWorldLifecycle.Blocked()){
                diagnostic::arms::Mat poses[2];bool available[2]{};
                for(unsigned h=0;h<2;++h){diagnostic::arms::Mat lower,bind,hand,calibration,anchor;
                    available[h]=gTrackedArms.LowerArmHud(h,lower,bind,hand,calibration)&&
                        diagnostic::HandHudAnchor(bind,hand,calibration,h,anchor)&&
                        diagnostic::HandHudPose(anchor,lower,gVrSettings.numbers+87+h*6,gConfig.unitsPerMeter,poses[h]);}
                auto* wrapper=DeviceAccess::sGetAPIDevice();if(diagnostic::Readable(wrapper,sizeof(eCDX9Device))){
                    float view[16],projection[16];for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){view[r*4+c]=camera->GetViewMatrix()(r,c);projection[r*4+c]=camera->GetProjectionMatrix()(r,c);}
                    gHandHud.Draw(reinterpret_cast<eCDX9Device*>(wrapper)->m_pDevice,view,projection,poses,available,gConfig.unitsPerMeter,gVrSettings.numbers+99);
                }}}
            __except(EXCEPTION_EXECUTE_HANDLER){gHandHud.Reset();Message("vr_hand_hud_unavailable",GetExceptionCode(),true);}
            RestoreTrackedArms();
            __try {auto* wrapper=DeviceAccess::sGetAPIDevice();if(diagnostic::Readable(wrapper,sizeof(eCDX9Device))){
                float reticleProjection[16];for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)reticleProjection[row*4+col]=camera->GetProjectionMatrix()(row,col);
                gStereoRuntime.UnjitterTemporalProjection(eye,reticleProjection);
                gBowReticle.Draw(reinterpret_cast<eCDX9Device*>(wrapper)->m_pDevice,*camera,eye,reticleProjection);
                gStereoRuntime.ProtectTemporalReticle(eye,gBowReticle.visible[eye],gBowReticle.lastU[eye],gBowReticle.lastV[eye]);}}
            __except(EXCEPTION_EXECUTE_HANDLER){gBowReticle.fault=true;gBowReticle.Reset();Message("vr_bow_reticle_draw_fault; reticle_disabled",GetExceptionCode(),true);}
            if(gSample.load())
            {
                Event event=Record(Kind::StereoEye,int(eye));auto const& p=camera->GetPosition();event.position[0]=p.m_fX;event.position[1]=p.m_fY;event.position[2]=p.m_fZ;
                auto const& r=camera->GetRotation();event.rotation[0]=r.m_fX;event.rotation[1]=r.m_fY;event.rotation[2]=r.m_fZ;
                for(int row=0;row<4;++row)for(int col=0;col<4;++col)event.projection[row*4+col]=camera->GetProjectionMatrix()(row,col);
                event.counts[0]=gStereo.eyeDraws[eye];Push(event);
            }
            if(!gStereoRuntime.Capture(eye)){complete=false;break;}
#ifdef G3VR_RENDERER_FEASIBILITY
            diagnostic::renderer_trial::experiment.Eye(eye);
#endif
        }
    }
    __finally
    {
        // Clear before cleanup too, so a cleanup fault cannot leave a stale
        // tracked-render context on this thread after returning to the engine.
        gTrackedRenderThread=previousTrackedThread;gPupStereoEye=previousPupEye;
        // Eye rendering may temporarily disable threading for a camera jump.
        // Restore the pre-stereo pool state, also on partial-frame failure.
        if(pupThreading)eCThreadPool::EnableAllThreads(savedPoolThreading);
#ifdef G3VR_SCENE_COST_TRIAL
        gCostLane=0;
#endif
        gHandHud.EndFrame();RestoreTrackedArms(true);
        if(gBowReticle.Active()&&gSample.load()){Event event=Record(Kind::Message);sprintf_s(event.text,"vr_ranged_reticle eye_visible=%d,%d uv=%.3f,%.3f/%.3f,%.3f frames=%llu",gBowReticle.visible[0],gBowReticle.visible[1],gBowReticle.lastU[0],gBowReticle.lastV[0],gBowReticle.lastU[1],gBowReticle.lastV[1],gBowReticle.frames);Push(event);}
        unsigned menuStaffHidden=menuStaff.Count();if(gWorldLifecycle.Blocked())menuStaff.Abandon();else menuStaff.Restore();
        if(gWorldLifecycle.Blocked())stowedWeapons.Abandon();else stowedWeapons.Restore();
        playerAlpha.Restore();
        headVisibility.visibility.Restore();
        static uint64_t nextEffectsLog=0;
        if(diagnostic::probes::Enabled&&GetTickCount64()>=nextEffectsLog&&(menuStaffHidden||gFlameProcessSkipped||gFlameDrawSkipped)){
            nextEffectsLog=GetTickCount64()+3000;char effectsText[180];
            sprintf_s(effectsText,"vr_flame_layers world_process_skipped=%llu world_draws_skipped=%llu menu_staff_hidden=%u; local_flame_preserved=1 bounded_target_cache=1",gFlameProcessSkipped,gFlameDrawSkipped,menuStaffHidden);Message(effectsText,0,true);}

        if(complete&&gLocatorNativeInput.projectionHook.installed&&!MenuPanelActive()&&!gRadialOpen){unsigned width=0,height=0;auto panel=diagnostic::PanelLayout(gVrSettings,false);
            if(gLocatorNativeInput.Viewport(width,height))gLocatorProjection.Publish(gLocatorEyeView[0],gLocatorEyeView[1],width,height,panel.widthMeters,panel.distanceMeters,panel.verticalMeters,GetTickCount64());else gLocatorProjection.Reset();
        }else gLocatorProjection.Reset();
        RestoreStereoCamera(*camera,saved);gStereo.camera=nullptr;gStereo.rendering=false;gStereo.basePrepared=false;
#ifdef G3VR_SCENE_COST_TRIAL
        CostEnd(unsigned(diagnostic::scene_cost::Scope::Stereo),0,costStereoStarted);
#ifdef G3VR_DRAW_COST_TRIAL
        gDrawCollector.EndGpuFrame();
#endif
#endif
        if(diagnostic::probes::Enabled&&gPupWorkHook.original&&GetTickCount64()>=gPupStats.nextLog){gPupStats.nextLog=GetTickCount64()+2000;
            char text[192];std::snprintf(text,sizeof(text),"vr_pup_prefetch calls=%llu run=%llu defer_busy=%llu defer_budget=%llu bypass=%llu work_max_ms=%.3f; stereo_budget_ms=2",
                gPupStats.calls,gPupStats.run,gPupStats.busy,gPupStats.budget,gPupStats.bypass,double(gPupStats.maximum)*1000/diagnostic::performance::Frequency());Message(text,0,true);
            std::snprintf(text,sizeof(text),"vr_pup_threading eyes_allow=%llu eyes_sync=%llu forced=%llu optional=%llu; context=1 saved_pool_restored=1",
                gPupStats.eyesAllowed,gPupStats.eyesSync,gPupStats.worldForced,gPupStats.worldOptional);Message(text,0,true);}
        if(gOcclusionInfoHook.installed&&GetTickCount64()>=gOcclusionStats.nextLog){gOcclusionStats.nextLog=GetTickCount64()+2000;
            char text[192];sprintf_s(text,"vr_occlusion_nonblocking ready=%llu conservative=%llu errors=%llu refused=%llu; no_flush_no_spin; native_pool_owner=1",gOcclusionStats.ready,gOcclusionStats.conservative,gOcclusionStats.errors,gOcclusionStats.refused);Message(text,0,true);}
        if(AbnormalTermination()){gStereo.framePending=false;gStereoRuntime.Cancel();}
    }
    if(!complete){gStereoRuntime.Cancel();StereoFailure(gStereoRuntime.Error()[0]?gStereoRuntime.Error():"eye_camera_or_projection_validation_failed");original(camera,allowThreading,update);return;}
    // Native OnPostRender contains OnRenderDraw. Submit after that entire
    // phase, when the HUD and menu commands have also reached the UI layer.
    gStereo.framePending=true;
}
int NativeUiState()
{
    if(!gApplication||!gApplication->IsGameRunning())return -1;
    auto* manager=gCSession::GetInstance().GetGUIManager();
    if(!manager)return -2;
    if(manager->IsMenuOpen())return -3;
    return manager->IsAnyPageOpen()?int(manager->GetActivePage()):-2;
}
// Native GUI gets a landscape canvas independently of square eye targets.
// No alternate scene resolution, projection, texture or extra render pass.
eCGfxShared::eSGfxViewport gMenuWorldViewport{};
bool gMenuViewportOverride=false;
void RestoreNativeMenuViewport(){
    if(!gMenuViewportOverride)return;
    gMenuViewportOverride=false;
    auto* wrapper=DeviceAccess::sGetAPIDevice();
    if(diagnostic::Readable(wrapper,sizeof(eCDX9Device)))wrapper->SetViewport(gMenuWorldViewport);
}
bool SynchronizeNativeUiResolution(bool keepMenuViewport=false)
{
    auto* wrapper=DeviceAccess::sGetAPIDevice();if(!diagnostic::Readable(wrapper,sizeof(eCDX9Device)))return false;
    auto* device=reinterpret_cast<eCDX9Device*>(wrapper)->m_pDevice;if(!device)return false;
    IDirect3DSurface9* surface=nullptr;if(FAILED(device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&surface)))return false;
    D3DSURFACE_DESC desc{};HRESULT hr=surface->GetDesc(&desc);surface->Release();if(FAILED(hr))return false;
    bool menu=MenuPanelActive();auto canvas=diagnostic::MenuCanvas(desc.Width,desc.Height,menu);
    eCGfxShared::eSGfxViewport previous{},full{},ui{};wrapper->GetViewport(previous);
    full.Width=desc.Width;full.Height=desc.Height;full.MinZ=0;full.MaxZ=1;
    ui=full;ui.Width=canvas.width;ui.Height=canvas.height;
    if(wrapper->SetViewport(ui)!=bEResult_Ok)return false;
    gMenuWorldViewport=full;gMenuViewportOverride=canvas.height!=desc.Height;
    static unsigned lastWidth=0,lastHeight=0;static int lastState=-100;
    int state=NativeUiState();bool changed=state!=lastState;
    // Do not rebuild menu layout every frame merely because the world
    // viewport was restored. Its cached GUI canvas is already correct.
    bool viewportRepair=!gMenuViewportOverride&&(previous.X||previous.Y||previous.Width!=canvas.width||previous.Height!=canvas.height);
    if(lastWidth!=canvas.width||lastHeight!=canvas.height||viewportRepair||changed){
        auto& gui=eCGUIModule::GetInstance();gui.EnableDrawScale(GETrue);gui.EnableMouseScale(GETrue);
        if(!gui.ReSize()){RestoreNativeMenuViewport();return false;}
        lastWidth=canvas.width;lastHeight=canvas.height;lastState=state;
        Event event=Record(Kind::Message);sprintf_s(event.text,"native_ui_resolution_sync world=%ux%u canvas=%ux%u menu=%u; independent_landscape_layout",desc.Width,desc.Height,canvas.width,canvas.height,menu);Push(event,true);
        auto virtualSize=gui.GetVirtualResolution();bCVector scale;gui.GetAdmin().GetRenderer().GetDrawScale(scale);
        Event layout=Record(Kind::Message);sprintf_s(layout.text,"native_ui_layout state=%d virtual=%dx%d scale=%.3f,%.3f",state,virtualSize.GetX(),virtualSize.GetY(),scale.m_fX,scale.m_fY);Push(layout,true);
    }
    // OnProcess uses the cached GUI layout, but world/physics/camera callbacks
    // still see their full square viewport. Keep the smaller viewport only
    // for OnPostRender's native menu draws and restore even on failures.
    if(!keepMenuViewport)RestoreNativeMenuViewport();
    return true;
}
bool BeginStereoUi()
{
    if(gStereo.shuttingDown || !gHeadControl.enabled || !gConfig.stereoEnabled || gStereo.fault)return false;
    if(!ConnectStereoRuntimeSafely())return false;
    bool sized=false;__try{sized=SynchronizeNativeUiResolution(true);}__except(EXCEPTION_EXECUTE_HANDLER){RestoreNativeMenuViewport();Message("native_ui_resolution_sync_fault",GetExceptionCode(),true);}
    if(!sized){StereoFailure("native_ui_resolution_sync_failed");return false;}
    gStereoRuntime.SetUiDiagnosticState(NativeUiState());
    gStereoRuntime.SetUiLayout(diagnostic::PanelLayout(gVrSettings,MenuPanelActive()));
    if(!gStereo.framePending)
    {
        // A rejected auxiliary camera / temporarily omitted world render is
        // not a request for a black-background HUD frame. Do not begin a new
        // XR frame here during gameplay: retain the last submitted scene for
        // runtime reprojection until the next complete native world pair.
        // Native desktop UI still executes once after this callback returns.
        // Real menus, loading/teardown and outside-game UI keep their existing
        // standalone UI path. Resolution synchronization above still runs.
        if(gApplication&&gApplication->IsGameRunning()&&!gWorldLifecycle.Blocked()&&!MenuPanelActive()){
            static unsigned reported=0;if(reported<3){++reported;Message("vr_world_gap_preserved; HUD_only_frame_not_started; previous_XR_scene_retained",0,true);}
            return false;
        }
        // Paused/full-screen menu frames can omit the perspective camera.
        // Continue the existing XR session with an independent UI-only frame.
        diagnostic::StereoFrame frame;auto status=gStereoRuntime.Begin(frame);
        if(status==diagnostic::StereoBegin::Failed){StereoFailure(gStereoRuntime.Error());return false;}
        if(status!=diagnostic::StereoBegin::Ready)return false;
        PublishHeadPose(frame.head);gStereo.framePending=true;
    }
    if(!gStereoRuntime.BeginUi()){StereoFailure(gStereoRuntime.Error());return false;}
    gStereo.uiRendering=true;return true;
}
void __fastcall StereoUiMouse(eCGUIModule* gui,void*,GEUInt message,GEUInt& flags,GEInt& coordinates)
{
    using Convert=void(__thiscall*)(eCGUIModule*,GEUInt,GEUInt&,GEInt&);
    bool clientMouse=(message>=WM_MOUSEMOVE&&message<=WM_MBUTTONDBLCLK)||(message>=WM_XBUTTONDOWN&&message<=WM_XBUTTONDBLCLK);
    if(clientMouse&&gHeadControl.enabled&&gConfig.stereoEnabled&&!gStereo.shuttingDown&&!gStereo.fault&&MenuPanelActive()){
        __try {
            auto* wrapper=DeviceAccess::sGetAPIDevice();
            if(diagnostic::Readable(wrapper,sizeof(eCDX9Device))){
                auto* device=reinterpret_cast<eCDX9Device*>(wrapper)->m_pDevice;D3DDEVICE_CREATION_PARAMETERS creation{};
                if(device&&SUCCEEDED(device->GetCreationParameters(&creation))){
                    RECT client{};POINT origin{};MONITORINFO monitor{sizeof(monitor)};
                    if(GetClientRect(creation.hFocusWindow,&client)&&ClientToScreen(creation.hFocusWindow,&origin)&&GetMonitorInfoW(MonitorFromWindow(creation.hFocusWindow,MONITOR_DEFAULTTONEAREST),&monitor)){
                        RECT screen{client.left+origin.x,client.top+origin.y,client.right+origin.x,client.bottom+origin.y},visible{};
                        if(IntersectRect(&visible,&screen,&monitor.rcMonitor)){
                            eCGfxShared::eSGfxViewport viewport{};wrapper->GetViewport(viewport);
                            auto canvas=diagnostic::MenuCanvas(viewport.Width,viewport.Height,true);
                            int x=diagnostic::UiMouseAxis(short(LOWORD(coordinates)),visible.left-origin.x,visible.right-origin.x,int(canvas.width));
                            int y=diagnostic::UiMouseAxis(short(HIWORD(coordinates)),visible.top-origin.y,visible.bottom-origin.y,int(canvas.height));
                            coordinates=GEInt(MAKELONG(x,y));
                            static bool reported=false;if(!reported){reported=true;Event event=Record(Kind::Message);sprintf_s(event.text,"ui_mouse_mapping visible=%ldx%ld target=%ux%u",visible.right-visible.left,visible.bottom-visible.top,viewport.Width,viewport.Height);Push(event,true);}
                        }
                    }
                }
            }
        } __except(EXCEPTION_EXECUTE_HANDLER){Message("ui_mouse_mapping_fault",GetExceptionCode(),true);}
    }
    reinterpret_cast<Convert>(gUiMouseHook.original)(gui,message,flags,coordinates);
}
void RequestCameraReset(char const* source)
{
    gLocatorProjection.Reset();
    RestoreCurrentBodyFacing();gHeadBodyHeading.Reset();
    ReleaseNativeSneak();
    gMelee.Reset();gBow.Cancel();gMagic.Cancel();gTrackedArms.ResetCombatSample();
    // Re-anchor translation AND orientation from the next valid headset pose.
    // Keep actor-relative height: dialogue transition cameras must never
    // recalibrate it. Settings offsets are also retained.
    gHeadControl.recenterPending=true;gHeadControl.centered=false;gHeadControl.movementHeadingValid=false;
    gStereo.cameraStabilizer.Reset();
    gControllerYaw.Reset();gTurnOutput.Reset();ReleaseNativeControllerActions();gWorldInteraction.selected=0;gControllerTranslator.ResetPointer();gControllerInput.Release();
    Event event=Record(Kind::Message);sprintf_s(event.text,"vr_camera_reset_requested source=%.96s",source);Push(event,true);gSample.store(diagnostic::probes::Enabled);
}
void UpdateQuestControllers()
{
    // Optional DLL presence is sampled once per second. The shortcut uses its
    // native configured key; no INI rewriting or resource scans per frame.
    static uint64_t locatorChecked=0;static bool locatorReady=false,locatorConfigured=false;
    auto locatorNow=GetTickCount64();
    if(!locatorConfigured&&locatorNow>=locatorChecked){locatorChecked=locatorNow+1000;
        if(auto module=GetModuleHandleW(L"Script_eCDELocator.dll")){
            locatorReady=gLocatorNativeInput.Install(module,reinterpret_cast<void*>(&LocatorKeyPressed));gMapperLocatorReady=locatorReady;
            if(locatorReady){bool projection=gLocatorNativeInput.InstallProjection(reinterpret_cast<void*>(&LocatorWorldToScreen));Message(projection?"vr_locator_projection_adapter_ready; headset_center_to_HUD_quad; no_fixed_pixel_offset":"vr_locator_projection_adapter_refused; original_matrix_preserved",0,true);}
            locatorConfigured=true;Message(locatorReady?"vr_optional_locator_ready; native_buffered_input_adapter; left_trigger_plus_X":"vr_optional_locator_ignored; install_ItemLocator_in_Union_or_native_adapter_refused",0,true);
        }
    }
    static bool initialized=false;
    if(!initialized){initialized=true;
        wchar_t documents[MAX_PATH];if(SUCCEEDED(SHGetFolderPathW(nullptr,CSIDL_PERSONAL,nullptr,SHGFP_TYPE_CURRENT,documents))){wchar_t path[MAX_PATH];swprintf_s(path,L"%s\\Gothic3\\UserOptions.ini",documents);if(GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES)swprintf_s(path,L"%s\\Gothic3\\__UserOptions.ini",documents);gControllerInput.Initialize(path);}
    }
    DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    bool allowed=gVrSettings.controllers&&foreground==GetCurrentProcessId()&&gHeadControl.enabled&&gConfig.stereoEnabled&&!gStereo.shuttingDown&&!gStereo.fault;
    auto* returnPlayer=gApplication&&gApplication->IsGameRunning()?gCSession::GetInstance().GetPlayer():nullptr;
    bool animal=returnPlayer&&diagnostic::NativeAnimalForm(returnPlayer);
    bool directionalActivity=!animal&&diagnostic::NativeDirectionalActivity(returnPlayer)&&!gFinisher.Busy(returnPlayer);
    auto frame=MappedControllers();bool menu=MenuPanelActive();bool lockpick=diagnostic::NativeLockpickSteps(returnPlayer);auto panel=diagnostic::PanelLayout(gVrSettings,menu);
    auto* wrapper=DeviceAccess::sGetAPIDevice();eCGfxShared::eSGfxViewport viewport{};if(diagnostic::Readable(wrapper,sizeof(eCDX9Device)))wrapper->GetViewport(viewport);
    auto canvas=diagnostic::MenuCanvas(viewport.Width,viewport.Height,menu);
    float aspect=canvas.width?float(canvas.height)/float(canvas.width):9.f/16.f;
    bool trade=menu&&NativeUiState()==int(gEGamePage_Trade);
    bool locatorGameplay=locatorReady&&!animal&&!diagnostic::NativeActivity(returnPlayer)&&!diagnostic::NativeRecovery(returnPlayer)&&gApplication&&gApplication->IsGameRunning()&&!gCSession::GetInstance().IsPaused();
    if(!locatorGameplay||!allowed||menu||!frame.focused||GetTickCount64()<frame.timeMs||GetTickCount64()-frame.timeMs>100)gLocatorNativeInput.pulse.Cancel();
    auto commands=gControllerTranslator.Translate(frame,GetTickCount64(),allowed,menu,panel.widthMeters,panel.widthMeters*aspect,panel.distanceMeters,panel.verticalMeters,gApplication&&gApplication->IsGameRunning(),lockpick,directionalActivity,trade,locatorGameplay);
    auto now=GetTickCount64();
    bool chordAllowed=allowed&&!menu&&!commands.radial.visible&&diagnostic::arms::FreshGrip(frame,0,now)&&frame.focused&&gBow.ready;
    if(gAnimalReturn.Step(chordAllowed,animal,reinterpret_cast<uintptr_t>(returnPlayer),gHeadControl.centerSession,frame.timeMs,frame.bindingsMapped?diagnostic::mapping::Active(frame,diagnostic::mapping::ReturnHuman):frame.hands[0].primary,frame.bindingsMapped?diagnostic::mapping::Active(frame,diagnostic::mapping::ReturnHuman):frame.hands[0].secondary)){
        Entity form(returnPlayer);form.EndTransform();auto* human=gCSession::GetInstance().GetPlayer();
        if(diagnostic::Readable(human,sizeof(eCDynamicEntity))&&diagnostic::ReturnedToDifferentHuman(reinterpret_cast<uintptr_t>(returnPlayer),reinterpret_cast<uintptr_t>(human),diagnostic::NativeAnimalForm(human))){
            // Match Script_Game's own transform-expiry cleanup. EndTransform
            // restores the human but intentionally leaves disposal to its caller.
            form.NPC.Species=gESpecies_EMPTY_B;form.Routine.AIMode=gEAIMode_Dead;
            auto discarded=returnPlayer->GetWorldMatrix();discarded.ModifyTranslation(bCVector(-220000.f,100000.f,100000.f));form.MoveTo(discarded);form.Decay();
            Message("vr_animal_return_requested; native_EndTransform_and_owned_form_decay",0,true);
        }else Message("vr_animal_return_cleanup_refused; active_form_preserved",0,true);
    }
    if((animal&&!menu&&frame.hands[0].primary&&frame.hands[0].secondary)||gAnimalReturn.Captured()){commands.held[diagnostic::Weapon]=false;commands.held[diagnostic::Inventory]=false;commands.held[diagnostic::Back]=false;commands.held[diagnostic::Sneak]=false;}
    if(commands.recenter)RequestCameraReset("controller_L3_R3_500ms");
    if(commands.pointer&&wrapper){auto* device=reinterpret_cast<eCDX9Device*>(wrapper)->m_pDevice;D3DDEVICE_CREATION_PARAMETERS creation{};
        if(device&&SUCCEEDED(device->GetCreationParameters(&creation))){RECT client{};POINT origin{};MONITORINFO monitor{sizeof(monitor)};
            if(GetClientRect(creation.hFocusWindow,&client)&&ClientToScreen(creation.hFocusWindow,&origin)&&GetMonitorInfoW(MonitorFromWindow(creation.hFocusWindow,MONITOR_DEFAULTTONEAREST),&monitor)){
                RECT screen{client.left+origin.x,client.top+origin.y,client.right+origin.x,client.bottom+origin.y},visible{};
                if(IntersectRect(&visible,&screen,&monitor.rcMonitor)){int x=visible.left+int(commands.u*(visible.right-visible.left-1)),y=visible.top+int(commands.v*(visible.bottom-visible.top-1));SetCursorPos(x,y);}
            }
        }
    }
    // Snapshot on the application thread for the wheel and independent hand HUD.
    // World transitions clear native ownership before another inventory read.
    if(gWorldLifecycle.Blocked())gNativeRadialItems.Reset();
    __try {
        auto const& items=gNativeRadialItems.Update(gWorldLifecycle.Blocked()?nullptr:returnPlayer,(commands.radial.visible&&commands.radial.page==0)||gVrSettings.handHudEnabled,now);
        gHandHud.Update(gWorldLifecycle.Blocked()?nullptr:returnPlayer,gVrSettings.handHudEnabled,items);
        gStereoRuntime.SetRadial(commands.radial,gVrSettings.numbers[51],&items);
    }__except(EXCEPTION_EXECUTE_HANDLER){
        gHandHud.Reset();gNativeRadialItems.Reset();gStereoRuntime.SetRadial(commands.radial,gVrSettings.numbers[51]);
        Message("vr_radial_items_unavailable; numbered_slots_preserved",GetExceptionCode(),true);
    }
    gRadialOpen=commands.radial.visible;
    // Physical melee consumes neither trigger as a native attack. Menu clicks,
    // unequipped interaction, ranged weapons and the radial keep their bindings.
    auto* combatPlayer=gApplication&&gApplication->IsGameRunning()?gCSession::GetInstance().GetPlayer():nullptr;
    bool activity=diagnostic::NativeActivity(combatPlayer);
    bool orbitContext=allowed&&gVrSettings.thirdPersonOrbit&&!gStereo.dialog&&!gStereo.chest&&!gStereo.lectern&&!gStereo.firstPerson&&!animal&&gApplication&&gApplication->IsGameRunning()&&diagnostic::Readable(combatPlayer,sizeof(eCDynamicEntity))&&
        (CameraThirdPersonActivity(combatPlayer)||diagnostic::NativeRecovery(combatPlayer));
    unsigned orbitMode=orbitContext?(diagnostic::NativeRecovery(combatPlayer)?2:3):0;
    static unsigned lastOrbitMode=0;if(orbitMode!=lastOrbitMode){lastOrbitMode=orbitMode;Event event=Record(Kind::Message);sprintf_s(event.text,"vr_third_person_orbit_context mode=%u; render_only; native_actor_facing_preserved",orbitMode);Push(event,true);}
    bool orbitInput=orbitContext&&frame.focused&&frame.hands[1].active&&now>=frame.timeMs&&now-frame.timeMs<=100&&!commands.captured&&!commands.radial.visible&&
        !menu&&!gCinemaScope.Active(GetCurrentThreadId());
    gThirdPersonOrbit.Step(reinterpret_cast<uintptr_t>(combatPlayer),orbitMode,orbitContext,orbitInput,frame.hands[1].stick[0],gVrSettings.numbers[7],now);
    if(activity){commands.moveX=commands.moveZ=commands.turnX=0;commands.sneakToggleButton=false;commands.held[diagnostic::Forward]=commands.held[diagnostic::Backward]=commands.held[diagnostic::Left]=commands.held[diagnostic::Right]=commands.held[diagnostic::Sprint]=false;}
    auto* finishWeapon=allowed&&!activity&&!menu&&!gRadialOpen&&gMelee.ready&&!gMelee.fault?gMelee.Weapon(combatPlayer):nullptr;
    bool finishAllowed=finishWeapon&&!gMagic.Selected(combatPlayer)&&!diagnostic::NativeRecovery(combatPlayer);
    eCEntity* finishTarget=nullptr;
    bool finishFresh=finishAllowed&&diagnostic::WorldAimReady(frame,now,true,false)&&diagnostic::arms::FreshGrip(frame,1,now);
    if(!finishFresh){gFinisherSearch.Reset();gFinisher.ForgetHint();}
    if(finishFresh){
        gFinisher.SearchContext(combatPlayer,finishWeapon,gHeadControl.centerSession);
        bool pressed=gFinisherSearch.Pressed(frame.hands[1].trigger>.5f&&frame.hands[1].squeeze>.5f);
        bool fullScan=gFinisherSearch.Due(reinterpret_cast<uintptr_t>(combatPlayer),reinterpret_cast<uintptr_t>(finishWeapon),gHeadControl.centerSession,now,pressed);
        if(fullScan||gFinisher.HasHint()){
            auto searchStarted=diagnostic::probes::Now();CameraSnapshot snapshot;SaveStereoCamera(gApplication->GetCurrentCamera(),snapshot);diagnostic::WorldAim aim;
            if(BuildControllerWorldAim(snapshot,aim,true)){
                if(!fullScan){finishTarget=gFinisher.Reuse(combatPlayer,aim,gConfig.unitsPerMeter);
                    // A removed/reordered ROI entry, lost line of sight or
                    // changed aim must not leave the finisher on a stale NPC.
                    if(!finishTarget)fullScan=gFinisherSearch.Due(reinterpret_cast<uintptr_t>(combatPlayer),reinterpret_cast<uintptr_t>(finishWeapon),gHeadControl.centerSession,now,true);}
                if(fullScan){finishTarget=gFinisher.Find(combatPlayer,aim,gConfig.unitsPerMeter);gFinisherSearch.Complete(finishTarget!=nullptr,now);}
            }else{gFinisher.ForgetHint();gFinisherSearch.Complete(false,now);}
            gStereoRuntime.NoteCpuUpdate(diagnostic::performance::UpdateStage::Finisher,searchStarted);
        }
    }
    if(finishFresh&&gSample.load()){Event event=Record(Kind::Message);sprintf_s(event.text,"vr_finisher_focus near_npc=%d down=%d ray=%d clear=%d selected=%08lx scans=%llu skipped_full=%llu reused=%llu",gFinisher.candidates,gFinisher.down,gFinisher.rays,gFinisher.clear,DWORD(reinterpret_cast<uintptr_t>(finishTarget)),gFinisherSearch.searches,gFinisherSearch.skipped,gFinisher.Reuses());Push(event);}
    auto* locomotion=diagnostic::Readable(combatPlayer,sizeof(eCDynamicEntity))?static_cast<gCCharacterMovement_PS*>(combatPlayer->GetPropertySet(eEPropertySetType_CharacterMovement)):nullptr;
    bool sneaking=allowed&&!animal&&!activity&&diagnostic::NativeSneaking(combatPlayer);
    static eCEntity* feedbackPlayer=nullptr;static bool previousSneak=false;
    if(feedbackPlayer==combatPlayer&&sneaking!=previousSneak){bool delivered=gStereoRuntime.Haptic(0,sneaking?.9f:.65f,160);Message(delivered?"vr_sneak_haptic_sent":"vr_sneak_haptic_refused; inspect_stereo_log",0,true);Message(sneaking?"vr_sneak_entered; native_state":"vr_sneak_left; native_state",0,true);}
    feedbackPlayer=combatPlayer;previousSneak=sneaking;
    auto* rangedWeapon=allowed&&!activity&&!menu&&gVrSettings.trackedArms?gBow.Weapon(combatPlayer):nullptr;
    bool guardAllowed=allowed&&!activity&&!menu&&!gRadialOpen&&!commands.captured&&!animal&&!gFinisher.Pending()&&!gFinisher.Busy(combatPlayer)&&
        diagnostic::Readable(combatPlayer,sizeof(eCDynamicEntity))&&!diagnostic::NativeRecovery(combatPlayer)&&!rangedWeapon;
    auto guardLoadout=guardAllowed?NativeParryLoadout(combatPlayer):diagnostic::ParryLoadout{};
    guardAllowed=guardAllowed&&guardLoadout.Compatible()&&!gMagic.Selected(combatPlayer);
    unsigned guardHand=guardLoadout.GuardHand();
    diagnostic::HeadPose guardHead{};bool guardHeadReady=false;
    if(guardAllowed&&TryAcquireSRWLockShared(&gHeadPoseLock)){guardHead=gHeadPose;ReleaseSRWLockShared(&gHeadPoseLock);
        LARGE_INTEGER ticks;QueryPerformanceCounter(&ticks);guardHeadReady=diagnostic::FreshOrientation(guardHead,ticks.QuadPart,gFrequency.QuadPart,100);}
    bool guardRaised=gGuardPose.Step(frame,guardHand,guardHead.position[1],now,guardAllowed&&guardHeadReady);
    if(guardAllowed)commands.held[diagnostic::Use2]=guardRaised&&guardLoadout.Pressed(commands.held[diagnostic::Use2],commands.shieldGrip);
    bool blocking=guardAllowed&&guardRaised&&diagnostic::NativeBlocking(combatPlayer);
    static bool previousBlock=false;if(blocking&&!previousBlock)gStereoRuntime.Haptic(guardHand,.35f,65);previousBlock=blocking;
    gStereoRuntime.SetWorldStatus(allowed&&!menu&&!gRadialOpen?(finishTarget?2:blocking?4:sneaking?1:0):0);
    bool healAllowed=allowed&&frame.focused&&frame.hands[0].active&&now>=frame.timeMs&&now-frame.timeMs<=100&&!gWorldLifecycle.Blocked()&&!menu&&!activity&&!animal&&!commands.captured&&!gRadialOpen&&!gFinisher.Busy(combatPlayer)&&
        diagnostic::Readable(combatPlayer,sizeof(eCDynamicEntity))&&!diagnostic::NativeRecovery(combatPlayer)&&!gCSession::GetInstance().IsPaused();
    if(gQuickHeal.Update(combatPlayer,healAllowed,commands.quickHeal,frame.timeMs))gStereoRuntime.Haptic(0,.4f,80);
    if(gFinisher.Update(combatPlayer,finishTarget,frame.hands[1].trigger>.65f&&frame.hands[1].squeeze>.65f,finishFresh,frame.timeMs,finishFresh?eCApplication::GetInstance().GetScaledFrameTimeInSeconds():0,gConfig.unitsPerMeter)){
        gMelee.Reset();AutoFirstPerson();gStereoRuntime.Haptic(1,.8f);Message("vr_native_finisher_requested; player_state_and_native_animation; quest_and_witness_logic_native",0,true);
    }
    if(gFinisher.Busy(combatPlayer)){commands.held[diagnostic::Use1]=commands.held[diagnostic::Use2]=false;commands.held[diagnostic::Forward]=commands.held[diagnostic::Backward]=commands.held[diagnostic::Left]=commands.held[diagnostic::Right]=commands.held[diagnostic::Sprint]=false;commands.turnX=0;}
    if(gFinisher.Pending())commands.held[diagnostic::Use2]=false; // finishing hold reserves right grip + trigger
    gMelee.SetFeedback([](unsigned hand,float strength){gStereoRuntime.Haptic(hand,strength);Message("vr_power_strike_charged; haptic_ready; one_stroke_per_trigger_press",hand,true);});
    if(allowed&&!menu&&gMelee.ready&&!gMelee.fault&&!gTrackedArms.fault&&gMelee.Weapon(combatPlayer)&&!gMagic.Selected(combatPlayer)&&!diagnostic::NativeRecovery(combatPlayer)&&!activity){
        commands.held[diagnostic::Use1]=false;if(!AllowNativeParry(combatPlayer))commands.held[diagnostic::Use2]=false;
    }
    if(rangedWeapon){
        commands.held[diagnostic::Use1]=false;commands.held[diagnostic::Use2]=false;
    }
    static diagnostic::RadialState previousRadial;
    if(commands.radial.visible!=previousRadial.visible||commands.radial.page!=previousRadial.page||commands.radial.selected!=previousRadial.selected){Event event=Record(Kind::Message);sprintf_s(event.text,"vr_radial_state visible=%d page=%d selected=%d clicks=%d,%d AB=%d,%d menu=%d",commands.radial.visible,commands.radial.page,commands.radial.selected,frame.hands[0].stickClick,frame.hands[1].stickClick,frame.hands[1].primary,frame.hands[1].secondary,menu);Push(event,true);previousRadial=commands.radial;}
    static uint64_t locatorObserve=0;
    if(locatorObserve&&now>=locatorObserve){locatorObserve=0;int key=-1,visible=-1;bool component=gLocatorNativeInput.Snapshot(key,visible);
        Event event=Record(Kind::Message);sprintf_s(event.text,"vr_locator_native_observation component=%d key=%d markers=%d polls=%llu consumed=%llu projection_reads=%llu",component,key,visible,gLocatorNativeInput.pulse.polls.load(),gLocatorNativeInput.pulse.consumed.load(),gLocatorProjectionUses);Push(event,true);}
    if(commands.pulse==diagnostic::LocatorToggle){
        int toggleKey=-1,markers=-1;
        if(gLocatorNativeInput.Snapshot(toggleKey,markers)&&toggleKey>=0&&toggleKey<=255)gLocatorNativeInput.pulse.Queue(now,unsigned(toggleKey));
        commands.pulse=-1;
        locatorObserve=now+300;
        int key=-1,visible=-1;bool component=gLocatorNativeInput.Snapshot(key,visible);
        Event event=Record(Kind::Message);sprintf_s(event.text,"vr_locator_native_request component=%d key=%d markers_before=%d; no_Windows_key",component,key,visible);Push(event,true);
    }
    if(commands.pulse>=0){auto binding=gControllerInput.Binding(commands.pulse);Event event=Record(Kind::Message);sprintf_s(event.text,"vr_radial_selected command=%d binding=%d,%d",commands.pulse,binding.type,binding.offset);Push(event,true);}
    auto send=[menu,directionalActivity](unsigned type,unsigned offset,bool down){bool sent=diagnostic::ControllerInput::Send(type,offset,down);if(type==0&&offset>=2&&offset<=11){Event event=Record(Kind::Message);sprintf_s(event.text,"vr_quick_key scancode=%u down=%d sent=%d",offset,down,sent);Push(event,true);}if((menu||directionalActivity)&&type==0&&(offset==0x1e||offset==0x20)){Event event=Record(Kind::Message);sprintf_s(event.text,"vr_menu_direction key=%c down=%d sent=%d; native_AD",offset==0x1e?'A':'D',down,sent);Push(event,true);}return sent;};
    bool walkingContext=allowed&&!menu&&!gRadialOpen&&!commands.captured&&!activity&&!animal&&!gFinisher.Busy(combatPlayer)&&!diagnostic::NativeRecovery(combatPlayer)&&
        frame.focused&&frame.hands[0].active&&now>=frame.timeMs&&now-frame.timeMs<=100&&
        gApplication&&gApplication->IsGameRunning()&&!gCSession::GetInstance().IsPaused()&&gStereo.firstPerson;
    static uintptr_t skillPlayer=0;static uint64_t nextSkill=0;static bool sneakLearned=false;
    if(walkingContext&&(skillPlayer!=reinterpret_cast<uintptr_t>(combatPlayer)||now>=nextSkill)){Entity hero(combatPlayer);sneakLearned=hero.Inventory.IsSkillActive(Template("Perk_Sneak_1"))||hero.Inventory.IsSkillActive(Template("Perk_Sneak_2"));skillPlayer=reinterpret_cast<uintptr_t>(combatPlayer);nextSkill=now+1000;}
    diagnostic::HeadPose crouchHead;
    if(TryAcquireSRWLockShared(&gHeadPoseLock)){crouchHead=gHeadPose;ReleaseSRWLockShared(&gHeadPoseLock);}
    LARGE_INTEGER crouchTicks;QueryPerformanceCounter(&crouchTicks);
    bool crouchTracking=diagnostic::FreshOrientation(crouchHead,crouchTicks.QuadPart,gFrequency.QuadPart,100)&&crouchHead.runtimeConnected&&crouchHead.deviceConnected&&(crouchHead.trackingResult&8)&&crouchHead.session==gHeadControl.centerSession;
    bool wasPhysicalWalk=gPhysicalCrouchWalk.Walking();
    bool physicalCrouchContext=walkingContext&&gVrSettings.physicalCrouching&&gVrSettings.trackedArms&&gVrSettings.stableBody&&gVrSettings.stableHorizon&&gHeadControl.centered&&!gHeadControl.recenterPending;
    bool physicalWalk=crouchTracking?gPhysicalCrouchWalk.Update(reinterpret_cast<uintptr_t>(combatPlayer),crouchHead.session,now,physicalCrouchContext,gStereo.centerPosition[1],crouchHead.position[1]):
        gPhysicalCrouchWalk.Hold(reinterpret_cast<uintptr_t>(combatPlayer),gHeadControl.centerSession,now,physicalCrouchContext,gStereo.centerPosition[1]);
    if(wasPhysicalWalk!=physicalWalk){Event event=Record(Kind::Message);sprintf_s(event.text,"vr_physical_crouch_walk active=%d drop_cm=%.1f; native_Walk; no_sneak_action",physicalWalk,100.f*(gStereo.centerPosition[1]-crouchHead.position[1]));Push(event,true);}
    if(physicalWalk){commands.held[diagnostic::Sprint]=false;gControllerTranslator.CancelSprint();}
    bool moving=std::hypot(commands.moveX,commands.moveZ)>.001f;
    bool toggleFresh=allowed&&frame.focused&&frame.hands[0].active&&now>=frame.timeMs&&now-frame.timeMs<=100&&gApplication&&gApplication->IsGameRunning();
    bool requestedSneak=gSneakToggle.Step(reinterpret_cast<uintptr_t>(combatPlayer),toggleFresh,walkingContext,commands.sneakToggleButton,sneakLearned,frame.timeMs,frame.bindingsMapped);
    // A radial wheel captures movement/buttons, but it must not leave native
    // Sneak just to let its weapon command bypass the game's restriction.
    bool sneakContext=walkingContext||(commands.captured&&allowed&&!menu&&!activity&&!animal&&!gFinisher.Busy(combatPlayer)&&!diagnostic::NativeRecovery(combatPlayer)&&
        toggleFresh&&gApplication&&gApplication->IsGameRunning()&&!gCSession::GetInstance().IsPaused()&&gStereo.firstPerson);
    if(sneakContext&&!walkingContext)requestedSneak=gSneakToggle.Enabled();
    EnsureArmedSneak();
    if(gSample.load()&&diagnostic::Readable(combatPlayer,sizeof(eCDynamicEntity))){
        auto* routine=static_cast<gCScriptRoutine_PS*>(combatPlayer->GetPropertySet(eEPropertySetType_ScriptRoutine));
        if(diagnostic::Readable(routine,sizeof(*routine))){Event event=Record(Kind::Message);
            _snprintf_s(event.text,_TRUNCATE,"vr_input_context menu=%d activity=%d lock=%d action=%d ani=%d sneak=%d native=%d motion=%d sneak_action=14 state=%.32s task=%.32s",menu,directionalActivity,lockpick,routine->GetAction().GetValue(),routine->GetAniState().GetValue(),requestedSneak,sneaking,diagnostic::Readable(locomotion,sizeof(*locomotion))&&locomotion->IsSneaking(),routine->GetCurrentState().GetText(),routine->GetCurrentTask().GetText());Push(event,true);}
    }
    gNativeSneakAction.Update(reinterpret_cast<uintptr_t>(combatPlayer),sneakContext&&!gNativeSneakFault,requestedSneak,now,SendNativeSneakAction);
    ApplyArmedSneak(requestedSneak&&sneakContext);
    commands.held[diagnostic::Sneak]=false; // Logical native action; never a physical Ctrl binding.
    bool sprintSelected=commands.held[diagnostic::Sprint];
    bool standing=true,braking=true;
    if(walkingContext&&diagnostic::Readable(combatPlayer,sizeof(eCDynamicEntity))){Entity hero(combatPlayer);standing=hero.IsStanding();braking=hero.IsBraking();}
    commands.held[diagnostic::Sprint]=walkingContext&&diagnostic::NativeSprintPress(commands.held[diagnostic::Sprint],moving,requestedSneak,standing,braking);
    // Set only the native wished mode. Native input still authors direction,
    // goals, translation, terrain handling and Sprint permission/stamina.
    gControllerModes.Update(walkingContext,reinterpret_cast<uintptr_t>(combatPlayer),frame.timeMs,moving,requestedSneak,commands.held[diagnostic::Sprint],gProcess.load(std::memory_order_relaxed),physicalWalk);
    if(gSample.load()&&walkingContext){Event event=Record(Kind::Message);
        sprintf_s(event.text,"vr_run_sprint_input selected=%d native_press=%d standing=%d braking=%d moving=%d sneak=%d; only_Run_or_native_Sprint; no_walk_press_from_idle",sprintSelected,commands.held[diagnostic::Sprint],standing,braking,moving,requestedSneak);Push(event,true);}
    if(walkingContext&&commands.sneakToggleButton&&!sneakLearned)gStereoRuntime.SetWorldStatus(finishTarget?2:3);
    // Logical actions still enter the original character/script handlers and
    // native ProcessMovements. Only their transport changes: no keyboard-held
    // state or delayed SendInput release can select another direction.
    bool nativeMotion=gActionHook.installed&&gMovementHook.installed;
    bool nativeMotionAllowed=allowed&&frame.focused&&frame.hands[0].active&&now>=frame.timeMs&&now-frame.timeMs<=100&&
        !menu&&!directionalActivity&&!activity&&!commands.captured&&!gRadialOpen&&!gFinisher.Busy(combatPlayer)&&!diagnostic::NativeRecovery(combatPlayer)&&
        !gWorldLifecycle.Blocked()&&gApplication&&gApplication->IsGameRunning()&&!gCSession::GetInstance().IsPaused();
    gNativeMovement.Sample(reinterpret_cast<uintptr_t>(combatPlayer),frame.timeMs,nativeMotion&&nativeMotionAllowed&&!gNativeMovementFault,
        commands.held[diagnostic::Forward],commands.held[diagnostic::Backward],commands.held[diagnostic::Left],commands.held[diagnostic::Right],commands.held[diagnostic::Sprint],gProcess.load(std::memory_order_relaxed));
    if(gMovementTrace.Armed()){gMovementTraceInput.frame=gProcess.load(std::memory_order_relaxed);gMovementTraceInput.x=commands.moveX;gMovementTraceInput.z=commands.moveZ;
        gMovementTraceInput.sprintSelected=sprintSelected;gMovementTraceInput.sprintPressed=commands.held[diagnostic::Sprint];}
    if(nativeMotion)diagnostic::ControllerInput::NativeMovementOnly(commands);
    if(!nativeMotionAllowed)ReleaseNativeControllerActions();
    if(gSample.load()&&nativeMotionAllowed){Event event=Record(Kind::Message);sprintf_s(event.text,"vr_native_movement_batch fresh=%d axes=%.2f,%.2f sprint=%d; logical_actions; no_Windows_movement_keys",gNativeMovement.Fresh(reinterpret_cast<uintptr_t>(combatPlayer),now,gProcess.load(std::memory_order_relaxed)),commands.moveX,commands.moveZ,sprintSelected);Push(event,true);}
    gControllerInput.Update(commands,now,allowed&&frame.focused&&now>=frame.timeMs&&now-frame.timeMs<=100,send);
    static bool previousUse=false;bool use=commands.held[diagnostic::Use1]&&nativeMotionAllowed;
    if(use&&!previousUse){char name[128]{};auto* target=reinterpret_cast<eCEntity*>(gWorldInteraction.selected);int type=0;
        if(diagnostic::Readable(target,sizeof(eCEntity))){diagnostic::visual::ReadName(target,name);type=int(gCEntity::GetUseType(target));}
        Event event=Record(Kind::Message);sprintf_s(event.text,"vr_interaction_press target=%08lx use_type=%d name=%.90s; native_Use1",DWORD(gWorldInteraction.selected),type,name);Push(event,true);}
    previousUse=use;
    bool nativeTurn=gMouseFilterXHook.original&&gWishedRotationHook.original&&gVrSettings.instantControllerTurn&&!animal;
    LARGE_INTEGER turnTicks;QueryPerformanceCounter(&turnTicks);
    gControllerYaw.Sample(nativeTurn&&!orbitContext&&allowed&&frame.focused&&frame.hands[1].active&&!menu&&!gRadialOpen&&!commands.captured&&!activity&&now>=frame.timeMs&&now-frame.timeMs<=100,
        commands.turnX,gVrSettings.numbers[7],now,gProcess.load(std::memory_order_relaxed),double(turnTicks.QuadPart)/double(gFrequency.QuadPart));
    if(!nativeTurn&&!orbitContext)gControllerInput.Turn(commands,gVrSettings.numbers[7],now);
    if(gSample.load()){Event event=Record(Kind::Message);sprintf_s(event.text,"quest_controllers focused=%d left=%d right=%d sticks=%.2f,%.2f/%.2f,%.2f triggers=%.2f,%.2f buttons=%d%d%d%d pointer=%d uv=%.2f,%.2f",frame.focused,frame.hands[0].active,frame.hands[1].active,frame.hands[0].stick[0],frame.hands[0].stick[1],frame.hands[1].stick[0],frame.hands[1].stick[1],frame.hands[0].trigger,frame.hands[1].trigger,frame.hands[0].primary,frame.hands[0].secondary,frame.hands[1].primary,frame.hands[1].secondary,commands.pointer,commands.u,commands.v);Push(event,true);}
}
bool PhysicalMeleeAllowed(unsigned hand=1)
{
    if(gCinemaScope.Active(GetCurrentThreadId()))return false;
    auto frame=MappedControllers();DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    return gHeadControl.enabled&&gHeadControl.centered&&!gHeadControl.recenterPending&&gConfig.stereoEnabled&&gVrSettings.controllers&&gVrSettings.trackedArms&&
        !gTrackedArms.fault&&!gStereo.shuttingDown&&!gStereo.fault&&gStereo.firstPerson&&!gRadialOpen&&foreground==GetCurrentProcessId()&&
        diagnostic::arms::FreshGrip(frame,hand,GetTickCount64())&&!MenuPanelActive()&&!gCSession::GetInstance().IsPaused();
}
bool BlockPhysicalSwordClick(gCCharacterControl_PS* self,uint16_t key)
{
    if(key!=gESessionKey_Use1&&key!=gESessionKey_Use2)return false;
    __try {
        if(((!gMelee.ready||gMelee.fault)&&(!gBow.ready||gBow.fault))||gTrackedArms.fault||!gHeadControl.enabled||!gConfig.stereoEnabled||!gVrSettings.trackedArms||MenuPanelActive())return false;
        auto* player=gCSession::GetInstance().GetPlayer();if(gMagic.Selected(player)||diagnostic::NativeActivity(player))return false;
        if(key==gESessionKey_Use2&&AllowNativeParry(player)&&!diagnostic::NativeRecovery(player))return false;
        return diagnostic::Readable(player,sizeof(eCDynamicEntity))&&player->GetPropertySet(eEPropertySetType_CharacterControl)==self&&
            (gBow.Weapon(player)!=nullptr||diagnostic::melee::BlockAttack(true,false,gMelee.Weapon(player)!=nullptr,key,diagnostic::NativeRecovery(player)));
    } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void __fastcall PhysicalSwordDamage(gCEntity* self,void*,eCEntity* inflictor,eCEntity* other,GEInt amount,GEInt type,eCContactIterator& contact)
{
    using Callback=void(__thiscall*)(gCEntity*,eCEntity*,eCEntity*,GEInt,GEInt,eCContactIterator&);
    if(gMelee.Pending(self,inflictor)){
        __try {amount=gMelee.Scale(self,inflictor,amount);}
        __except(EXCEPTION_EXECUTE_HANDLER){gMelee.fault=true;gMelee.ClearPending();Message("vr_melee_damage_scaling_fault; contact_refused",GetExceptionCode(),true);return;}
    }
    reinterpret_cast<Callback>(gMelee.damageHook.original)(self,inflictor,other,amount,type,contact);
}
GEInt GE_STDCALL PhysicalSwordHealth(gCScriptProcessingUnit* unit,GELPVoid self,GELPVoid other,GEInt delta)
{
    using Callback=GEInt(GE_STDCALL*)(gCScriptProcessingUnit*,GELPVoid,GELPVoid,GEInt);
    if(delta<0&&gMelee.PendingHealth()){
        __try {
            eCEntity* target=self?static_cast<eCEntity*>(*static_cast<Entity*>(self)):unit?unit->GetSelfEntity():nullptr;
            delta=gMelee.ScaleHealth(target,delta);
        } __except(EXCEPTION_EXECUTE_HANDLER){gMelee.fault=true;gMelee.ClearPending();Message("vr_melee_health_scaling_fault; contact_refused",GetExceptionCode(),true);return 0;}
    }
    return reinterpret_cast<Callback>(gMelee.hitPointsHook.original)(unit,self,other,delta);
}
void UpdatePhysicalMelee()
{
    gMelee.SetStrokeSettings(gVrSettings.numbers+52);gMelee.SetVelocitySettings(gVrSettings.numbers+64);gMelee.SetStabSettings(gVrSettings.numbers+67);gMelee.SetStabSpeedSettings(gVrSettings.numbers+70);
    // Both hands use the same hook/scaling context, but independent strokes.
    // Damage runs sequentially on the application thread after rendering.
    auto now=GetTickCount64();
    for(unsigned hand=0;hand<2;++hand){bool allowed=PhysicalMeleeAllowed(hand);eCEntity* player=nullptr; eCEntity* weapon=nullptr;
        if(allowed){player=gCSession::GetInstance().GetPlayer();weapon=gMelee.Weapon(player,hand);allowed=weapon&&!gFinisher.Pending()&&!gFinisher.Busy(player)&&!gMagic.Selected(player)&&!diagnostic::NativeRecovery(player)&&!diagnostic::NativeBlocking(player);
            if(allowed){auto loadout=NativeParryLoadout(player);if(loadout.Compatible()){
                auto controllers=MappedControllers();
                bool right=diagnostic::arms::FreshGrip(controllers,1,now)&&controllers.hands[1].squeeze>.65f;
                bool left=loadout.LeftShield()&&diagnostic::arms::FreshGrip(controllers,0,now)&&controllers.hands[0].squeeze>.65f;
                if(loadout.Pressed(right,left))allowed=false;
            }}
        }
        gMelee.Update(gTrackedArms.CombatSample(hand),player,weapon,allowed,now,hand);
    }
}
void __fastcall PhysicalBowProjectile(PSProjectile* self,void*)
{
    using Shoot=void(__thiscall*)(PSProjectile*);
    bool ours=false,prepared=false;
    __try{ours=gBow.OwnProjectile(self);}__except(EXCEPTION_EXECUTE_HANDLER){gBow.fault=true;Message("vr_bow_projectile_owner_fault",GetExceptionCode(),true);}
    if(ours){auto prepareStarted=diagnostic::probes::Now();__try{prepared=gBow.Prepare(self);}__except(EXCEPTION_EXECUTE_HANDLER){gBow.fault=true;Message("vr_bow_projectile_prepare_fault",GetExceptionCode(),true);}
        gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::ProjectilePrepare,prepareStarted,1);
        if(!prepared){gBow.fault=true;Message("vr_bow_projectile_ownership_or_pose_refused",0,true);return;}}
    if(!ours&&gMagic.aim.sample.valid){__try {gMagic.PrepareProjectile(self,GetTickCount64());}
        __except(EXCEPTION_EXECUTE_HANDLER){gMagic.fault=true;gMagic.Cancel();Message("vr_magic_projectile_aim_fault; native_magic_preserved",GetExceptionCode(),true);}}
    auto shootStarted=ours?diagnostic::probes::Now():0;
    __try {reinterpret_cast<Shoot>(gBow.projectileHook.original)(self);}
    __finally {if(shootStarted)gStereoRuntime.NoteNativePhase(diagnostic::performance::NativePhase::ProjectileShoot,shootStarted,1);}
}
void __cdecl RecordRangedTiming(diagnostic::performance::NativePhase phase,uint64_t started){gStereoRuntime.NoteNativePhase(phase,started,1);}
void UpdatePhysicalBow()
{
    gBow.SetTiming(RecordRangedTiming);
    gBow.BeginContext();
    if(gCinemaScope.Active(GetCurrentThreadId()))return;
    if(gWorldLifecycle.Blocked()||gStereo.shuttingDown||!gApplication||!gApplication->IsGameRunning()){gBow.Abandon();return;}
    auto sample=gTrackedArms.BowSample();
    bool allowed=PhysicalMeleeAllowed()&&(sample.crossbow||diagnostic::arms::FreshGrip(MappedControllers(),0,GetTickCount64()));
    bool game=gApplication&&gApplication->IsGameRunning()&&!gStereo.shuttingDown;
    auto* player=game?gCSession::GetInstance().GetPlayer():nullptr;auto* weapon=game?gBow.Weapon(player):nullptr;
    allowed=gBow.Recover(player,weapon,gHeadControl.centerSession,GetTickCount64())&&allowed;
    // Session/time APIs are valid only while the native game is running;
    // never query destroyed session objects on return to desktop/shutdown.
    bool paused=!gApplication||!gApplication->IsGameRunning()||gStereo.shuttingDown||gCSession::GetInstance().IsPaused();
    double scaled=paused?0:eCApplication::GetInstance().GetScaledFrameTimeInSeconds();
    gBow.Update(sample,player,weapon,allowed,GetTickCount64(),gVrSettings.numbers+23,scaled,paused);
}
struct RangedFaultSite {uintptr_t instruction=0,address=0;DWORD code=0,access=0;char const* operation="unknown";};
RangedFaultSite gRangedFaultSite;
int CaptureRangedFault(EXCEPTION_POINTERS* info)
{
    gRangedFaultSite={};gRangedFaultSite.operation=gBow.Operation();
    if(info&&info->ExceptionRecord){auto const* record=info->ExceptionRecord;gRangedFaultSite.code=record->ExceptionCode;
        gRangedFaultSite.instruction=reinterpret_cast<uintptr_t>(record->ExceptionAddress);
        if(record->ExceptionCode==EXCEPTION_ACCESS_VIOLATION&&record->NumberParameters>=2){gRangedFaultSite.access=DWORD(record->ExceptionInformation[0]);gRangedFaultSite.address=record->ExceptionInformation[1];}}
    return EXCEPTION_EXECUTE_HANDLER;
}
void ReportRangedFault()
{
    auto const& fault=gRangedFaultSite;HMODULE module=nullptr;char name[64]="unmapped_or_private",path[MAX_PATH]{};uintptr_t rva=0;
    if(fault.instruction&&GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCSTR>(fault.instruction),&module)){
        rva=fault.instruction-reinterpret_cast<uintptr_t>(module);
        if(GetModuleFileNameA(module,path,MAX_PATH)){auto* slash=strrchr(path,'\\');strncpy_s(name,slash?slash+1:path,_TRUNCATE);}}
    Event event=Record(Kind::Message);event.code=fault.code;
    sprintf_s(event.text,"vr_physical_ranged_fault phase=%.32s ip=%08lx module=%.40s+%08lx access=%lu address=%08lx retry_ms=500",fault.operation,DWORD(fault.instruction),name,DWORD(rva),fault.access,DWORD(fault.address));Push(event,true);
}
void FinishStereoUi(bool abnormal)
{
    gStereo.uiRendering=false;gStereo.framePending=false;
    if(abnormal){gStereoRuntime.Cancel();return;}
    if(!gStereoRuntime.EndUi() || !gStereoRuntime.Submit()){StereoFailure(gStereoRuntime.Error());return;}
    if(gSample.load())
    {
        Event event=Record(Kind::Message);sprintf_s(event.text,"geometric_stereo_with_ui submissions=%llu left=%llu right=%llu restores=%llu",gStereoRuntime.Submissions(),gStereo.eyeDraws[0],gStereo.eyeDraws[1],gStereo.restores);Push(event);
    }
}
// Verified CP 1.75 OnFogChanged fills vecFogAndDepthOffset.xy. Preserve
// its generation counters, color, depth offset and native lifecycle. No stored
// entity/material pointers and no cache rebuild or per-frame scene scans.
void __fastcall SetWorldFogIntensity(eCShaderMaterialAdmin* self,void*,eCGfxFog* fog){
    reinterpret_cast<void(__thiscall*)(eCShaderMaterialAdmin*,eCGfxFog*)>(gFogIntensityHook.original)(self,fog);
    if(gStereo.shuttingDown||!fog||gVrSettings.numbers[85]>=100)return;
    auto* data=reinterpret_cast<unsigned char*>(self);float x,y;
    memcpy(&x,data+0x188,4);memcpy(&y,data+0x18c,4);
    if(diagnostic::fog::Scale(x,y,gVrSettings.numbers[85])){
        memcpy(data+0x188,&x,4);memcpy(data+0x18c,&y,4);
    }
}
void InstallFogIntensity(HMODULE engine){
    auto* entry=GetProcAddress(engine,"?OnFogChanged@eCShaderMaterialAdmin@@QAEXPAVeCGfxFog@@@Z");
    auto reset=GetProcAddress(engine,"?sResetCachedStates@eCGfxFog@@KGXXZ");
    // First complete instruction is MOVSS xmm1,[absolute literal 1.0f]. Its
    // relocated address is copied into the trampoline, not a relative jump.
    unsigned char prefix[8]={0xf3,0x0f,0x10,0x0d};
    uint32_t literal=uint32_t(reinterpret_cast<uintptr_t>(engine)+0x42612c);memcpy(prefix+4,&literal,4);
    if(reinterpret_cast<unsigned char*>(entry)==reinterpret_cast<unsigned char*>(engine)+0x237270&&
       reset&&diagnostic::Executable(reinterpret_cast<void*>(reset))&&
       gFogIntensityHook.Install(reinterpret_cast<void*>(entry),reinterpret_cast<void*>(&SetWorldFogIntensity),prefix,8)){
        gResetNativeFog=reinterpret_cast<ResetNativeFog>(reset);gResetNativeFog();
        Message("vr_fog_intensity_ready; live_native_uniform; draw_distances_unchanged",0,true);
    }else Message("vr_fog_intensity_unavailable; native_fog_preserved",0,true);
}
bool InstallStereoHook()
{
    if(!gConfig.stereoEnabled)return true;
    auto* engine=GetModuleHandleW(L"Engine.dll");void* render=engine?reinterpret_cast<void*>(GetProcAddress(engine,"?Render@eCCameraBase@@QAEX_N0@Z")):nullptr;
    constexpr unsigned char expectedHash[32]={0x99,0x1c,0xf6,0x74,0xa9,0xcf,0x1b,0xc6,0x0c,0xdd,0xc2,0x04,0xbe,0x09,0x93,0xe8,0x25,0x13,0x36,0xf4,0xd3,0x47,0x15,0x43,0xa3,0x78,0x46,0xcc,0x68,0x40,0xbf,0x5a};
    wchar_t path[MAX_PATH];unsigned char digest[32]{};
    if(!engine||!GetModuleFileNameW(engine,path,MAX_PATH)||!diagnostic::FileSha256(path,digest)||memcmp(digest,expectedHash,32)||!render||!diagnostic::Readable(render,8)||memcmp(render,diagnostic::CameraRenderHook::Prefix,8))
    {Message("stereo_hook_refused; Engine_hash_or_Render_prologue_mismatch",0,true);gStereo.fault=true;return false;}
    // This build's native application imports Game::OnDestroyEngine. Validate
    // the SDK slot and named import, preserving any existing IAT detour.
    size_t destroySlot=0;auto appTable=*reinterpret_cast<void***>(gApplication);
    void* destroyTarget=nullptr;auto exe=GetModuleHandleW(nullptr);
    if(!diagnostic::VirtualSlot(&ApplicationAccess::OnDestroyEngine,destroySlot)||destroySlot!=165||
        !diagnostic::Readable(appTable+destroySlot,sizeof(void*))||
        !diagnostic::NamedImportTarget(exe,appTable[destroySlot],"Game.dll","?OnDestroyEngine@gCGameApp@@MAEXXZ",destroyTarget)||
        !gDestroyEngineHook.Prepare(appTable,destroySlot,reinterpret_cast<void*>(&StereoDestroyEngine))||!gDestroyEngineHook.Install())
    {Message("stereo_shutdown_hook_refused; stereo_not_enabled",0,true);gStereo.fault=true;return false;}
    size_t workspaceSlot=0;
    if(!diagnostic::VirtualSlot(&ApplicationAccess::OnDestroyWorkspace,workspaceSlot)||workspaceSlot!=166||
       !diagnostic::Readable(appTable+workspaceSlot,sizeof(void*))||appTable[workspaceSlot]!=reinterpret_cast<unsigned char*>(exe)+diagnostic::WorkspaceCallbackRva||
       !gDestroyWorkspaceHook.Prepare(appTable,workspaceSlot,reinterpret_cast<void*>(&StereoDestroyWorkspace))||!gDestroyWorkspaceHook.Install())
    {gDestroyEngineHook.Restore();Message("stereo_workspace_shutdown_hook_refused; stereo_not_enabled",0,true);gStereo.fault=true;return false;}
    auto& camera=gApplication->GetCurrentCamera();auto table=*reinterpret_cast<void***>(&camera);size_t slot=0;
    auto* projection=GetProcAddress(engine,"?UpdateProjection@eCCameraBase@@UAEX_N@Z");
    if(!diagnostic::VirtualSlot(&eCCameraBase::UpdateProjection,slot)||!diagnostic::Readable(table+slot,sizeof(void*))||diagnostic::ResolveJumpChain(table[slot])!=projection||
        !gProjectionHook.Prepare(table,slot,reinterpret_cast<void*>(&StereoProjectionUpdate))||!gProjectionHook.Install())
    {gDestroyEngineHook.Restore();Message("stereo_projection_hook_refused",0,true);gStereo.fault=true;return false;}
    if(!gCameraRenderHook.Install(render,reinterpret_cast<void*>(&StereoCameraRender)))
    {gProjectionHook.Restore();gDestroyEngineHook.Restore();Message("stereo_render_hook_failed",0,true);gStereo.fault=true;return false;}
    InstallNativeRenderProfiler(engine);
#ifdef G3VR_SCENE_COST_TRIAL
    InstallSceneCost(engine);
#endif
    InstallTemporalPhaseCapture(engine);
#ifdef G3VR_RENDERER_FEASIBILITY
    InstallRendererFeasibility(engine);
#endif
    InstallPupStereoCompatibility();
    InstallStereoOcclusion(engine);
    InstallFogIntensity(engine);
    Message(gNativeHudVisibility.Install(reinterpret_cast<void*>(&NativeHudWindowProc))?
        "vr_hud_visibility_ready; compass_quickbar_vitals; paint_only":"vr_hud_visibility_unavailable; native_hud_preserved",0,true);
    auto* mouse=GetProcAddress(engine,"?ToVirtualCoords@eCGUIModule@@QBEXIAAIAAJ@Z");
    if(!gUiMouseHook.Install(mouse,reinterpret_cast<void*>(&StereoUiMouse))){Message("stereo_ui_mouse_hook_refused",0,true);gStereo.fault=true;return false;}
    if(!InstallControllerFocus()){gWorldInteraction.fault=true;Message("controller_world_focus_hooks_refused; native_focus_preserved",0,true);}
    auto* armPalette=GetProcAddress(engine,"?UpdateRenderMatrices@eCVisualAnimationFactory@@QAE_NKABVbCMatrix@@@Z");
    if(!gArmPaletteHook.Install(armPalette,reinterpret_cast<void*>(&UpdateTrackedArmPalette))){gTrackedArms.fault=true;Message("vr_arms_palette_hook_refused; native_arms_preserved",0,true);}
    else Message("vr_arms_palette_hook_installed; player_only; per_eye_restore; physical_damage_pending",0,true);
    auto* particleProcess=GetProcAddress(engine,"?OnProcess@eCParticle_PS@@UAEXXZ");
    auto* particleAlpha=GetProcAddress(engine,"?OnRenderAlpha@eCParticle_PS@@UAEXF@Z");
    gFlameHooksReady=reinterpret_cast<void*>(particleProcess)==reinterpret_cast<unsigned char*>(engine)+diagnostic::visual::ParticleProcessRva&&
        reinterpret_cast<void*>(particleAlpha)==reinterpret_cast<unsigned char*>(engine)+diagnostic::visual::ParticleAlphaRva&&
        gFlameProcessHook.Install(particleProcess,reinterpret_cast<void*>(&ProcessOwnedFlame),diagnostic::visual::ParticleProcessPrefix,sizeof(diagnostic::visual::ParticleProcessPrefix))&&
        gFlameAlphaHook.Install(particleAlpha,reinterpret_cast<void*>(&RenderOwnedFlameAlpha),diagnostic::visual::ParticleAlphaPrefix,sizeof(diagnostic::visual::ParticleAlphaPrefix));
    auto* particleVertices=GetProcAddress(engine,"?RenderVertices@eCParticle_PS@@IAE_NMPAV1@@Z");
    auto* particleWorld=GetProcAddress(engine,"?SetWorldMatrix@eCGfxMixerAdmin@@QAE?AW4bEResult@@ABVbCMatrix@@@Z");
    unsigned char worldPrefix[]{0x83,0x79,0x0c,0x00,0x53,0x55};
    gTorchHooksReady=reinterpret_cast<unsigned char*>(particleVertices)==reinterpret_cast<unsigned char*>(engine)+0x154c80&&
        reinterpret_cast<unsigned char*>(particleWorld)==reinterpret_cast<unsigned char*>(engine)+0x88c20&&
        gParticleVerticesHook.Install(particleVertices,reinterpret_cast<void*>(&RenderAttachedParticleVertices),diagnostic::FocusHook::FindPrefix,6)&&
        gParticleWorldHook.Install(particleWorld,reinterpret_cast<void*>(&AttachedParticleWorldMatrix),worldPrefix,sizeof(worldPrefix));
    gMagicHandLayoutReady=gTorchHooksReady&&diagnostic::visual::VerifyHandEffectLayout(engine);
    auto* budgetVertices=GetProcAddress(engine,"?UpdateVertices@eCParticle_PS@@IAE_NXZ");
    auto* refraction=GetProcAddress(engine,"?IsRefracted@eCParticle_PS@@UBE_NXZ");
    gParticlePerformanceReady=gFlameHooksReady&&
        reinterpret_cast<unsigned char*>(budgetVertices)==reinterpret_cast<unsigned char*>(engine)+diagnostic::particles::VerticesRva&&
        reinterpret_cast<unsigned char*>(refraction)==reinterpret_cast<unsigned char*>(engine)+diagnostic::particles::RefractionRva&&
        gParticleBudgetHook.Install(budgetVertices,reinterpret_cast<void*>(&RenderBudgetParticleVertices),diagnostic::particles::VerticesPrefix,sizeof(diagnostic::particles::VerticesPrefix))&&
        gParticleRefractionHook.Install(refraction,reinterpret_cast<void*>(&ParticleRefraction),diagnostic::particles::RefractionPrefix,sizeof(diagnostic::particles::RefractionPrefix));
    Message(gParticlePerformanceReady?"vr_particle_performance_ready; scoped_vertex_budget; heat_haze_bypass; native_simulation_preserved":"vr_particle_performance_unavailable; native_effects_preserved",0,true);
    Message(gMagicHandLayoutReady?"vr_magic_bone_effects_ready; actual_hero_hand_attachments; per_eye_restore":"vr_magic_bone_effects_unavailable; native_effects_preserved",0,true);
    Message(gTorchHooksReady?"vr_torch_particles_ready; scoped_world_draw; native_particle_buffers_unchanged":"vr_torch_particles_unavailable; native_effects_preserved",0,true);
    Message(gFlameHooksReady?"vr_flame_tail_hooks_installed; bounded_target_cache; unrelated_emitters_no_sdk_queries":"vr_flame_tail_hooks_refused; native_effects_preserved",0,true);
#ifdef G3VR_ENTITY_COST_TRIAL
    InstallEntityCost(engine);
#endif
    Message("geometric_stereo_hooks_installed; Ctrl_Shift_F8_toggle; Ctrl_Shift_F11_recenter",0,true);return true;
}
void PollKeys()
{
    if(gStereo.shuttingDown)return;
    static bool previousMarker = false, previousTrace = false, previousToggle=false, previousRecenter=false;
    DWORD foregroundProcess = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
    bool chord = foregroundProcess == GetCurrentProcessId() && Pressed(VK_CONTROL) && Pressed(VK_SHIFT);
    bool marker = diagnostic::probes::Enabled&&chord && Pressed(VK_F6);
    bool trace = diagnostic::probes::Enabled&&chord && Pressed(VK_F7);
    bool toggle=chord && Pressed(VK_F8),recenter=chord && Pressed(VK_F11);
    if(toggle && !previousToggle && gConfig.headControlAllowed && gConfig.trackingEnabled && !gHeadControl.fault)
    {
        gHeadControl.enabled=!gHeadControl.enabled;gHeadControl.centered=false;
        if(gHeadControl.enabled&&gConfig.stereoEnabled)ReloadFirstPersonHeight(true);
        if(gConfig.stereoEnabled){gStereo.fault=false;gStereo.nextRetry=0;if(!gHeadControl.enabled)StopStereo();}
        gHeadControl.recenterPending=gHeadControl.enabled;
        ControlRecord(gHeadControl.enabled?"enabled; recenter_pending":"disabled");
        gSample.store(diagnostic::probes::Enabled);
    }
    if(recenter && !previousRecenter && gHeadControl.enabled && !gHeadControl.fault)
    {if(gConfig.stereoEnabled)ReloadFirstPersonHeight(true);RequestCameraReset("keyboard_Ctrl_Shift_F11");}
    if (marker && !previousMarker)
    {
        Event event = Record(Kind::Marker);
        strcpy_s(event.text, "manual_snapshot");
        Push(event);
        gSample.store(diagnostic::probes::Enabled);
    }
    if (trace && !previousTrace)
    {
        gTraceEnd.store(gProcess.load() + gConfig.traceFrames - 1);
        Message("manual_trace_started");
        gSample.store(diagnostic::probes::Enabled);
    }
    previousMarker = marker;
    previousTrace = trace;
    previousToggle=toggle;previousRecenter=recenter;
}

bool Tracing()
{
    if constexpr(!diagnostic::probes::Enabled)return false;
    uint64_t process = gProcess.load(std::memory_order_relaxed);
    return process != 0 && process <= gTraceEnd.load(std::memory_order_relaxed);
}

using OriginalCallback = void(__thiscall*)(eCApplication*);
void RestoreNativeBowCrosshair(gCFocus_PS* focus,bool visible)
{
    if(!focus)return;
    __try {focus->SetDrawCrossHair(visible);auto* manager=gCSession::GetInstance().GetGUIManager();if(manager)manager->ShowCrossHair(visible);}
    __except(EXCEPTION_EXECUTE_HANDLER){Message("vr_bow_native_crosshair_restore_fault",GetExceptionCode(),true);}
}
void Observe(eCApplication* app, int hook)
{
    bool ours = app == gApplication;
    if(ours&&gWorldLifecycle.Blocked()){reinterpret_cast<OriginalCallback>(gHooks[hook].original)(app);return;}
    bool applicationThread=ours && GetCurrentThreadId()==gApplicationThread.load();
    if(applicationThread && hook==1 && gStereo.framePending)
    {gStereoRuntime.Cancel();gStereo.framePending=false;gStereo.uiRendering=false;}
    // Also restore on the next update/pre-render if a previous frame skipped PostRender.
    if(applicationThread && (hook==0 || hook==1))RestoreHeadCamera(app);
    if (ours)
    {
        if constexpr(diagnostic::probes::Enabled)gCounts[hook].fetch_add(1, std::memory_order_relaxed);
        if (hook == 0)
        {
            gProcess.fetch_add(1, std::memory_order_relaxed);
            DWORD thread = GetCurrentThreadId();
            DWORD unassigned = 0;
            gApplicationThread.compare_exchange_strong(unassigned, thread);
            if (thread == gApplicationThread.load())
            {
                uint64_t now = GetTickCount64();
                bool due = now >= gNextSampleMs;
                gSample.store(diagnostic::probes::Enabled&&due);
                if (due) gNextSampleMs = now + gConfig.sampleMs;
                PollKeys();
                static uint64_t nextSettingsRead=0;
                if(now>=nextSettingsRead){ReloadFirstPersonHeight(false);nextSettingsRead=now+1000;}
            }
        }
        else if (hook == 1) gRender.fetch_add(1, std::memory_order_relaxed);
        if (Tracing()) Push(Record(Kind::Phase, hook));
    }
    // Do not catch game exceptions or change the callback's behavior.
    // The native game can create/layout its inventory and dialogue windows in
    // OnProcess, before OnPostRender. Give those calculations the full viewport
    // as well, rather than repairing only their later draw scale.
    if(applicationThread&&hook==0&&gHeadControl.enabled&&gConfig.stereoEnabled&&!gStereo.fault&&!gStereo.shuttingDown){
        __try{SynchronizeNativeUiResolution();}__except(EXCEPTION_EXECUTE_HANDLER){RestoreNativeMenuViewport();Message("native_ui_update_resolution_sync_fault",GetExceptionCode(),true);}
    }
    if(diagnostic::probes::Enabled&&applicationThread&&hook==0){
        __try{InstallNativeSessionProfiler();}__except(EXCEPTION_EXECUTE_HANDLER){Message("native_session_profiler_unavailable; preparation_fault; gameplay_unchanged",GetExceptionCode(),true);}
        gSceneUpdateContext=app->IsGameRunning()?(MenuPanelActive()?2:1):0;gStereoRuntime.BeginCpuUpdate(gSceneUpdateContext);}
    if(applicationThread&&hook==0&&!gCinemaScope.Active(GetCurrentThreadId())){auto inputStarted=diagnostic::probes::Now();__try{UpdateQuestControllers();}__except(EXCEPTION_EXECUTE_HANDLER){ReleaseNativeControllerActions();ReleaseNativeSneak();gControllerInput.Release();Message("quest_controller_input_fault",GetExceptionCode(),true);}
        gStereoRuntime.NoteCpuUpdate(diagnostic::performance::UpdateStage::Input,inputStarted);}
    if(applicationThread&&hook==0){__try{BeginInstantControllerTurn();}__except(EXCEPTION_EXECUTE_HANDLER){gInstantTurnCamera=nullptr;Message("vr_instant_turn_scope_refused",GetExceptionCode(),true);}}
    bool captureUi=applicationThread && hook==3 && !gCinemaScope.Active(GetCurrentThreadId()) && BeginStereoUi();uint64_t uiEpoch=gMovieEpoch;
    gCFocus_PS* bowFocus=nullptr;bool nativeCrosshair=false;
    if(captureUi&&(gBowReticle.Active()||(!gVrSettings.bowReticle&&(gTrackedArms.BowSample().valid||gMagic.aim.sample.valid)))&&!MenuPanelActive()&&!gRadialOpen){
        __try {auto* player=gCSession::GetInstance().GetPlayer();if(diagnostic::Readable(player,sizeof(eCDynamicEntity)))bowFocus=static_cast<gCFocus_PS*>(player->GetPropertySet(eEPropertySetType_Focus));
            if(diagnostic::Readable(bowFocus,sizeof(*bowFocus))){nativeCrosshair=bowFocus->GetDrawCrossHair();bowFocus->SetDrawCrossHair(GEFalse);auto* manager=gCSession::GetInstance().GetGUIManager();if(manager)manager->ShowCrossHair(GEFalse);}else bowFocus=nullptr;
        }__except(EXCEPTION_EXECUTE_HANDLER){bowFocus=nullptr;Message("vr_bow_native_crosshair_hide_fault",GetExceptionCode(),true);}
    }
    uint64_t nativeStarted=applicationThread&&hook==0?diagnostic::probes::Now():0;
    __try {reinterpret_cast<OriginalCallback>(gHooks[hook].original)(app);}
    __finally {if(applicationThread&&hook==0){gStereoRuntime.NoteCpuUpdate(diagnostic::performance::UpdateStage::Native,nativeStarted);}RestoreNativeBowCrosshair(bowFocus,nativeCrosshair);if(captureUi&&uiEpoch==gMovieEpoch)FinishStereoUi(AbnormalTermination()!=FALSE);if(applicationThread&&hook==3)RestoreNativeMenuViewport();}
    if (!ours) return;
    if(gWorldLifecycle.Blocked())return;
    applicationThread=GetCurrentThreadId()==gApplicationThread.load();
    if(applicationThread&&hook==0){__try{ApplyArmedSneak(gNativeSneakAction.Owns(reinterpret_cast<uintptr_t>(gCSession::GetInstance().GetPlayer()),GetTickCount64()));}
        __except(EXCEPTION_EXECUTE_HANDLER){gArmedSneak.Abandon();Message("vr_armed_sneak_post_update_refused",GetExceptionCode(),true);}}
    if(applicationThread&&hook==0){__try{AutoFirstPerson();}__except(EXCEPTION_EXECUTE_HANDLER){
        gFirstPersonPermissionFault=true;gConfig.keepFirstPerson=false;Message("auto_first_person_fault; native_camera_preserved",GetExceptionCode(),true);}}
    if(applicationThread&&hook==0){auto meleeStarted=diagnostic::probes::Now();__try{UpdatePhysicalMelee();}__except(EXCEPTION_EXECUTE_HANDLER){
        gMelee.ClearPending();gMelee.Reset();gMelee.fault=true;gTrackedArms.ResetCombatSample();Message("vr_physical_melee_fault; physical_contacts_disabled",GetExceptionCode(),true);}
        gStereoRuntime.NoteCpuUpdate(diagnostic::performance::UpdateStage::Melee,meleeStarted);}
    if(applicationThread&&hook==0){auto rangedStarted=diagnostic::probes::Now();__try{UpdatePhysicalBow();}__except(CaptureRangedFault(GetExceptionInformation())){
        gBow.Fail(GetTickCount64());ReportRangedFault();}
        gStereoRuntime.NoteCpuUpdate(diagnostic::performance::UpdateStage::Ranged,rangedStarted);gStereoRuntime.EndCpuUpdate();}
    if(applicationThread && hook==1)ApplyHeadCamera(app);
    if (Tracing()) Push(Record(Kind::Phase, hook, true));
    if (!gLogging.load(std::memory_order_relaxed))
    {if(applicationThread)RestoreHeadCamera(app);return;}
    if (hook == 1 && gConfig.trackingEnabled && (Tracing() || gSample.load(std::memory_order_relaxed)))
        CaptureHeadPose("OnPreRender.exit");
    if (GetCurrentThreadId() == gApplicationThread.load() && gSample.load(std::memory_order_relaxed))
    {
        CaptureCamera(app, hook);
        if (hook == 0)
        {
            Event summary = Record(Kind::Summary);
            for (unsigned i = 0; i < 4; ++i) summary.counts[i] = gCounts[i].load(std::memory_order_relaxed);
            summary.dropped = gDropped.load(std::memory_order_relaxed);
            Push(summary);
        }
        if (hook == 3) CaptureDevice();
    }
    if(applicationThread && hook==3)RestoreHeadCamera(app);
}

void __fastcall OnProcess(eCApplication* self, void*) { Observe(self, 0); }
void __fastcall OnPreRender(eCApplication* self, void*) { Observe(self, 1); }
void __fastcall OnRenderDraw(eCApplication* self, void*) { Observe(self, 2); }
void __fastcall OnPostRender(eCApplication* self, void*) { Observe(self, 3); }

bool InstallMovementHook()
{
    HMODULE game=GetModuleHandleW(L"Game.dll");
    wchar_t path[MAX_PATH]{};unsigned char hash[32]{};
    constexpr unsigned char expectedHash[32]={
        0x47,0x28,0xa4,0x7b,0x24,0x4a,0xdc,0xb0,0xea,0x2e,0xc3,0x50,0x1d,0xb0,0x49,0x94,
        0x90,0x9d,0xcf,0xcf,0xf3,0x38,0xc7,0xb8,0x07,0x5f,0x2b,0x68,0xdd,0x8d,0x6e,0x00};
    if(!game || !GetModuleFileNameW(game,path,MAX_PATH) ||
       !diagnostic::FileSha256(path,hash) || memcmp(hash,expectedHash,sizeof(hash)))
    {Message("movement_Game_DLL_profile_mismatch; camera tracking remains available");return false;}
#ifdef G3VR_NAVIGATION_BUDGET_RUNTIME
    // Exact full Game.dll hash above, then native entry and full local control
    // flow guard. Only the existing simple-navigation time budget is adapted.
    Message(gNavigationBudget.Install(game)?"vr_navigation_budget_ready; distant_simple_navigation_4ms_soft; native_16ms_catchup; active_ROI_loop_unchanged":"vr_navigation_budget_refused; native_navigation_preserved",0,true);
#endif
    // Copy the absolute import operand with this module's actual relocation.
    // Complete boundaries: push ecx; push ebx; push ebp; mov ebp,[IAT].
    unsigned char stopPrefix[9]{0x51,0x53,0x55,0x8b,0x2d};
    uintptr_t threadImport=reinterpret_cast<uintptr_t>(game)+diagnostic::SessionStopThreadImportRva;
    memcpy(stopPrefix+5,&threadImport,4);
    auto stop=GetProcAddress(game,"?Stop@gCSession@@QAEXXZ");
    if(reinterpret_cast<uintptr_t>(stop)!=reinterpret_cast<uintptr_t>(game)+diagnostic::SessionStopRva||
       !gSessionStopHook.Install(stop,reinterpret_cast<void*>(&StopNativeSession),stopPrefix,9))
    {Message("vr_world_stop_hook_refused; stereo_world_use_disabled",0,true);gStereo.fault=true;return false;}
    size_t slot=0, actionSlot=0;
    auto** table=reinterpret_cast<void**>(GetProcAddress(game,"??_7gCCharacterControl_PS@@6B@"));
    void* expected=reinterpret_cast<void*>(GetProcAddress(game,"?ProcessMovements@gCCharacterControl_PS@@MAEXXZ"));
    if(!diagnostic::VirtualSlot(&CharacterControlAccess::ProcessMovements,slot) || slot!=85 ||
       !diagnostic::Readable(table,sizeof(void*)*(slot+1)) || !expected || table[slot]!=expected ||
       !gMovementHook.Prepare(table,slot,reinterpret_cast<void*>(&ProcessPlayerMovements)))
    {Message("movement_virtual_slot_validation_failed; camera tracking remains available");return false;}
    void* expectedAction=reinterpret_cast<void*>(GetProcAddress(game,"?OnAction@gCCharacterControl_PS@@UAEHABUeSGameKeyMessage@eCInpShared@@@Z"));
    if(!diagnostic::VirtualSlot(&gCCharacterControl_PS::OnAction,actionSlot) || actionSlot!=47 ||
       !expectedAction || table[actionSlot]!=expectedAction ||
       !gActionHook.Prepare(table,actionSlot,reinterpret_cast<void*>(&ProcessPlayerAction)))
    {Message("movement_action_virtual_slot_validation_failed; camera tracking remains available");return false;}
    HMODULE engine=GetModuleHandleW(L"Engine.dll");
    constexpr unsigned char engineHash[32]={
        0x99,0x1c,0xf6,0x74,0xa9,0xcf,0x1b,0xc6,0x0c,0xdd,0xc2,0x04,0xbe,0x09,0x93,0xe8,
        0x25,0x13,0x36,0xf4,0xd3,0x47,0x15,0x43,0xa3,0x78,0x46,0xcc,0x68,0x40,0xbf,0x5a};
    if(!engine||!GetModuleFileNameW(engine,path,MAX_PATH)||!diagnostic::FileSha256(path,hash)||memcmp(hash,engineHash,sizeof(hash)))
    {Message("room_scale_Engine_DLL_profile_mismatch");return false;}
    Message(InstallHeroVoiceHooks(game)?"vr_hero_voice_hooks_installed; verified_SPU_speech_channel; legacy_FMOD_exports":"vr_hero_voice_hooks_refused; native_audio_preserved",0,true);
    size_t processSlot=0,worldSlot=0;
    auto** movementTable=reinterpret_cast<void**>(GetProcAddress(game,"??_7gCCharacterMovement_PS@@6B@"));
    void* processExpected=reinterpret_cast<void*>(GetProcAddress(game,"?OnProcess@gCCharacterMovement_PS@@MAEXXZ"));
    void* worldExpected=reinterpret_cast<void*>(GetProcAddress(game,"?OnUpdatedWorldMatrix@gCCharacterMovement_PS@@MAEXXZ"));
    if(!diagnostic::VirtualSlot(&CharacterMovementAccess::OnProcess,processSlot)||processSlot!=73||
       !diagnostic::Readable(movementTable,(processSlot+1)*sizeof(void*))||!processExpected||movementTable[processSlot]!=processExpected||
       !diagnostic::VirtualSlot(&CharacterMovementAccess::OnUpdatedWorldMatrix,worldSlot)||worldSlot!=68||!worldExpected||movementTable[worldSlot]!=worldExpected||
       !gRoomWorldHook.Prepare(movementTable,worldSlot,reinterpret_cast<void*>(&RoomWorldMatrixUpdated))||
       !gPhysicsProcessHook.Prepare(movementTable,processSlot,reinterpret_cast<void*>(&ProcessPlayerPhysics)))
    {Message("room_scale_virtual_slot_validation_failed");return false;}
    if(!gMovementHook.Install()||!gActionHook.Install()||!gRoomWorldHook.Install()||!gPhysicsProcessHook.Install()){
        Message("movement_hooks_install_failed",gMovementHook.error?gMovementHook.error:gActionHook.error);
        gRoomWorldHook.Restore();gPhysicsProcessHook.Restore();gActionHook.Restore();gMovementHook.Restore();return false;
    }
    auto* mouse=reinterpret_cast<unsigned char*>(game)+diagnostic::MouseFilterXRva;
    auto rotation=GetProcAddress(game,"?GetCurrentWishedRotation@gCCameraAI_PS@@IAE?AVbCMatrix@@XZ");
    if(reinterpret_cast<uintptr_t>(rotation)!=reinterpret_cast<uintptr_t>(game)+diagnostic::WishedRotationRva||
       !diagnostic::Readable(rotation,sizeof(diagnostic::WishedRotationPrefix))||memcmp(rotation,diagnostic::WishedRotationPrefix,sizeof(diagnostic::WishedRotationPrefix))||
       !gWishedRotationHook.Install(rotation,reinterpret_cast<void*>(&ProcessControllerWishedRotation),diagnostic::WishedRotationPrefix,sizeof(diagnostic::WishedRotationPrefix))||
       !gMouseFilterXHook.Install(mouse,reinterpret_cast<void*>(&ProcessInstantMouseX),diagnostic::FocusHook::FindPrefix,sizeof(diagnostic::FocusHook::FindPrefix)))Message("vr_instant_turn_hook_refused; native_mouse_preserved",0,true);
    else {
        auto* base=reinterpret_cast<unsigned char const*>(game);
        gNativeTurnInterpolationReady=diagnostic::Readable(base+diagnostic::TurnRampLoadRva,sizeof(diagnostic::TurnRampLoad))&&
            diagnostic::Readable(base+diagnostic::TurnRateMultiplyRva,sizeof(diagnostic::TurnRateMultiply))&&
            !memcmp(base+diagnostic::TurnRampLoadRva,diagnostic::TurnRampLoad,sizeof(diagnostic::TurnRampLoad))&&
            !memcmp(base+diagnostic::TurnRateMultiplyRva,diagnostic::TurnRateMultiply,sizeof(diagnostic::TurnRateMultiply));
        Message("vr_instant_turn_rotation_hook_installed; translation_previews_preserve_pending_turn; scoped_native_slerp_bypass",gNativeTurnInterpolationReady?1:0,true);
    }
    Message("native_locomotion_enabled; no_translation_or_velocity_override; controller_run_sneak_sprint",0,true);
    Message("room_world_matrix_hook_installed; slot68; scoped_alive_actor_standing_reset_suppressed",0,true);
    Message("player_action_virtual_hook_installed; wished_direction_authored_with_head_heading",static_cast<DWORD>(actionSlot),true);
    Message("player_movement_virtual_hook_installed; horizontal_head_direction",static_cast<DWORD>(slot),true);
    return true;
}

bool Version(wchar_t const* path, char const* label)
{
    DWORD ignored = 0;
    DWORD length = GetFileVersionInfoSizeW(path, &ignored);
    if (!length || length > 65536) { Message("missing_or_oversized_version_resource"); return false; }
    BYTE buffer[65536];
    if (!GetFileVersionInfoW(path, 0, length, buffer)) return false;
    VS_FIXEDFILEINFO* version = nullptr;
    UINT size = 0;
    if (!VerQueryValueW(buffer, L"\\", reinterpret_cast<void**>(&version), &size) ||
        size < sizeof(VS_FIXEDFILEINFO) || version->dwSignature != 0xfeef04bd) return false;
    Event event = Record(Kind::Message);
    sprintf_s(event.text, "%s version %u.%u.%u.%u", label,
        HIWORD(version->dwFileVersionMS), LOWORD(version->dwFileVersionMS),
        HIWORD(version->dwFileVersionLS), LOWORD(version->dwFileVersionLS));
    Push(event);
    return version->dwFileVersionMS == MAKELONG(75, 1) && version->dwFileVersionLS == MAKELONG(14, 20108);
}

bool SupportedVersion()
{
    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) return false;
    bool supported = Version(path, "EXE");
    wchar_t const* moduleNames[] = {L"Engine.dll", L"Game.dll"};
    for (auto name : moduleNames)
    {
        HMODULE module = GetModuleHandleW(name);
        if (!module || !GetModuleFileNameW(module, path, MAX_PATH)) return false;
        bool valid = Version(path, name[0] == L'E' ? "Engine.dll" : "Game.dll");
        supported = supported && valid;
        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(reinterpret_cast<BYTE*>(module) + dos->e_lfanew);
        supported = supported && nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386;
    }
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(GetModuleHandleW(nullptr));
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(reinterpret_cast<BYTE*>(dos) + dos->e_lfanew);
    Message((nt->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) ? "EXE_large_address_aware=true" : "EXE_large_address_aware=false");
    Message(GetModuleHandleW(L"paru_s1.dll") || GetModuleHandleW(L"paru_s2.dll") ?
            "Parallel_Universe_native_module_present" : "Parallel_Universe_native_module_not_seen_at_ScriptInit");
    return supported;
}

void ModuleName(void const* address, char (&name)[64])
{
    HMODULE module = nullptr;
    if (!address || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(address), &module))
    { strcpy_s(name, "unmapped_or_private"); return; }
    char path[MAX_PATH]{};
    if (!GetModuleFileNameA(module, path, MAX_PATH)) { strcpy_s(name, "unknown"); return; }
    char const* slash = strrchr(path, '\\');
    strncpy_s(name, slash ? slash + 1 : path, _TRUNCATE);
}

bool InstallHooks()
{
    size_t slots[4]{};
    bool decoded[] = {
        diagnostic::VirtualSlot(&eCApplication::OnProcess, slots[0]),
        diagnostic::VirtualSlot(&ApplicationAccess::OnPreRender, slots[1]),
        diagnostic::VirtualSlot(&ApplicationAccess::OnRenderDraw, slots[2]),
        diagnostic::VirtualSlot(&ApplicationAccess::OnPostRender, slots[3])};
    bool valid = true;
    for (unsigned i = 0; i < 4; ++i)
    {
        Event event = Record(Kind::Message);
        sprintf_s(event.text, "SDK_virtual_slot_%s: %s", HookNames[i], decoded[i] ? "decoded" : "rejected");
        event.code = static_cast<DWORD>(slots[i]);
        Push(event);
        valid = valid && decoded[i];
    }
    if (!valid) { Message("unsupported_compiler_virtual_thunk; hooks not installed"); return false; }
    gApplication = &eCApplication::GetInstance();
    if (!diagnostic::Readable(gApplication, sizeof(void*))) return false;
    auto table = *reinterpret_cast<void***>(gApplication);
    HMODULE engine = GetModuleHandleW(L"Engine.dll"), game = GetModuleHandleW(L"Game.dll");
    auto engineTable = reinterpret_cast<void**>(GetProcAddress(engine, "??_7eCApplication@@6B@"));
    auto gameTable = reinterpret_cast<void**>(GetProcAddress(game, "??_7gCGameApp@@6B@"));
    HMODULE exe = GetModuleHandleW(nullptr);
    wchar_t exePath[MAX_PATH]{};
    unsigned char exeHash[32]{};
    bool nativeFile = GetModuleFileNameW(exe, exePath, MAX_PATH) && diagnostic::FileSha256(exePath, exeHash) &&
                      std::memcmp(exeHash, diagnostic::NativeExeSha256, sizeof(exeHash)) == 0;
    bool nativeApplication = diagnostic::NativeApplicationMatches(exe, table, slots, nativeFile);
    Message(nativeFile ? "native_EXE_SHA256_matches_profile" : "native_EXE_SHA256_unmatched_or_unreadable", 0, true);
    Message(nativeApplication ? "native_aCProtoTypeApp_vtable_verified" : "native_application_profile_not_matched", 0, true);
    char const* expectedNames[] = {
        "?OnProcess@eCApplication@@UAEXXZ", "?OnPreRender@eCApplication@@MAEXXZ",
        "?OnRenderDraw@eCApplication@@MAEXXZ", "?OnPostRender@eCApplication@@MAEXXZ"};
    void* replacements[] = {reinterpret_cast<void*>(&OnProcess), reinterpret_cast<void*>(&OnPreRender),
                           reinterpret_cast<void*>(&OnRenderDraw), reinterpret_cast<void*>(&OnPostRender)};
    // Prepare and diagnose ALL originals before publishing any callback.
    bool allValid = true;
    for (unsigned i = 0; i < 4; ++i)
    {
        for (unsigned j = 0; j < i; ++j)
            if (slots[i] == slots[j]) { Message("duplicate_virtual_slot; hooks not installed"); return false; }
        if (!gHooks[i].Prepare(table, slots[i], replacements[i]))
        { Message("virtual_slot_validation_failed", gHooks[i].error); return false; }
        void* expected = reinterpret_cast<void*>(GetProcAddress(engine, expectedNames[i]));
        void const* resolvedExpected = expected ? diagnostic::ResolveJumpChain(expected) : nullptr;
        void const* resolvedOriginal = diagnostic::ResolveJumpChain(gHooks[i].original);
        bool direct = expected && gHooks[i].original == expected;
        bool resolved = resolvedExpected && resolvedExpected == resolvedOriginal;
        // An IAT detour need not resolve to the current Engine export. Verify
        // the stub's symbolic import and the SDK slot in the base table instead.
        // This preserves the existing imported implementation, including PUP.
        void* importTarget = nullptr;
        bool baseSlot = engineTable && diagnostic::Readable(engineTable + slots[i], sizeof(void*)) &&
                        expected && engineTable[slots[i]] == expected;
        bool namedImport = table == gameTable && baseSlot &&
            diagnostic::NamedImportTarget(game, gHooks[i].original, "Engine.dll", expectedNames[i], importTarget);
        bool nativeOverride = nativeApplication && baseSlot;
        bool accepted = direct || resolved || namedImport || nativeOverride;
        Event event = Record(Kind::Validation, static_cast<int>(i));
        event.code = static_cast<DWORD>(slots[i]);
        event.pointer = reinterpret_cast<uintptr_t>(gHooks[i].original);
        event.pointer2 = reinterpret_cast<uintptr_t>(expected);
        event.resolvedOriginal = reinterpret_cast<uintptr_t>(resolvedOriginal);
        event.resolvedExpected = reinterpret_cast<uintptr_t>(resolvedExpected);
        event.importTarget = reinterpret_cast<uintptr_t>(importTarget);
        ModuleName(gHooks[i].original, event.originalModule);
        ModuleName(importTarget ? importTarget : resolvedOriginal, event.targetModule);
        if (diagnostic::Readable(gHooks[i].original, sizeof(event.originalBytes)))
            std::memcpy(event.originalBytes, gHooks[i].original, sizeof(event.originalBytes));
        strcpy_s(event.text, direct ? "direct_export" : (resolved ? "resolved_jump" :
                           (namedImport ? "verified_named_import" :
                           (nativeOverride ? "verified_native_EXE_override" : "rejected"))));
        Push(event, true);
        allValid = allValid && accepted;
    }
    if (!allValid) { Message("virtual_callback_identity_unverified; no hooks installed"); return false; }
    for (unsigned i = 0; i < 4; ++i)
    {
        if (!gHooks[i].Install())
        {
            Message("hook_install_failed; rolling_back", gHooks[i].error);
            for (unsigned j = 0; j <= i; ++j)
                if (!gHooks[j].Restore()) Message("hook_rollback_failed; module remains pinned", gHooks[j].error);
            return false;
        }
        Event event = Record(Kind::Hook, static_cast<int>(i));
        event.code = static_cast<DWORD>(slots[i]);
        event.pointer = reinterpret_cast<uintptr_t>(gHooks[i].original);
        event.pointer2 = reinterpret_cast<uintptr_t>(gHooks[i].replacement);
        Push(event, true);
    }
    Message("four_application_virtual_observers_installed", 0, true);
    return true;
}

DWORD Setting(wchar_t const* key, DWORD value, DWORD minimum, DWORD maximum, wchar_t const* section = L"Diagnostics")
{
    UINT setting = GetPrivateProfileIntW(section, key, static_cast<INT>(value), gIni);
    return setting < minimum ? minimum : (setting > maximum ? maximum : setting);
}

bool InitializePaths()
{
    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(gModule, path, MAX_PATH);
    if (!length || length >= MAX_PATH) return false;
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) return false;
    *slash = 0;
    if (_snwprintf_s(gIni, _TRUNCATE, L"%s\\Script_Gothic3VRDiagnostics.ini", path) < 0) return false;
    wchar_t folder[MAX_PATH];
    if (_snwprintf_s(folder, _TRUNCATE, L"%s\\Gothic3VRDiagnostics", path) < 0) return false;
    if (!CreateDirectoryW(folder, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
    if (_snwprintf_s(gLogDirectory, _TRUNCATE, L"%s\\logs", folder) < 0) return false;
    if (_snwprintf_s(gRuntimeLoader, _TRUNCATE, L"%s\\openxr_loader.dll", folder) < 0) return false;
    if (!CreateDirectoryW(gLogDirectory, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
    return true;
}

bool SkipNativePickupPause(uintptr_t caller,diagnostic::PickupPause kind,void* args,gCScriptProcessingUnit* spu,GEBool fullStop){
    if(!gVrSettings.fastPickup||!gPickup.Matches(caller,kind)||!args||fullStop)return false;
    __try {
        if(GetCurrentThreadId()!=gApplicationThread.load()||!gHeadControl.enabled||!gConfig.stereoEnabled||
           gStereo.shuttingDown||gStereo.fault||gWorldLifecycle.Blocked()||MenuPanelActive()||
           !gApplication||!gApplication->IsGameRunning()||gCSession::GetInstance().IsPaused())return false;
        if(!gPickup.Skip(caller,kind,args,spu,fullStop,gCSession::GetInstance().GetPlayer()))return false;
        Message("vr_pickup_pause_skipped; native_inventory_transfer_preserved",kind==diagnostic::PickupPause::Wait?0:1,true);return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){Message("vr_pickup_pause_guard_fault; native_pickup_preserved",GetExceptionCode(),true);return false;}
}
GEBool __stdcall FastPickupWait(void* args,gCScriptProcessingUnit* spu,GEBool fullStop){
    if(SkipNativePickupPause(reinterpret_cast<uintptr_t>(_ReturnAddress()),diagnostic::PickupPause::Wait,args,spu,fullStop))return GETrue;
    return gPickup.Dispatch(diagnostic::PickupPause::Wait,args,spu,fullStop);
}
GEBool __stdcall FastPickupAnimation(void* args,gCScriptProcessingUnit* spu,GEBool fullStop){
    if(SkipNativePickupPause(reinterpret_cast<uintptr_t>(_ReturnAddress()),diagnostic::PickupPause::Animation,args,spu,fullStop))return GETrue;
    return gPickup.Dispatch(diagnostic::PickupPause::Animation,args,spu,fullStop);
}
void Initialize()
{
    QueryPerformanceFrequency(&gFrequency);
    QueryPerformanceCounter(&gStarted);
    if (!InitializePaths()) { OutputDebugStringW(L"Gothic3VRDiagnostics: invalid or unwritable log path\n"); return; }
    gConfig.enabled = GetPrivateProfileIntW(L"Diagnostics", L"Enabled", 1, gIni) != 0;
    if (!gConfig.enabled) return;
    gConfig.autoTrace = diagnostic::probes::Enabled&&GetPrivateProfileIntW(L"Diagnostics", L"AutoTrace", 1, gIni) != 0;
    gConfig.sampleMs = Setting(L"SampleIntervalMs", 1000, 100, 60000);
    gConfig.traceFrames = Setting(L"TraceFrames", 120, 1, 600);
    gConfig.maxLogMiB = Setting(L"MaxLogMiB", 16, 1, 64);
    gConfig.trackingEnabled = GetPrivateProfileIntW(L"Tracking", L"Enabled", 1, gIni) != 0;
    gConfig.trackingPollMs = Setting(L"PollIntervalMs", 11, 5, 1000, L"Tracking");
    gConfig.trackingSampleMs = Setting(L"SampleIntervalMs", 100, 20, 60000, L"Tracking");
    gConfig.trackingRetryMs = Setting(L"RetryIntervalMs", 5000, 1000, 60000, L"Tracking");
    gConfig.headControlAllowed=GetPrivateProfileIntW(L"HeadControl",L"Allowed",1,gIni)!=0;
    gConfig.maxPoseAgeMs=Setting(L"MaxPoseAgeMs",50,20,200,L"HeadControl");
    gConfig.headDirectedMovement=GetPrivateProfileIntW(L"Locomotion",L"HeadDirected",1,gIni)!=0;
    gConfig.stereoEnabled=GetPrivateProfileIntW(L"Stereo",L"Enabled",1,gIni)!=0;
    gConfig.keepFirstPerson=GetPrivateProfileIntW(L"Stereo",L"KeepFirstPerson",1,gIni)!=0;
    gConfig.unitsPerMeter=float(Setting(L"UnitsPerMeter",100,10,1000,L"Stereo"));
    ReloadFirstPersonHeight(false);
    // Keep observer code loaded even if another mod later chains into it.
    HMODULE pinned = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&Initialize), &pinned)) return;
    gWake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!gWake) return;
    gLogging.store(true);
    uintptr_t writer = _beginthreadex(nullptr, 0, &Writer, nullptr, 0, nullptr);
    if (!writer) { gLogging.store(false); CloseHandle(gWake); gWake = nullptr; return; }
    CloseHandle(reinterpret_cast<HANDLE>(writer));
    Push(Record(Kind::Start), true);
    if constexpr(diagnostic::probes::Dx11ValidationOnly)Message("vr_runtime_build=dx11_owned_validation; mode5_only; no_CPU_GPU_profilers; native_draws_preserved",0,true);
    else if constexpr(!diagnostic::probes::Enabled)Message("vr_runtime_build=clean; no_experimental_render_hooks; no_CPU_GPU_profilers; native_occlusion_results",0,true);
    if (!SupportedVersion()) { Message("unsupported_game_version; no hooks installed"); return; }
    // A request is consumed once at startup, outside all game callbacks.
    wchar_t movementRequest[MAX_PATH];
    if(_snwprintf_s(movementRequest,_TRUNCATE,L"%s\\native-movement.request",gLogDirectory)>=0&&DeleteFileW(movementRequest)){
        gMovementTrace.Arm();Message("vr_native_movement_trace_armed; 120s_from_gameplay; 4Hz_CPU_snapshots_only; no_state_changes",0,true);
    }
    if (gConfig.autoTrace) gTraceEnd.store(gConfig.traceFrames);
    if (!InstallHooks()) Message("diagnostics_initialization_incomplete");
    else {
        if(gConfig.headControlAllowed && gConfig.trackingEnabled)InstallMovementHook();
        if(gActionHook.installed&&!gMelee.Install(reinterpret_cast<void*>(&PhysicalSwordDamage),reinterpret_cast<void*>(&PhysicalSwordHealth),TrackedArmsRecord))
            Message("vr_physical_melee_profile_or_callback_refused; native_combat_preserved",0,true);
        if(gActionHook.installed&&!gBow.Install(reinterpret_cast<void*>(&PhysicalBowProjectile),TrackedArmsRecord))
            Message("vr_physical_bow_native_profile_refused; native_ranged_combat_preserved",0,true);
        gMagic.Initialize(gBow.ready,TrackedArmsRecord);
        if(gPickup.Install(reinterpret_cast<void*>(&FastPickupWait),reinterpret_cast<void*>(&FastPickupAnimation)))
            Message("vr_fast_pickup_installed; hero_only; skip_take_wait_and_animation; native_inventory_and_rules",0,true);
        else Message("vr_fast_pickup_profile_refused; native_pickup_preserved",0,true);
        if(!InstallMovieHook())Message("vr_movie_PlayVideo_profile_refused; native_movies_preserved",0,true);
        if(InstallStereoHook()&&gConfig.stereoEnabled&&gConfig.headControlAllowed&&gConfig.trackingEnabled&&gVrSettings.autoStart){
            gHeadControl.enabled=true;gHeadControl.recenterPending=true;
            Message("VR_auto_start_enabled; startup_menu_UI_and_world_stereo",0,true);
        }
    }
    if (gConfig.trackingEnabled && !gConfig.stereoEnabled)
    {
        uintptr_t tracker = _beginthreadex(nullptr, 0, &TrackingWorker, nullptr, 0, nullptr);
        if (tracker) CloseHandle(reinterpret_cast<HANDLE>(tracker));
        else Message("OpenXR_worker_start_failed");
    }
}
}

extern "C" __declspec(dllexport) gSScriptInit const* GE_STDCALL ScriptInit()
{
    static gSScriptInit emptyScriptList;
    if (InterlockedCompareExchange(&gInitialized, 1, 0) == 0) Initialize();
    return &emptyScriptList;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) gModule = module;
    // No hooks, workers, waits, SDK calls, or disk I/O under the loader lock.
    // The module is pinned in ScriptInit; process teardown releases all resources.
    return TRUE;
}





