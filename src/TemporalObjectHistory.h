#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace diagnostic::temporal {
// Values copied while a native animated render context is alive. Never retain
// SDK objects or dereference last frame's object/buffer addresses.
struct Object {
    std::array<uint32_t,5> id{}; // bCPropertyID core (16 bytes) + count
    uintptr_t context=0,visual=0;
    std::array<float,16> world{};
    bool Valid()const noexcept {
        if(!context||!visual||!(id[0]|id[1]|id[2]|id[3]))return false;
        for(float f:world)if(!std::isfinite(f))return false;
        return true;
    }
};
struct Geometry {
    uint32_t primitive=0;int32_t base=0;uint32_t minimum=0,vertices=0,first=0,count=0;
};
struct DrawKey {
    std::array<uint32_t,5> id{};uintptr_t context=0,visual=0,vertices=0,indices=0;
    uint64_t shader=0,declaration=0;
    std::array<uint64_t,4> geometryVersion{}; // observed VB/IB object + content versions
    uintptr_t target=0,depth=0;
    std::array<uint32_t,5> pass{}; // depth-write, depth-func, cull, alpha-test, color-write
    uint32_t offset=0,stride=0,indexFormat=0;Geometry draw{};
    bool operator==(DrawKey const& b)const noexcept {
        return id==b.id&&context==b.context&&visual==b.visual&&vertices==b.vertices&&indices==b.indices&&
            shader==b.shader&&declaration==b.declaration&&geometryVersion==b.geometryVersion&&target==b.target&&depth==b.depth&&pass==b.pass&&offset==b.offset&&stride==b.stride&&indexFormat==b.indexFormat&&
            draw.primitive==b.draw.primitive&&draw.base==b.draw.base&&draw.minimum==b.draw.minimum&&
            draw.vertices==b.draw.vertices&&draw.first==b.draw.first&&draw.count==b.draw.count;
    }
};
struct SkinLayout {unsigned projectionRegister=0;explicit operator bool()const{return projectionRegister!=0;}};
inline SkinLayout Layout(uint64_t hash)noexcept {
    // Exact bytecode hashes observed and disassembled in Gothic 3 1.75.14:
    // four weights, four indices, 64 x three float4 affine columns at c0..191.
    switch(hash){case 0x8e4a823574960350ull:case 0x8879d99ef7dcd4f9ull:return {192};
        case 0x1d21092550ea2dd4ull:case 0x7faf9fa857237e52ull:return {199};default:return {};}
}
struct SkinPose {
    std::array<float,768> bones{};
    std::array<float,16> clip{},world{};
    bool Valid()const noexcept {
        for(float f:bones)if(!std::isfinite(f))return false;
        for(float f:clip)if(!std::isfinite(f))return false;
        for(float f:world)if(!std::isfinite(f))return false;
        float norm=0;for(float f:clip)norm+=std::fabs(f);return norm>0;
    }
};
inline bool Same(SkinPose const& a,SkinPose const& b)noexcept {
    return a.bones==b.bones&&a.clip==b.clip&&a.world==b.world;
}
enum class HistoryReason {Ready,FirstObservation,Gap,Ambiguous,Teleport,Invalid};
inline char const* Reason(HistoryReason r)noexcept {
    switch(r){case HistoryReason::Ready:return "ready";case HistoryReason::FirstObservation:return "first_observation";
    case HistoryReason::Gap:return "frame_gap_or_reset";case HistoryReason::Ambiguous:return "ambiguous_draw";
    case HistoryReason::Teleport:return "root_discontinuity";default:return "invalid_pose";}
}
// Diagnostic bound, not complete scene coverage. Storage allocated when the
// one-shot probe is armed, never from Observe on the native draw path.
class ObjectHistory {
public:
    static constexpr unsigned Limit=256;
    struct Entry {DrawKey key;SkinPose pose;int previous=-1;unsigned observations=1;bool ambiguous=false;HistoryReason reason=HistoryReason::FirstObservation;};
    struct Frame {uint64_t serial=0,epoch=0;unsigned count=0,dropped=0,invalid=0;std::array<Entry,Limit> entries;};
private:
    struct Eye {std::array<Frame,2> frames;unsigned current=0;bool begun=false,continuous=false;};
    std::array<Eye,2> eyes;
public:
    void Reset()noexcept {for(auto& e:eyes){e.begun=e.continuous=false;for(auto& f:e.frames)f.serial=f.count=f.dropped=f.invalid=0;}}
    void Begin(unsigned eye,uint64_t frame,uint64_t epoch=1)noexcept {
        if(eye>1)return;auto& e=eyes[eye];auto const& old=e.frames[e.current];
        if(e.begun&&old.serial==frame&&old.epoch==epoch)return;
        e.continuous=e.begun&&old.serial+1==frame&&old.epoch==epoch;
        e.current^=1;e.begun=true;auto& now=e.frames[e.current];now.serial=frame;now.epoch=epoch;now.count=now.dropped=now.invalid=0;
    }
    Frame const& Current(unsigned eye)const noexcept {return eyes[eye].frames[eyes[eye].current];}
    Frame const& Previous(unsigned eye)const noexcept {return eyes[eye].frames[eyes[eye].current^1];}
    Entry const* Add(unsigned eye,DrawKey const& key,SkinPose const& pose)noexcept {
        if(eye>1||!eyes[eye].begun)return nullptr;auto& e=eyes[eye];auto& now=e.frames[e.current];
        if(!pose.Valid()){++now.invalid;for(unsigned i=0;i<now.count;++i)if(now.entries[i].key==key){now.entries[i].ambiguous=true;now.entries[i].previous=-1;now.entries[i].reason=HistoryReason::Invalid;}return nullptr;}
        for(unsigned i=0;i<now.count;++i){auto& found=now.entries[i];if(!(found.key==key))continue;
            ++found.observations;if(!Same(found.pose,pose)){found.ambiguous=true;found.previous=-1;found.reason=HistoryReason::Ambiguous;}return &found;}
        if(now.count>=Limit){++now.dropped;return nullptr;}
        auto& out=now.entries[now.count++];out.key=key;out.pose=pose;out.previous=-1;out.observations=1;out.ambiguous=false;
        out.reason=e.continuous?HistoryReason::FirstObservation:HistoryReason::Gap;
        if(!e.continuous)return &out;
        auto const& previous=Previous(eye);
        for(unsigned i=0;i<previous.count;++i){auto const& old=previous.entries[i];if(!(old.key==key))continue;
            if(old.ambiguous){out.reason=HistoryReason::Ambiguous;break;}
            double distance=0;for(unsigned k=12;k<15;++k){double d=pose.world[k]-old.pose.world[k];distance+=d*d;}
            if(distance>1000.*1000.){out.reason=HistoryReason::Teleport;break;}
            out.previous=int(i);out.reason=HistoryReason::Ready;break;
        }return &out;
    }
};
struct Vertex {float position[4]{0,0,0,1};float weights[4]{1,0,0,0};unsigned indices[4]{};};
// Mirror the observed SM3 dp4 skinning and projection convention, rather than
// interpreting GPU constant registers as the SDK's raw row-major matrix.
inline bool Project(SkinPose const& pose,Vertex const& vertex,float (&clip)[4])noexcept {
    float world[4]{0,0,0,1},sum=0;
    for(unsigned w=0;w<4;++w){float weight=vertex.weights[w];if(!std::isfinite(weight)||weight<0||vertex.indices[w]>=64)return false;sum+=weight;
        for(unsigned c=0;c<3;++c)for(unsigned j=0;j<4;++j)world[c]+=weight*vertex.position[j]*pose.bones[vertex.indices[w]*12+c*4+j];}
    if(std::fabs(sum-1)>0.001f)return false;
    for(unsigned c=0;c<4;++c){clip[c]=0;for(unsigned j=0;j<4;++j)clip[c]+=world[j]*pose.clip[c*4+j];if(!std::isfinite(clip[c]))return false;}
    return clip[3]>1e-5f;
}
inline bool MotionPixels(SkinPose const& current,SkinPose const& previous,Vertex const& vertex,unsigned width,unsigned height,float (&motion)[2])noexcept {
    float a[4],b[4];if(!width||!height||!Project(current,vertex,a)||!Project(previous,vertex,b))return false;
    // Top-left texture origin; NGX convention is previous pixel - current pixel.
    motion[0]=(b[0]/b[3]-a[0]/a[3])*.5f*width;
    motion[1]=(a[1]/a[3]-b[1]/b[3])*.5f*height;
    return std::isfinite(motion[0])&&std::isfinite(motion[1]);
}
}
