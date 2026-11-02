#include "FrameCopyBatch.h"
#include "FencePolling.h"
#include <cstdio>
#include <stdexcept>
#include <vector>
#define CHECK(x) do{if(!(x)){std::printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    g3vr::FrameCopyBatch<unsigned> batch;unsigned waits=0;std::vector<unsigned> released;
    auto wait=[&]{++waits;};auto release=[&](unsigned h){released.push_back(h);};
    batch.Drain(wait,release);CHECK(!waits&&released.empty());
    CHECK(!batch.Add(0)&&batch.Add(11)&&batch.Add(12)&&batch.Add(13)&&batch.Add(14));
    CHECK(!batch.Add(11)&&!batch.Add(15)&&batch.Count()==4);
    batch.Drain(wait,release);CHECK(waits==1&&released==std::vector<unsigned>({11,12,13,14})&&batch.Count()==0);
    batch.Drain(wait,release);CHECK(waits==1); // empty/cancel after successful submit
    CHECK(batch.Add(21)&&batch.Add(22));
    bool failed=false;try{batch.Drain([&]{throw std::runtime_error("GPU timeout");},release);}catch(...){failed=true;}
    CHECK(failed&&batch.Count()==2&&released.size()==4);
    // Cancellation retries the fence, never hands an unfinished destination
    // to the compositor, and returns each acquired image exactly once.
    batch.Drain(wait,release);CHECK(waits==2&&batch.Count()==0&&released.back()==22);
    CHECK(batch.Add(31)&&batch.Add(32)&&batch.Add(33));failed=false;
    try{batch.Drain(wait,[&](unsigned h){if(h==32)throw std::runtime_error("release failure");release(h);});}catch(...){failed=true;}
    CHECK(failed&&waits==3&&batch.Count()==2&&released.back()==31&&!batch.Add(34));
    batch.Drain(wait,release);CHECK(waits==3&&batch.Count()==0&&released.back()==33);
    // No additional frame is queued. An independent source for each layer
    // remains untouched until the shared completion fence, including a frame
    // cancelled after its first eye and a menu-only frame.
    for(unsigned layers: {1u,2u,3u,4u}){
        bool sourcesSafe=false;unsigned expected=0;
        for(unsigned i=1;i<=layers;++i)CHECK(batch.Add(i));
        batch.Drain([&]{sourcesSafe=true;},[&](unsigned h){if(!sourcesSafe||h!=++expected)throw std::runtime_error("ownership order");});
        CHECK(sourcesSafe&&expected==layers&&batch.Count()==0);
    }
    unsigned legacyBarriers=0,groupedBarriers=0;
    for(unsigned i=1;i<=4;++i){CHECK(batch.Add(i));batch.Drain([&]{++legacyBarriers;},[](unsigned){});}
    for(unsigned i=1;i<=4;++i)CHECK(batch.Add(i));batch.Drain([&]{++groupedBarriers;},[](unsigned){});
    CHECK(legacyBarriers==4&&groupedBarriers==1);
    g3vr::DeferredFrameCopies<unsigned> textures;unsigned src[3]{11,22,33},dst[3]{};unsigned writesWaited=0;
    CHECK(!textures.Add(nullptr,src)&&textures.Add(dst,src)&&!textures.Add(dst,src+1));
    CHECK(textures.Add(dst+1,src+1)&&textures.Add(dst+2,src+2)&&!textures.Add(dst,src));
    CHECK(dst[0]==0&&dst[1]==0&&textures.Count()==3);
    textures.Drain([&]{++writesWaited;src[0]=44;},[&](unsigned* d,unsigned* s){if(!writesWaited)throw std::runtime_error("D3D9 incomplete");*d=*s;});
    CHECK(writesWaited==1&&dst[0]==44&&dst[1]==22&&dst[2]==33&&textures.Count()==0);
    textures.Drain([&]{++writesWaited;},[](unsigned*,unsigned*){});CHECK(writesWaited==1);
    CHECK(textures.Add(dst,src));failed=false;
    try{textures.Drain([]{throw std::runtime_error("D3D9 timeout");},[](unsigned*,unsigned*){});}catch(...){failed=true;}
    CHECK(failed&&textures.Count()==1);textures.Drain([]{},[](unsigned* d,unsigned* s){*d=*s;});CHECK(!textures.Count());
    g3vr::FencePollSchedule polls;unsigned flushes=0;
    for(unsigned i=0;i<1000;++i)flushes+=polls.Flush(500);
    CHECK(flushes==1&&!polls.Flush(501)&&polls.Flush(502)&&!polls.Flush(502)&&polls.Flush(504));
    CHECK(polls.Flush(1)); // tolerate a restarted synthetic clock
    std::puts("PASS stereo/UI/radial ownership; one same-frame completion fence; partial-frame cancellation; timeout/release retry; bounded non-flushing polls");return 0;
}
