#pragma once
#include <g3sdk/Script.h>
#include <array>
#include "RangedFocusLabel.h"
namespace diagnostic {
// Native theme owns cached font siblings. Never mutate a shared font, and
// never change inventory/dialogue fonts. Four bounded slots cover near/far
// labels with distinct source fonts, without per-label font construction.
class NativeRangedLabelFont {
    struct Cached {unsigned source=~0u;int font=-1,percent=0;void const* sourceNode=nullptr;void const* node=nullptr;};
    std::array<Cached,4> cache{};unsigned next=0;
public:
    int Resolve(unsigned font,int percent=50){
        auto& theme=eCThemeAdmin::GetInstance();
        if(font>4096||percent<1||percent>200||!theme.IsValidFontIndex(int(font)))return -1;
        for(auto const& c:cache)if(c.source==font&&c.percent==percent&&c.font>=0&&theme.IsValidFontIndex(c.font)&&
            theme.GetFontNode(int(font))==c.sourceNode&&theme.GetFontNode(c.font)==c.node)return c.font;
        eCGfxShared::eSFontDesc desc{};
        if(!theme.GetFontInfo(int(font),desc))return -1;
        int height=ScaleLabelHeight(desc.Height,percent);if(!height)return -1;
        desc.Height=height;if(desc.Width)desc.Width=std::max(1u,(desc.Width*unsigned(percent)+50)/100);
        int index=-1;
        if(!theme.FindFontIndex(desc,index)&&!theme.CreateFontA(desc,index))return -1;
        if(index<0||!theme.IsValidFontIndex(index))return -1;
        cache[next++%cache.size()]={font,index,percent,theme.GetFontNode(int(font)),theme.GetFontNode(index)};return index;
    }
};
}
