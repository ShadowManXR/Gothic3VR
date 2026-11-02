#pragma once
#include "RadialImage.h"
#include "WorldStatusArtwork.h"
namespace diagnostic {
inline constexpr unsigned StatusWidth=256,StatusHeight=64;
inline constexpr float StatusDistance=1.3f;
struct StatusVisual {unsigned width,height;float x,y,metersWide,metersHigh;};
inline bool StatusIcon(int status){return status==1||status==2||status==4;}
inline StatusVisual WorldStatusVisual(int status,float hudWidth=2.6f,float hudDistance=2.f,float hudVertical=-.3f,float aspect=9.f/16.f){
    auto visual=StatusIcon(status)?StatusVisual{artwork::StatusIconPixels,artwork::StatusIconPixels,0,0,.07f,.07f}:StatusVisual{StatusWidth,StatusHeight,0,0,.26f,.065f};
    // Project the top of the native bottom quickbar to this view-space layer.
    // HUD offset, scale, distance and aspect all retain the same visible gap.
    float quickbarTop=hudVertical-.36f*hudWidth*aspect;
    visual.y=quickbarTop*StatusDistance/hudDistance+.025f+visual.metersHigh*.5f;
    return visual;
}
inline std::vector<uint32_t> WorldStatusImage(int status){
    std::vector<uint32_t> result(RadialPixels*RadialPixels);if(status<1||status>4)return result;
    // Artwork is compiled and already premultiplied. The compositor uploads
    // only on status changes, reusing the existing single shared HUD layer.
    if(auto const* icon=artwork::Status(status)){
        for(unsigned y=0;y<artwork::StatusIconPixels;++y)
            memcpy(result.data()+y*RadialPixels,icon+y*artwork::StatusIconPixels,artwork::StatusIconPixels*sizeof(uint32_t));
        return result;
    }
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=StatusWidth;info.bmiHeader.biHeight=-int(StatusHeight);info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    HDC dc=CreateCompatibleDC(nullptr);void* memory=nullptr;HBITMAP bitmap=dc?CreateDIBSection(dc,&info,DIB_RGB_COLORS,&memory,nullptr,0):nullptr;
    if(!bitmap){if(dc)DeleteDC(dc);throw std::runtime_error("Cannot create VR status image");}
    auto previous=SelectObject(dc,bitmap);auto* pixels=static_cast<uint32_t*>(memory);
    memset(pixels,0,StatusWidth*StatusHeight*sizeof(uint32_t));
    for(unsigned y=0;y<StatusHeight;++y)for(unsigned x=0;x<StatusWidth;++x){bool edge=x<2||x>=StatusWidth-2||y<2||y>=StatusHeight-2;pixels[y*StatusWidth+x]=edge?0x997c43:0x261d14;}
    if(status==3){HFONT font=CreateFontW(-20,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Georgia");
        if(font){auto old=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(232,213,170));RECT area{0,0,StatusWidth,StatusHeight};DrawTextW(dc,L"NO SNEAK SKILL",-1,&area,DT_CENTER|DT_VCENTER|DT_SINGLELINE);SelectObject(dc,old);DeleteObject(font);}}
    GdiFlush();for(unsigned y=0;y<StatusHeight;++y)for(unsigned x=0;x<StatusWidth;++x){auto color=pixels[y*StatusWidth+x];unsigned r=(color>>16)&255,g=(color>>8)&255,b=color&255;result[y*RadialPixels+x]=0xef000000|((r*239/255)<<16)|((g*239/255)<<8)|(b*239/255);}
    SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(dc);return result;
}
}
