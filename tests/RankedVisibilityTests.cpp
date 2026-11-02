#include "RankedVisibility.h"
#include <cstdio>
#include <vector>
#include <limits>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct Candidate {unsigned token=0;float rating=0;bool visible=false;};
int main(){
    unsigned random=4127;auto next=[&]{random=random*1664525u+1013904223u;return random;};
    unsigned oldCalls=0,newCalls=0;
    for(unsigned trial=0;trial<10000;++trial){
        std::vector<Candidate> all;unsigned count=next()%160;
        for(unsigned i=0;i<count;++i)all.push_back({i+1,float(next()%40)*.05f,(next()%5)!=0});
        float oldBest=2,newBest=2;unsigned oldToken=0;Candidate chosen;
        for(auto const& c:all)if(c.rating<oldBest){++oldCalls;if(c.visible){oldBest=c.rating;oldToken=c.token;}}
        RankedVisibility<Candidate> ranked;
        auto visible=[&](Candidate const& c){++newCalls;return c.visible;};
        for(auto const& c:all){if(ranked.Full())ranked.Drain(newBest,chosen,visible);CHECK(ranked.Add(c));}
        ranked.Drain(newBest,chosen,visible);
        CHECK(oldToken==chosen.token&&oldBest==newBest);
    }
    CHECK(newCalls<=oldCalls);
    // Worst native enumeration order: each successive visible target improves
    // the score. A full batch needs one visibility query instead of32.
    RankedVisibility<Candidate> descending;float best=2;Candidate chosen;unsigned calls=0;
    for(unsigned i=0;i<32;++i)CHECK(descending.Add({i+1,1.8f-.05f*i,true}));
    CHECK(descending.Full()&&!descending.Add({99,0,true}));
    CHECK(descending.Drain(best,chosen,[&](Candidate const&){++calls;return true;})&&calls==1&&chosen.token==32);
    CHECK(!descending.Add({1,std::numeric_limits<float>::quiet_NaN(),true}));
    // Blocked best score falls through; ties keep original identity.
    RankedVisibility<Candidate> blocked;best=2;chosen={};
    blocked.Add({1,.1f,false});blocked.Add({2,.2f,true});blocked.Add({3,.2f,true});
    blocked.Drain(best,chosen,[](Candidate const& c){return c.visible;});CHECK(chosen.token==2);
    printf("PASS10000 native-order equivalence replays, bounded overflow batches, blocked/tie cases; LOS callbacks %u -> %u (synthetic workload, not game FPS)\n",oldCalls,newCalls);
    return 0;
}
