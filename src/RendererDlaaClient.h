#pragma once
#include <d3d11.h>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <array>
namespace g3vr {
enum class DlaaInputSource { GameRenderer, SyntheticValidation };
enum class DlaaColorEncoding { Unknown, Linear, Srgb };
// The experimental policy explicitly marks unsupported object motion reactive
// and uses camera reprojection there. It NEVER claims complete object motion.
enum class DlaaMotionPolicy { ExactScene, ExperimentalReactiveFallback };
enum DlaaReadiness : uint32_t {
    DlaaJitterApplied=1, DlaaFullSceneMotion=2, DlaaWorldBeforeHud=4,
    DlaaStandardDepth=8, DlaaReactiveMaterialsHandled=16, DlaaReady=31
};
struct DlaaEyeInput {
    ID3D11Texture2D* color=nullptr; // RGBA16F linear or BGRA/RGBA8 explicitly encoded.
    ID3D11Texture2D* depth=nullptr; // Standard [0,1] device depth, R32F.
    ID3D11Texture2D* motion=nullptr; // RGBA16F: previous-current pixels, occupied, history-valid.
    uint64_t frame=0,epoch=0,displayTimeNs=0;
    unsigned eye=2;uint32_t readiness=0;
    DlaaColorEncoding colorEncoding=DlaaColorEncoding::Unknown;
    ID3D11Texture2D* reactive=nullptr; // R8_UNORM, BiasCurrentColorMask. It does not prove missing geometry motion.
    std::array<float,16> currentClipToPreviousClip{};
    std::array<float,2> previousMinusCurrentRasterJitter{};
    bool cameraHistory=false;
    std::array<uint32_t,4> protectedOverlay{}; // Pixel rectangle L,T,R,B; zero disables it.
};
struct DlaaStereoFrame {
    uint64_t serial=0;DlaaEyeInput eyes[2];float jitterX=0,jitterY=0,deltaMs=0;
    uint32_t resetMask=0;
    DlaaMotionPolicy motionPolicy=DlaaMotionPolicy::ExactScene;
};
// GPU-only packing, bounded x86->x64 NGX exchange, independent stereo outputs.
// ExactScene requires complete inputs. ExperimentalReactiveFallback explicitly
// accepts partial object motion and biases unsupported pixels to current color.
// Constructing this client alone never enables projection jitter or game AA.
class RendererDlaaClient {
    struct Impl;std::unique_ptr<Impl> impl;
public:
    struct Result {uint64_t serial=0;uint32_t resetMask=0;double gpuMs[2]{};};
    RendererDlaaClient();~RendererDlaaClient();
    RendererDlaaClient(RendererDlaaClient const&)=delete;
    void Initialize(ID3D11Device*,ID3D11DeviceContext*,unsigned width,unsigned height,
        std::filesystem::path const& worker,DlaaInputSource source=DlaaInputSource::GameRenderer);
    // Invalid input is rejected BEFORE changing GPU data / sending a frame.
    // Transport errors disable the session; Output() then returns nullptr.
    Result Evaluate(DlaaStereoFrame const&);
    ID3D11Texture2D* Output(unsigned eye)const noexcept; // Private linear RGBA16F.
    // Copies ONLY the requested latest eye/frame into an XR-compatible sRGB
    // BGRA target. The application adds HUD/UI afterwards at native resolution.
    void Present(unsigned eye,uint64_t serial,ID3D11Texture2D* target);
    void Reset()noexcept;
};
}
