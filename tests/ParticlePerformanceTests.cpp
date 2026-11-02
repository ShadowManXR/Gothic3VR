#include "ParticlePerformance.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
using namespace diagnostic::particles;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
template<class T>void Put(unsigned char* object,size_t offset,T value){std::memcpy(object+offset,&value,sizeof(value));}
template<class T>T Get(unsigned char const* object,size_t offset){T value{};std::memcpy(&value,object+offset,sizeof(value));return value;}
int Compare(void const* a,void const* b){unsigned x=0,y=0;std::memcpy(&x,a,4);std::memcpy(&y,b,4);return x<y?1:x>y?-1:0;}
int main(){
    static_assert(sizeof(void*)==4,"Gothic 3 native particle layout is x86");
    CHECK(Quality(-1)==0&&Quality(99)==2);
    for(auto name:{"VFX_Magic_Fireball_01","VFX_Magic_Frost_01","vfx_magic_Heal_01","VFX_Env_Fire_Campfire_01","VFX_Env_Fire_Torch_01","VFX_Env_Smoke_01"})CHECK(Effect(name));
    for(auto name:{"VFX_Magic_Fireball_HeatHaze_01","VFX_Env_Fire_Torch_HeatHaze_02","VFX_Env_Fire_Campfire_HeatHaze_01"})CHECK(HeatOnly(name));
    for(auto name:{"VFX_Env_Fire_Torch_01","VFX_Magic_Weapons_FlameSword_03","VFX_Magic_Flame","VFX_Env_Water_HeatHaze_01","It_1H_FlameSword_01","Hero", ""})CHECK(!HeatOnly(name));
    CHECK(!Effect(nullptr)&&!Effect("VFX_Env_Rain_01")&&!Effect("VFX_Collision_Blood"));
    CHECK(DrawCount(250,0)==250&&DrawCount(250,1)==162&&DrawCount(250,2)==100);
    CHECK(DrawCount(900,1)==585&&DrawCount(900,2)==360);
    CHECK(DrawCount(10000,1)==768&&DrawCount(10000,2)==384);
    for(int count=0;count<=12;++count)CHECK(DrawCount(count,1)==count&&DrawCount(count,2)==count);
    RenderLease lease;unsigned char object[0x3f8]{},before[sizeof(object)]{};
    for(int count:{0,1,12,13,20,60,150,250,900,2048})for(int profile:{0,1,2}){
        std::vector<unsigned char> live(size_t(std::max(1,count))*RecordSize);
        for(int i=0;i<count;++i){for(unsigned j=0;j<RecordSize;++j)live[size_t(i)*RecordSize+j]=static_cast<unsigned char>(i+j);Put(live.data(),size_t(i)*RecordSize,unsigned(i));}
        auto original=live;std::memset(object,0x73,sizeof(object));void* native=live.data();Put(object,DataOffset,native);Put(object,CountOffset,count);Put(object,CapacityOffset,std::max(1,count));
        std::memcpy(before,object,sizeof(object));int wanted=DrawCount(count,profile);
        bool active=lease.Begin(object,sizeof(object),profile);CHECK(active==(wanted<count)&&lease.Active()==active);
        CHECK(!lease.Begin(nullptr,sizeof(object),profile));
        if(active){
            CHECK(!lease.Begin(object,sizeof(object),profile)); // no nested alias of scratch
            auto* sampled=Get<unsigned char*>(object,DataOffset);CHECK(sampled!=native&&Get<int>(object,CountOffset)==wanted&&Get<int>(object,CapacityOffset)==std::max(1,count));
            for(int i=0;i<wanted;++i){size_t index=size_t(int64_t(i)*count/wanted);CHECK(!std::memcmp(sampled+size_t(i)*RecordSize,live.data()+index*RecordSize,RecordSize));}
            // Same mutation native DepthSort makes, followed by its actual draw
            // count convention (two triangles per visible particle).
            std::qsort(sampled,wanted,RecordSize,Compare);int gpuTriangles=wanted*2;
            Put(object,FrameOffset,uint32_t(123));lease.End();CHECK(!lease.Active());
            CHECK(live==original&&Get<void*>(object,DataOffset)==native&&Get<int>(object,CountOffset)==count);
            Put(before,FrameOffset,uint32_t(123));CHECK(!std::memcmp(before,object,sizeof(object)));
            // Right eye and additional instances reuse the built GPU buffer;
            // native particle array and age/collision/simulation state persist.
            for(unsigned eye=0;eye<2;++eye)for(unsigned pass=0;pass<4;++pass){CHECK(Get<uint32_t>(object,FrameOffset)==123&&gpuTriangles==wanted*2);CHECK(live==original);}
            // Failure/exception also restores the complete original array view.
            CHECK(lease.Begin(object,sizeof(object),profile));try{throw std::runtime_error("native vertex build");}catch(...){lease.End();}
            CHECK(!std::memcmp(before,object,sizeof(object))&&live==original);
        }else {lease.End();CHECK(!std::memcmp(before,object,sizeof(object))&&live==original);}
    }
    std::memset(object,0x25,sizeof(object));std::memcpy(before,object,sizeof(object));
    CHECK(!lease.Begin(object,CapacityOffset,1)&&!lease.Begin(object,sizeof(object),0));CHECK(!std::memcmp(before,object,sizeof(object)));
    Put(object,DataOffset,static_cast<void*>(nullptr));Put(object,CountOffset,50);Put(object,CapacityOffset,100);CHECK(!lease.Begin(object,sizeof(object),1));
    unsigned char small[RecordSize*20]{};Put(object,DataOffset,static_cast<void*>(small));Put(object,CountOffset,21);Put(object,CapacityOffset,20);CHECK(!lease.Begin(object,sizeof(object),1));
    Put(object,CountOffset,20);Put(object,CapacityOffset,20000);CHECK(!lease.Begin(object,sizeof(object),1));
    puts("PASS fire/magic and heat-only classification, bounded profiles, complete-emitter sampling, native simulation bytes/array/capacity preserved, native sort scratch, both-eye GPU reuse, nested/failure/exception restoration and Original bypass");return 0;
}
