#include "InteractionFocusRanking.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct Target {uintptr_t id;float lo[3],hi[3];bool pickup=false,visible=true;int type=2;};
int main(){
    WorldAim ray{};ray.origin[1]=100;ray.direction[2]=1;float root[3]{},score=0;
    auto rating=[&](Target target,float& value){return RankedInteractionBox(ray,target.type,root,100,target.lo,target.hi,180,value,target.pickup);};
    Target chest{1,{-20,70,80},{20,120,120}},leftBed{2,{-160,0,20},{-2,120,220}},rightBed{3,{2,0,20},{160,120,220}};
    float chestScore=0,leftScore=0,rightScore=0;CHECK(rating(chest,chestScore)&&rating(leftBed,leftScore)&&rating(rightBed,rightScore));
    CHECK(chestScore<leftScore&&chestScore<rightScore);
    for(auto order:{0,1,2}){InteractionFocusRanking ranking;Target candidates[3]{chest,leftBed,rightBed};
        for(unsigned i=0;i<3;++i){auto t=candidates[(i+order)%3];float v=0;if(rating(t,v)&&t.visible)ranking.Accept(t.id,v);}
        CHECK(ranking.Selected()==chest.id);}
    Target shrine{4,{-70,0,20},{70,220,140}},offering{5,{-4,96,75},{4,104,83},true},neighbor{6,{11,96,75},{19,104,83},true};
    float shrineScore=0,offeringScore=0,neighborScore=0;CHECK(rating(shrine,shrineScore)&&rating(offering,offeringScore)&&rating(neighbor,neighborScore));
    CHECK(offeringScore<shrineScore&&offeringScore<neighborScore);
    CHECK(FinalInteractionFocus(12,true,offering.id)==12); // Native character selection wins unchanged.
    CHECK(FinalInteractionFocus(shrine.id,false,offering.id)==offering.id);
    CHECK(FinalInteractionFocus(12,true,0)==12&&FinalInteractionFocus(0,false,offering.id)==offering.id);
    InteractionFocusRanking ranking;ranking.Accept(shrine.id,shrineScore);ranking.Accept(offering.id,offeringScore);CHECK(ranking.Selected()==offering.id);
    // A native HasLowerPriority/closer-root veto cannot substitute the shrine
    // for the independently ranked, LOS-checked final choice.
    uintptr_t nativeChoice=shrine.id;CHECK(nativeChoice!=ranking.Selected());
    ranking.Reset();offering.visible=false;for(auto t:{shrine,offering}){float v=0;if(rating(t,v)&&t.visible)ranking.Accept(t.id,v);}CHECK(ranking.Selected()==shrine.id);
    // Small pickup forgiveness remains useful on top of a large altar.
    ray.origin[0]=5;CHECK(rating(offering,offeringScore)&&rating(shrine,shrineScore)&&offeringScore<shrineScore);
    ray.origin[0]=0;Target distant{7,{-2,98,300},{2,102,310},true};CHECK(!rating(distant,score));
    Target side{8,{150,96,70},{160,104,80},true};CHECK(!rating(side,score));
    Target broken{9,{10,10,10},{-10,20,20}};CHECK(!rating(broken,score));
    ranking.Reset();CHECK(!ranking.Selected());ranking.Accept(0,0);ranking.Accept(5,-1);CHECK(!ranking.Selected());
    puts("PASS chest between beds, offerings on/near shrine, adjacent small targets, candidate order, preserved LOS/reach and independent final ranking");return 0;
}
