#pragma once
#include "NativeTrialBinding.h"
#include <limits>

namespace g3vr::dx11 {
struct NativeBounds {float low[3]{},high[3]{};};
inline NativeBounds NativeMeshBounds(Mesh const& mesh){
    NativeBounds b;for(unsigned j=0;j<3;++j){b.low[j]=std::numeric_limits<float>::max();b.high[j]=-b.low[j];}
    for(auto const& section:mesh.sections)for(auto const& vertex:section.vertices)for(unsigned j=0;j<3;++j){b.low[j]=std::min(b.low[j],vertex.position[j]);b.high[j]=std::max(b.high[j],vertex.position[j]);}return b;
}
// Rank a visible central bounding box, rather than latching the first imported
// render context (which can be behind the entire town). This is a selection
// heuristic, never an occlusion/material-equivalence or replacement proof.
inline float NativeCandidateScore(NativeBounds const& bounds,NativeStaticSnapshot const& sample,Camera const& camera)noexcept{
    DirectX::XMFLOAT4X4 world;std::memcpy(&world,sample.worldMeters.data(),64);
    auto m=DirectX::XMLoadFloat4x4(&world)*DirectX::XMLoadFloat4x4(&camera.viewProjection);
    float left=2,right=-2,top=2,bottom=-2;unsigned nearRejected=0,farRejected=0;
    for(unsigned i=0;i<8;++i){DirectX::XMFLOAT4 p;DirectX::XMStoreFloat4(&p,DirectX::XMVector4Transform(DirectX::XMVectorSet(i&1?bounds.high[0]:bounds.low[0],i&2?bounds.high[1]:bounds.low[1],i&4?bounds.high[2]:bounds.low[2],1),m));
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||!std::isfinite(p.w)||p.w<=.01f)return 0;
        nearRejected+=p.z<0;farRejected+=p.z>p.w;float x=p.x/p.w,y=p.y/p.w;
        left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);
    }
    if(nearRejected==8||farRejected==8)return 0;
    left=std::max(left,-1.f);right=std::min(right,1.f);top=std::max(top,-1.f);bottom=std::min(bottom,1.f);
    float area=(right-left)*(bottom-top),x=(right+left)*.5f,y=(top+bottom)*.5f;
    if(right<=left||bottom<=top||area<.01f||std::abs(x)>.65f||std::abs(y)>.85f)return 0;
    return area/(1+12*(x*x+y*y));
}
struct NativeTrialCandidate {
    uintptr_t cookie=0;NativeStaticSnapshot snapshot{};Camera camera{};std::array<float,16> view{},projection{};float score=0;
};
class NativeTrialSelection {
    std::array<NativeTrialCandidate,64> candidates{};unsigned count=0;mutable unsigned probes=0;
public:
    void Reset()noexcept{count=probes=0;}
    bool Probe()const noexcept{return probes++<4096;}
    void Offer(uintptr_t cookie,NativeStaticSnapshot const& sample,Camera const& camera,float const* view,float const* projection,float score)noexcept{
        if(!cookie||!(score>0)||!std::isfinite(score)||!view||!projection)return;
        unsigned index=0;for(;index<count;++index)if(candidates[index].cookie==cookie&&SameNativePass(candidates[index].view.data(),view)&&SameNativePass(candidates[index].projection.data(),projection))break;
        if(index==count){if(count==candidates.size())return;++count;}
        auto& c=candidates[index];c.cookie=cookie;c.snapshot=sample;c.camera=camera;c.score=score;std::copy_n(view,16,c.view.begin());std::copy_n(projection,16,c.projection.begin());
    }
    NativeTrialCandidate const* Best(float const* view,float const* projection)const noexcept{
        NativeTrialCandidate const* best=nullptr;for(unsigned i=0;i<count;++i){auto const& c=candidates[i];if(SameNativePass(c.view.data(),view)&&SameNativePass(c.projection.data(),projection)&&(!best||c.score>best->score))best=&c;}return best;
    }
    unsigned Count()const noexcept{return count;}
    NativeTrialCandidate const& At(unsigned index)const noexcept{return candidates[index];}
};
}
