#include "ParticleAttachment.h"
#include <atomic>
#include <thread>
#include <cstdio>
#include <limits>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    visual::ParticleWorldScope draw,other;
    arms::Mat identity,delta,out;arms::Position(delta,{12,24,36});
    CHECK(!visual::ParticleWorldScope::Current());
    auto invalid=delta;invalid.m[0]=std::numeric_limits<float>::quiet_NaN();
    CHECK(!draw.Begin(invalid)&&!visual::ParticleWorldScope::Current());
    CHECK(draw.Begin(delta)&&visual::ParticleWorldScope::Current()==&draw);
    CHECK(!draw.Begin(delta)&&!other.Begin(delta));
    other.End();CHECK(visual::ParticleWorldScope::Current()==&draw);
    // A worker must forward native draws, even during a main-thread held
    // emitter, and may use its own independent scope without touching ours.
    std::atomic<bool> workerOk{true};
    std::thread worker([&]{
        for(unsigned i=0;i<100000;++i)if(visual::ParticleWorldScope::Current())workerOk=false;
        visual::ParticleWorldScope own;arms::Mat changed;
        if(!own.Begin(delta)||visual::ParticleWorldScope::Current()!=&own||!own.Rewrite(identity,changed))workerOk=false;
        own.End();if(visual::ParticleWorldScope::Current())workerOk=false;
    });worker.join();CHECK(workerOk&&visual::ParticleWorldScope::Current()==&draw);
    // Non-identity transforms and the engine's subsequent restoration pass
    // preserve native data. Only one identity transform per eye is corrected.
    auto translated=identity;arms::Position(translated,{1,0,0});
    CHECK(!draw.Rewrite(translated,out)&&draw.Rewrite(identity,out));
    CHECK(arms::Length(arms::Position(out)-arms::Position(delta))<.001f);
    CHECK(!draw.Rewrite(identity,out));draw.End();
    CHECK(!visual::ParticleWorldScope::Current()&&!draw.Rewrite(identity,out));
    for(unsigned eye=0;eye<2;++eye){
        CHECK(draw.Begin(delta)&&draw.Rewrite(identity,out));
        CHECK(!draw.Rewrite(identity,out));draw.End();
        CHECK(!visual::ParticleWorldScope::Current());
    }
    puts("PASS held-effect thread isolation, refused nesting, native restoration and both eyes");return 0;
}
