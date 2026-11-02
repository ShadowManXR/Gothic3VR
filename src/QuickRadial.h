#pragma once
#include <cmath>
#include <cstdint>
namespace diagnostic {
struct RadialState {bool visible=false;int selected=-1,page=0;};
inline int RadialCount(int page){return page?8:10;}
struct RadialResult {RadialState state;int choice=-1;bool capture=false;};
inline int RadialSector(float x,float y,int count) {
    if(!std::isfinite(x)||!std::isfinite(y)||std::hypot(x,y)<.45f||count<1)return -1;
    constexpr float tau=6.28318530718f;float angle=std::atan2(x,y);if(angle<0)angle+=tau;
    return int(std::floor(angle*float(count)/tau+.5f))%count;
}
class QuickRadial {
    RadialState state;bool armed=false,previousLeft=false,previousPage=false,previousConfirm=false,previousCancel=false;uint64_t previous=0;
public:
    RadialState State() const{return state;}
    void Cancel(){state={};armed=false;previousLeft=previousPage=previousConfirm=previousCancel=false;previous=0;}
    RadialResult Step(bool allowed,bool leftClick,bool rightClick,bool pageButton,float x,float y,uint64_t time,bool confirm=false,bool cancel=false) {
        RadialResult out;
        if(!allowed||(leftClick&&rightClick)){Cancel();out.capture=allowed&&leftClick;return out;}
        if(previous&&time<previous)Cancel();previous=time;
        bool openPressed=leftClick&&!previousLeft,confirmPressed=confirm&&!previousConfirm,cancelPressed=cancel&&!previousCancel,pagePressed=pageButton&&!previousPage;
        previousLeft=leftClick;previousConfirm=confirm;previousCancel=cancel;previousPage=pageButton;
        if(!armed){if(!leftClick&&!confirm&&!cancel)armed=true;out.capture=leftClick;return out;}
        if(!state.visible){if(!openPressed)return out;state.visible=true;state.page=0;state.selected=-1;confirmPressed=cancelPressed=pagePressed=false;}
        else if(cancelPressed||openPressed){out.capture=true;state={};armed=false;return out;}
        out.capture=true;
        // Closing needs a fresh A press. Keep selection when the stick springs
        // back to center, allowing the user's thumb to reach the A button.
        if(pagePressed){state.page=1-state.page;state.selected=-1;}
        int count=RadialCount(state.page),selected=RadialSector(x,y,count);
        // Hold the current sector across small movements near a boundary.
        if(selected>=0&&state.selected>=0&&selected!=state.selected) {
            constexpr float tau=6.28318530718f;float angle=std::atan2(x,y),center=float(state.selected)*tau/float(count);
            if(std::fabs(std::remainder(angle-center,tau))<tau/float(count)*.5f+.065f)selected=state.selected;
        }
        if(selected>=0)state.selected=selected;
        if(confirmPressed&&state.selected>=0){out.choice=state.page*10+state.selected;state={};armed=false;return out;}
        out.state=state;return out;
    }
};
}
