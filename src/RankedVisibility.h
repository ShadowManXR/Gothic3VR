#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
namespace diagnostic {
// Call-local storage only. Candidate identities never survive a native query.
// Flush bounded batches by rating so a visible better candidate avoids LOS
// calls for every worse one. Equal ratings keep native enumeration order.
template<class Candidate,std::size_t Capacity=32> class RankedVisibility {
    static_assert(Capacity>0);
    struct Entry {Candidate candidate{};unsigned order=0;};
    std::array<Entry,Capacity> entries{};std::size_t count=0;
public:
    bool Full()const{return count==Capacity;}
    bool Add(Candidate const& candidate){
        if(Full()||!std::isfinite(candidate.rating))return false;
        entries[count]={candidate,unsigned(count)};++count;return true;
    }
    template<class Visible> bool Drain(float& best,Candidate& selected,Visible&& visible){
        std::sort(entries.begin(),entries.begin()+count,[](Entry const& a,Entry const& b){
            return a.candidate.rating<b.candidate.rating||(a.candidate.rating==b.candidate.rating&&a.order<b.order);});
        bool changed=false;
        for(std::size_t i=0;i<count;++i){auto const& candidate=entries[i].candidate;
            if(candidate.rating>=best)break;
            if(visible(candidate)){best=candidate.rating;selected=candidate;changed=true;break;}
        }
        count=0;return changed;
    }
};
}
