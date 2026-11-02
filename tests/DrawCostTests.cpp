#include "DrawCostCore.h"
#include <iostream>
using namespace diagnostic::draw_cost;
#define CHECK(x) do{if(!(x)){std::cerr<<"failed "<<__LINE__<<'\n';return 1;}}while(false)
int main(){
 Span spans[]={{3,1,-1,10,110},{6,1,0,20,30},{4,1,0,40,90},{7,1,2,50,60},{3,2,-1,120,220},{4,2,4,140,180}};
 uint64_t result[MaxScopes]{};CHECK(Exclusive(spans,6,result));
 CHECK(result[0]==40&&result[1]==10&&result[2]==40&&result[3]==10&&result[4]==60&&result[5]==40);
 auto save=spans[1];spans[1].parent=1;CHECK(!Exclusive(spans,6,result));spans[1]=save;
 spans[1].end=120;CHECK(!Exclusive(spans,6,result));spans[1]=save;
 spans[1].end=41;CHECK(!Exclusive(spans,6,result));spans[1]=save;
 spans[1].lane=2;CHECK(!Exclusive(spans,6,result));spans[1]=save;
 spans[1].begin=31;CHECK(!Exclusive(spans,6,result));spans[1]=save;
 CHECK(!Exclusive(spans,0,result));CHECK(!Exclusive(spans,MaxScopes+1,result));
 CHECK(ExecuteKind(Kind::LowRender)==Kind::LowExecute);CHECK(VegetationKind(Kind::HighRender)==Kind::VegetationHigh);
 std::cout<<"PASS nested exclusive CPU/GPU attribution rules, both eyes, invalid bounds/overlap/parent/lane rejection\n";
}
