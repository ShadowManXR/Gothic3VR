#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
namespace diagnostic::draw_cost {
enum class Kind : unsigned {HighRender,LowRender,OtherRender,HighExecute,LowExecute,OtherExecute,VegetationHigh,VegetationLow,VegetationOther,Count};
inline constexpr unsigned Count=unsigned(Kind::Count),MaxScopes=48,MaxDepth=48;
inline constexpr char const* Names[]={"high_render","low_render","other_render","high_execute","low_execute","other_execute","vegetation_high","vegetation_low","vegetation_other"};
inline Kind ExecuteKind(Kind render)noexcept{return render==Kind::HighRender?Kind::HighExecute:render==Kind::LowRender?Kind::LowExecute:Kind::OtherExecute;}
inline Kind VegetationKind(Kind render)noexcept{return render==Kind::HighRender?Kind::VegetationHigh:render==Kind::LowRender?Kind::VegetationLow:Kind::VegetationOther;}
struct Span {unsigned kind=0,lane=0;int parent=-1;uint64_t begin=0,end=0;};
// Validate proper tree nesting before subtracting children. GPU timestamps
// with missing/reversed/overlapping scopes are unavailable, never zero cost.
inline bool Exclusive(Span const* spans,unsigned count,uint64_t* exclusive)noexcept{
 if(!spans||!exclusive||!count||count>MaxScopes)return false;
 std::array<uint64_t,MaxScopes> children{};
 for(unsigned i=0;i<count;++i){auto const& s=spans[i];
  if(s.kind>=Count||s.lane<1||s.lane>2||s.end<s.begin||s.parent>=int(i)||s.parent< -1)return false;
  if(s.parent>=0){auto const& p=spans[unsigned(s.parent)];if(s.lane!=p.lane||s.begin<p.begin||s.end>p.end)return false;
   uint64_t duration=s.end-s.begin;if(children[unsigned(s.parent)]>UINT64_MAX-duration)return false;children[unsigned(s.parent)]+=duration;}
  for(unsigned j=0;j<i;++j)if(spans[j].parent==s.parent&&spans[j].lane==s.lane&&s.begin<spans[j].end)return false;
 }
 for(unsigned i=0;i<count;++i){auto total=spans[i].end-spans[i].begin;if(children[i]>total)return false;exclusive[i]=total-children[i];}
 return true;
}
}
