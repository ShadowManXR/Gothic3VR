#include "OwnedStoneAtlas.h"
#include <vector>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <algorithm>
using namespace g3vr::dx11;
namespace {
void Need(bool value,char const* message){if(!value)throw std::runtime_error(message);}
struct Producer {
    std::vector<unsigned char> bytes=std::vector<unsigned char>(2097*24);
    unsigned calls=0;bool valid=true;
    static bool __cdecl Copy(void* context,void* target,unsigned vertices,unsigned bytes)noexcept{
        auto& s=*static_cast<Producer*>(context);++s.calls;
        if(!s.valid||vertices!=2097||bytes!=s.bytes.size())return false;
        std::memcpy(target,s.bytes.data(),bytes);return true;
    }
    NativeLightmapSource Source(){return {this,&Copy};}
};
}
int main()try{
    constexpr size_t bytes=2097*24;
    Producer source;StoneAtlasVersion v{5,10,1,1};OwnedStoneAtlasCache cache(5,2,4*bytes);
    auto first=cache.Publish(v,source.Source());Need(first&&cache.Captures()==1&&source.calls==1,"Initial publication failed");
    auto identity=first.get();auto firstBytes=std::vector<unsigned char>(first->Data(),first->Data()+first->Bytes());
    for(unsigned i=0;i<10000;++i){auto reused=cache.Acquire(v);Need(reused.get()==identity,"Stable version reallocated");}
    Need(source.calls==1&&cache.LiveBytes()==bytes,"Acquire touched provider or multiplied storage");
    Need(!cache.Publish(v,source.Source())&&source.calls==1,"Duplicate revision republished");
    source.bytes[0]=99;source.valid=false;
    Need(cache.Acquire(v).get()==first.get()&&std::memcmp(first->Data(),firstBytes.data(),bytes)==0,"Borrowed producer mutation changed immutable version");
    auto next=v;++next.revision;Need(!cache.Publish(next,source.Source())&&!cache.Acquire(next)&&cache.Acquire(v),"Failed producer replaced the good version");
    source.valid=true;auto second=cache.Publish(next,source.Source());Need(second&&second->Data()[0]==99&&!cache.Acquire(v)&&cache.Acquire(next),"Content revision failed to invalidate old lookup");
    Need(first->Data()[0]==0&&cache.LiveBytes()==2*bytes,"Pinned old version mutated or escaped budget");
    auto stale=next;--stale.generation;Need(!cache.Publish(stale,source.Source()),"Stale allocation generation accepted");
    auto reusedAddress=next;++reusedAddress.generation;reusedAddress.revision=1;
    auto third=cache.Publish(reusedAddress,source.Source());Need(third&&!cache.Acquire(next),"Allocation reuse failed to invalidate old lookup");
    Need(cache.Invalidate(v.source)&&!cache.Acquire(reusedAddress),"Explicit mutation invalidation left a hit");
    Need(!cache.Publish(reusedAddress,source.Source()),"Invalidated revision resurrected");
    auto fresh=reusedAddress;++fresh.revision;Need(bool(cache.Publish(fresh,source.Source())),"New revision after invalidation refused");
    auto other=fresh;++other.source;
    Need(!cache.Publish(other,source.Source()),"Pinned version byte budget ignored");
    first.reset();second.reset();third.reset();Need(cache.LiveBytes()==bytes,"Pinned byte accounting not released");
    auto another=cache.Publish(other,source.Source());Need(bool(another),"Released budget not reusable");
    auto over=other;++over.source;Need(!cache.Publish(over,source.Source()),"Source-count limit ignored");
    Need(!cache.Reset(5)&&cache.Reset(6)&&!cache.Acquire(fresh)&&!cache.Publish(fresh,source.Source()),"Epoch reset did not reject stale data");
    Need(another->Version()==other&&cache.LiveBytes()==bytes,"Reset mutated pinned version or lost live budget");
    another.reset();Need(cache.LiveBytes()==0,"Reset pinned lifetime leaked budget");
    StoneAtlasVersion current{6,10,1,1};uint32_t nan=0x7fc00000u;
    std::memcpy(source.bytes.data()+4,&nan,4);Need(!cache.Publish(current,source.Source()),"NaN coordinate published");
    uint32_t infinity=0x7f800000u;std::memcpy(source.bytes.data()+4,&infinity,4);Need(!cache.Publish(current,source.Source()),"Infinite coordinate published");
    std::fill(source.bytes.begin(),source.bytes.end(),static_cast<unsigned char>(0));std::memcpy(source.bytes.data(),&nan,4);
    auto packed=cache.Publish(current,source.Source());Need(bool(packed),"Packed color misclassified as NaN coordinate");
    Need(OwnedStoneAtlas::Resolve(packed->Source())==packed.get()&&!OwnedStoneAtlas::Resolve(source.Source()),"Borrowed source mistaken for owned immutable source");
    std::vector<unsigned char> output(bytes);auto bound=packed->Source();
    Need(bound.copy(bound.context,output.data(),2097,unsigned(bytes))&&output==source.bytes,"Owned callback copied different bytes");
    Need(!bound.copy(bound.context,output.data(),2096,unsigned(bytes))&&!bound.copy(bound.context,nullptr,2097,unsigned(bytes))&&!bound.copy(bound.context,output.data(),2097,unsigned(bytes)-1),"Owned callback ignored layout");
    // External ownership must survive cache and producer destruction.
    std::shared_ptr<OwnedStoneAtlas const> survivor;
    {OwnedStoneAtlasCache scoped(1);Producer temporary;temporary.bytes[0]=42;survivor=scoped.Publish({1,2,3,4},temporary.Source());}
    Need(survivor&&survivor->Data()[0]==42,"Version did not own producer bytes after cache destruction");survivor.reset();
    std::cout<<"PASS immutable atlas versions, generation/epoch invalidation, pinned budgets and 10000 zero-copy lookups\n";return 0;
}catch(std::exception const& error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}
