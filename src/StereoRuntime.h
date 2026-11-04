#pragma once
#include <windows.h>
#include <d3d9.h>
#include "TrackingData.h"
#include "VrSettings.h"
#include "ControllerData.h"
#include "QuickRadial.h"
#include "RadialItems.h"
#include "FramePerformance.h"
#include "UpdatePerformance.h"
#include "NativePhasePerformance.h"
#include "SceneWorkProbe.h"
#include "TemporalObjectHistory.h"
#include "TemporalInstanceIdentity.h"
namespace g3vr::dx11 {struct NativeStaticSnapshot;struct NativeLightmapSource;}
namespace diagnostic
{
struct StereoEyePose {float position[3]{},quaternion[4]{},fov[4]{};}; // xyzw; left/right/up/down
struct StereoFrame {StereoEyePose eyes[2];HeadPose head;bool focused=false,originChanged=false;};
enum class StereoBegin {Waiting,Ready,Failed};
class StereoRuntime
{
public:
    StereoRuntime()=default;
    bool Connect(IDirect3DDevice9* device,wchar_t const* loader,wchar_t const* logFile,VrSettings const& settings=VrSettings{}) noexcept;
    StereoBegin Begin(StereoFrame& frame) noexcept;
    bool Capture(unsigned eye) noexcept;
#ifdef G3VR_STEREO_FRAME_CAPTURE
    void CaptureCamera(unsigned eye,float const* view,float const* projection) noexcept;
#endif
    bool BeginUi() noexcept;
    bool CaptureCinema() noexcept;
    bool EndUi() noexcept;
    void SetUiLayout(UiLayout const& layout) noexcept;
    void SetFoveation(int mode,int diameterPercent,int transitionPercent=10) noexcept;
    void SetUiDiagnosticState(int state) noexcept;
    void SetRadial(RadialState const& state,int scalePercent=80,RadialItems const* items=nullptr) noexcept;
    void SetWorldStatus(int status) noexcept;
    bool Submit() noexcept;
    void NoteTiming(performance::Stage stage,uint64_t started) noexcept;
    void BeginCpuUpdate(int context) noexcept;
    void NoteCpuUpdate(performance::UpdateStage stage,uint64_t started) noexcept;
    void EndCpuUpdate() noexcept;
    void NoteNativePhase(performance::NativePhase stage,uint64_t started,int context) noexcept;
    void NoteSceneStall(char const* scope,uint64_t started,int context,performance::SceneWorkState const& before,performance::SceneWorkState const& after,uint64_t ended) noexcept;
    void BeginGpuEye(unsigned eye,int context) noexcept;
    bool PrepareTemporalEye(unsigned eye,int context) noexcept;
    bool ApplyTemporalProjection(unsigned eye,float* sdkProjection) noexcept;
    void UnjitterTemporalProjection(unsigned eye,float* sdkProjection) noexcept;
    void ProtectTemporalReticle(unsigned eye,bool visible,float u,float v) noexcept;
    bool TemporalDiagnosticActive() const noexcept;
    void EndGpuEye(unsigned eye) noexcept;
    bool TemporalEyeActive() const noexcept;
    void RecordTemporalCamera(float const* view,float const* projection) noexcept;
    void CaptureTemporalOpaqueDepth() noexcept;
    unsigned BeginTemporalObject(temporal::Object const& object) noexcept;
    void EndTemporalObject(unsigned token) noexcept;
    unsigned BeginTemporalInstances(temporal::InstanceEntity const* entities,unsigned count) noexcept;
    void EndTemporalInstances(unsigned token) noexcept;
    bool StaticTrialWantsContext(uintptr_t liveContextCookie)const noexcept;
    bool StaticTrialWantsMaterial(uintptr_t liveContextCookie)const noexcept;
    unsigned BeginStaticTrialMaterial(uintptr_t liveContextCookie,char const* material)noexcept;
    bool StaticTrialMaterialObservationEnabled()const noexcept;
    bool StaticTrialWantsMaterialEntity(temporal::InstanceEntity const&)const noexcept;
    unsigned BeginStaticTrialDeferredMaterial(temporal::InstanceEntity const*,char const* material,g3vr::dx11::NativeLightmapSource const&)noexcept;
    void EndStaticTrialMaterial(unsigned token)noexcept;
    void ObserveStaticTrial(uintptr_t liveContextCookie,g3vr::dx11::NativeStaticSnapshot const&,float const* view,float const* projection)noexcept;
    void StaticTrialSolidBoundary(float const* view,float const* projection)noexcept;
    void Cancel() noexcept;
    void Disconnect() noexcept;
    bool Connected() const {return implementation!=nullptr;}
    char const* Error() const {return error;}
    uint64_t Submissions() const;
    ControllerFrame Controllers() const noexcept;
    bool Haptic(unsigned hand,float amplitude=.45f,int milliseconds=45) noexcept;
private:
    void* implementation=nullptr;char error[192]{};uint64_t generation=0;
};
}
