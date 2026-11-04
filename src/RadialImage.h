#pragma once
#include <windows.h>
#include "QuickRadial.h"
#include "RadialItems.h"
#include <vector>
#include <array>
#include <stdexcept>
namespace diagnostic {
inline constexpr unsigned RadialPixels=768;
inline std::array<std::vector<uint8_t>,2> const& RadialGeometry() {
    // Sector/radius math is invariant. Build once while connecting VR rather
    // than evaluating trigonometry for every pixel on each controller event.
    static auto const masks=[] {
        std::array<std::vector<uint8_t>,2> result;
        for(unsigned page=0;page<2;++page){auto& mask=result[page];mask.resize(RadialPixels*RadialPixels);
            for(unsigned y=0;y<RadialPixels;++y)for(unsigned x=0;x<RadialPixels;++x){float dx=float(x)-384,dy=384-float(y);int r2=int(dx*dx+dy*dy);uint8_t value=0;
                if(r2<151*151)value=1;
                if(r2>=169*169&&r2<333*333){value=uint8_t(2+RadialSector(dx,dy,RadialCount(page)));
                    float angle=std::atan2(dx,dy),step=6.28318530718f/float(RadialCount(page));if(std::fabs(std::remainder(angle-step*.5f,step))<.012f)value=1;}
                if((r2>=147*147&&r2<151*151)||(r2>=328*328&&r2<333*333)||(r2>=338*338&&r2<340*340))value=254;
                mask[y*RadialPixels+x]=value;}}
        return result;
    }();
    return masks;
}
inline wchar_t const* RadialLabel(int page,int index) {
    static wchar_t const* const slots[]={L"1",L"2",L"3",L"4",L"5",L"6",L"7",L"8",L"9",L"0"};
    static wchar_t const* const actions[]={L"Inventory",L"Character",L"Journal",L"Map",L"Magic",L"Main menu",L"Quicksave",L"Back"};
    return page?actions[index%8]:slots[index%10];
}
inline std::vector<uint32_t> RadialImage(RadialState const& state,RadialItems const& items={}) {
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=RadialPixels;info.bmiHeader.biHeight=-int(RadialPixels);info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    void* memory=nullptr;HDC dc=CreateCompatibleDC(nullptr);HBITMAP bitmap=dc?CreateDIBSection(dc,&info,DIB_RGB_COLORS,&memory,nullptr,0):nullptr;
    if(!bitmap){if(dc)DeleteDC(dc);throw std::runtime_error("Cannot create VR radial image");}
    HGDIOBJ oldBitmap=SelectObject(dc,bitmap);auto* pixels=static_cast<uint32_t*>(memory);
    auto const& geometry=RadialGeometry()[state.page?1:0];
    for(unsigned i=0;i<geometry.size();++i){auto value=geometry[i];if(!value){pixels[i]=0;continue;}
        unsigned grain=((i*1664525u+1013904223u)^(i>>5))%9;bool selected=int(value)-2==state.selected;
        unsigned r=value==254?151:value==1?30:selected?105:49;
        unsigned g=value==254?120:value==1?23:selected?74:37;
        unsigned b=value==254?65:value==1?17:selected?34:25;
        pixels[i]=((r+grain)<<16)|((g+grain)<<8)|(b+grain/2);
    }
    auto text=[&](wchar_t const* value,int cx,int cy,int fontSize,int width){HFONT font=CreateFontW(-fontSize,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Georgia");
        if(font){HGDIOBJ oldFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(232,213,170));RECT area{cx-width/2,cy-fontSize,cx+width/2,cy+fontSize};DrawTextW(dc,value,-1,&area,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);SelectObject(dc,oldFont);DeleteObject(font);}};
    int count=RadialCount(state.page);for(int i=0;i<count;++i){
        float angle=float(i)*6.28318530718f/float(count);int cx=384+int(std::sin(angle)*250),cy=384-int(std::cos(angle)*250);
        if(state.page||items[i].amount<=0){text(RadialLabel(state.page,i),cx,cy,state.page?24:42,150);continue;}
        if(items[i].icon){
            GdiFlush(); // GDI batches text; finish it before direct pixel blending.
            auto const& icon=*items[i].icon;
            for(unsigned y=0;y<RadialIconPixels;++y)for(unsigned x=0;x<RadialIconPixels;++x){
                auto source=icon[y*RadialIconPixels+x];unsigned a=source>>24;
                auto& dest=pixels[(cy-int(RadialIconPixels)/2+int(y))*RadialPixels+cx-int(RadialIconPixels)/2+int(x)];
                unsigned r=(((source>>16)&255)*a+((dest>>16)&255)*(255-a)+127)/255;
                unsigned g=(((source>>8)&255)*a+((dest>>8)&255)*(255-a)+127)/255;
                unsigned b=((source&255)*a+(dest&255)*(255-a)+127)/255;
                dest=(r<<16)|(g<<8)|b;
            }
        }else text(RadialLabel(0,i),cx,cy,32,100);
        text(RadialLabel(0,i),cx,cy-48,22,90);
        wchar_t amount[24];swprintf_s(amount,L"%d",items[i].amount);text(amount,cx,cy+48,22,140);
    }
    text(state.page?L"Actions":L"Quick slots",384,331,25,285);
    text(state.selected<0?L"Select with thumbstick":RadialLabel(state.page,state.selected),384,378,24,285);
    text(L"A: select · X: cancel",384,426,19,290);text(L"B: change page",384,456,19,285);
    GdiFlush();std::vector<uint32_t> result(RadialPixels*RadialPixels);
    for(unsigned i=0;i<result.size();++i){uint32_t color=pixels[i]&0xffffff;if(color){unsigned b=color&255,g=(color>>8)&255,r=(color>>16)&255;result[i]=0xef000000|((r*239/255)<<16)|((g*239/255)<<8)|(b*239/255);}}
    SelectObject(dc,oldBitmap);DeleteObject(bitmap);DeleteDC(dc);return result;
}
}
