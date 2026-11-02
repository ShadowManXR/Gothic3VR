#include "RangedFocusLabel.h"
#include <limits>
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    CHECK(ScaleLabelHeight(40,100)==40&&ScaleLabelHeight(40,50)==20&&ScaleLabelHeight(-40,25)==-10);
    CHECK(ScaleLabelHeight(40,150)==60&&ScaleLabelHeight(1,25)==1&&ScaleLabelHeight(0,50)==0);
    CHECK(HalfLabelHeight(40)==20&&HalfLabelHeight(-40)==-20&&HalfLabelHeight(1)==1&&HalfLabelHeight(0)==0);
    CHECK(HalfLabelHeight(std::numeric_limits<int>::min())==0);
    float origin[3]{0,140,0},lo[3]{-20,0,3000},hi[3]{20,180,3040};RangedFocusLabel labels;
    labels.Capture(123,10,1000,true,true,origin,lo,hi,100,120);
    CHECK(labels.Applies(123,10,1000,true));CHECK(!labels.Applies(456,10,1000,true)&&!labels.Applies(123,9,1000,true)&&!labels.Applies(123,10,1000,false));
    // A sheathing animation clears focus before the native HUD finishes its
    // name fade. Keep only that name small, without refreshing its deadline.
    labels.InteractionFocus(0);
    CHECK(labels.Applies(123,11,1100,true));
    CHECK(labels.Applies(123,12,1000+RangedFocusLabel::FadeLimitMs,true));
    labels.InteractionFocus(0);
    CHECK(!labels.Applies(123,13,1001+RangedFocusLabel::FadeLimitMs,true));
    CHECK(!labels.Applies(123,11,999,true)); // backwards clock cannot revive stale context
    labels.Capture(123,14,4000,true,true,origin,lo,hi,100,120);
    CHECK(!labels.Applies(456,15,4050,true)); // different character never inherits the fade style
    labels.InteractionFocus(123);CHECK(!labels.Applies(123,15,4050,true)); // same NPC reacquired as a close interaction
    labels.Capture(123,16,4100,true,true,origin,lo,hi,100,120);
    labels.InteractionFocus(456);CHECK(!labels.Applies(123,17,4150,true)); // another close target clears the old name
    labels.Capture(123,18,4200,false,true,origin,lo,hi,100,120);CHECK(!labels.Applies(123,18,4200,true)); // normal melee/interaction/magic path
    labels.Capture(123,19,4300,true,false,origin,lo,hi,100,120);CHECK(!labels.Applies(123,19,4300,true)); // items/signposts unaffected
    lo[2]=120;hi[2]=170;labels.Capture(123,20,4400,true,true,origin,lo,hi,100,120);CHECK(!labels.Applies(123,20,4400,true)); // nearby NPC unchanged with bow drawn
    lo[2]=200;hi[2]=240;labels.Capture(123,21,4500,true,true,origin,lo,hi,100,120);CHECK(!labels.Applies(123,21,4500,true)); // exact boundary
    lo[2]=201;labels.Capture(123,22,4600,true,true,origin,lo,hi,100,120);CHECK(labels.Applies(123,22,4600,true));
    labels.Capture(123,23,4700,true,true,origin,lo,hi,100,300);CHECK(!labels.Applies(123,23,4700,true)); // preserve longer close-interaction preset
    lo[0]=std::numeric_limits<float>::quiet_NaN();labels.Capture(123,24,4800,true,true,origin,lo,hi,100,120);CHECK(!labels.Applies(123,24,4800,true));
    puts("Ranged labels: stable native sheathing fade; nearby/world/menu labels preserved");return 0;
}
