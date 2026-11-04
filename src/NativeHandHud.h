#pragma once
#include "D3D9HandHud.h"
#include "NativeRadialItems.h"
#include "VrSettings.h"

namespace diagnostic {
// Independent geometry and values; never move/reparent the game's GUI windows.
class NativeHandHud {
    using Texture=Microsoft::WRL::ComPtr<IDirect3DTexture9>;
    Texture digits,icons[10];RadialItems items,uploaded;
    IDirect3DDevice9* owner=nullptr;
    float vitals[3]{};bool valid=false;uint64_t retry=0;
    CFFGFCBitmap slotBitmap,statusBitmap;
    static bool Artwork(char const* name,ReticleArtwork& art){
        art.Reset();auto& images=eCImageAdmin::GetInstance();int group=-1,index=-1;
        if(!images.FindImageIndex(bCString(name),group,index))return false;
        NativeReticleImageInfo info;if(!images.GetImageInfo(group,index,reinterpret_cast<eSImage&>(info)))return false;
        auto* node=images.GetTexture(info.texture);if(!Readable(node,sizeof(void*)))return false;
        eCGfxImageData* data=nullptr;memcpy(&data,node,sizeof(data));if(!Readable(data,sizeof(*data)))return false;
        auto* api=static_cast<eCDX9Texture*>(data->m_pAPITexture);if(!Readable(api,sizeof(*api))||!api->m_pTexture)return false;
        D3DSURFACE_DESC desc{};if(FAILED(api->m_pTexture->GetLevelDesc(0,&desc)))return false;
        int x=info.rect.GetLeft(),y=info.rect.GetTop(),r=info.rect.GetRight(),b=info.rect.GetBottom();
        if(x<0||y<0||r<=x||b<=y||unsigned(r)>desc.Width||unsigned(b)>desc.Height)return false;
        art.texture=api->m_pTexture;art.device=api->m_pDirect3DDevice;
        art.u0=(x+.5f)/desc.Width;art.v0=(y+.5f)/desc.Height;art.u1=(r-.5f)/desc.Width;art.v1=(b-.5f)/desc.Height;return true;
    }
    static Texture Upload(IDirect3DDevice9* device,uint32_t const* pixels,unsigned width,unsigned height){
        Texture texture;Microsoft::WRL::ComPtr<IDirect3DDevice9Ex> extended;
        bool ex=SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&extended)));
        if(FAILED(device->CreateTexture(width,height,1,ex?D3DUSAGE_DYNAMIC:0,D3DFMT_A8R8G8B8,ex?D3DPOOL_DEFAULT:D3DPOOL_MANAGED,&texture,nullptr)))return {};
        D3DLOCKED_RECT lock{};if(FAILED(texture->LockRect(0,&lock,nullptr,0)))return {};
        for(unsigned y=0;y<height;++y)memcpy(static_cast<char*>(lock.pBits)+y*lock.Pitch,pixels+y*width,width*4);
        texture->UnlockRect(0);return texture;
    }
    void PrepareDigits(IDirect3DDevice9* device){
        if(digits)return;
        HDC dc=CreateCompatibleDC(nullptr);if(!dc)return;
        BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=256;info.bmiHeader.biHeight=-32;
        info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void* raw=nullptr;
        HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&raw,nullptr,0);
        if(!bitmap){DeleteDC(dc);return;}auto old=SelectObject(dc,bitmap);
        memset(raw,0,256*32*4);HFONT font=CreateFontW(-24,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Arial");
        auto oldFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,255,255));
        for(unsigned i=0;i<10;++i){wchar_t c=wchar_t(L'0'+i);TextOutW(dc,i*24,0,&c,1);}GdiFlush();
        auto* pixels=static_cast<uint32_t*>(raw);for(unsigned i=0;i<256*32;++i){unsigned a=pixels[i]&255;pixels[i]=(a<<24)|0xffffff;}
        digits=Upload(device,pixels,256,32);SelectObject(dc,oldFont);DeleteObject(font);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
    }
public:
    // Ex has no managed pool. Do not retain default-pool resources across the
    // game's reset boundary; both eyes still share the uploads within a frame.
    void EndFrame(){if(!owner)return;Microsoft::WRL::ComPtr<IDirect3DDevice9Ex> ex;
        if(SUCCEEDED(owner->QueryInterface(IID_PPV_ARGS(&ex)))){digits.Reset();for(auto& icon:icons)icon.Reset();uploaded={};owner=nullptr;}}
    void Reset(){valid=false;items={};uploaded={};digits.Reset();for(auto& icon:icons)icon.Reset();owner=nullptr;}
    void Update(eCEntity* player,bool enabled,RadialItems const& snapshot){
        valid=false;if(!enabled||!Readable(player,sizeof(eCEntity))){Reset();return;}
        if(!player->GetPropertySet(eEPropertySetType_PlayerMemory))return;
        Entity hero(player);auto const& memory=hero.PlayerMemory;
        auto fraction=[](int value,int maximum){return maximum>0?std::clamp(float(value)/maximum,0.f,1.f):0.f;};
        vitals[0]=fraction(memory.GetHitPoints(),memory.GetHitPointsMax());
        vitals[1]=fraction(memory.GetStaminaPoints(),memory.GetStaminaPointsMax());
        vitals[2]=fraction(memory.GetManaPoints(),memory.GetManaPointsMax());items=snapshot;valid=true;
    }
    void Draw(IDirect3DDevice9* device,float const* view,float const* projection,
              arms::Mat const (&poses)[2],bool const (&available)[2],float units,int const* scalePercent){
        if(!valid||!device||(!available[0]&&!available[1]))return;
        if(FAILED(device->TestCooperativeLevel())){Reset();return;}
        if(owner!=device){digits.Reset();for(auto& icon:icons)icon.Reset();uploaded={};owner=device;}
        ReticleArtwork bar,status;
        bool barReady=Artwork("g3_hud_view_quickslots_back.tga",bar),statusReady=Artwork("g3_hud_statusbar_back.tga",status);
        auto now=GetTickCount64();if((!barReady||!statusReady)&&now>=retry){retry=now+2000;
            if(!barReady)slotBitmap.Create(bCString("\\data\\gui\\icons\\hud\\v3\\backgrounds\\views\\g3_hud_view_quickslots_back.tga"));
            if(!statusReady)statusBitmap.Create(bCString("\\data\\gui\\icons\\hud\\v3\\backgrounds\\views\\g3_hud_statusbar_back.tga"));
            barReady=Artwork("g3_hud_view_quickslots_back.tga",bar);statusReady=Artwork("g3_hud_statusbar_back.tga",status);}
        PrepareDigits(device);std::vector<HandHudQuad> quads;quads.reserve(128);
        auto add=[&](unsigned h,float x,float y,float w,float height,IDirect3DTexture9* texture,DWORD color=0xffffffff,float u0=0,float v0=0,float u1=1,float v1=1){
            float pw=h?201.f:520.f,ph=h?59.f:56.f,width=(h?.19f:.38f)*units*scalePercent[h]*.01f;
            AddHandHudQuad(quads,poses[h],width,width*ph/pw,pw,ph,x,y,w,height,texture,color,u0,v0,u1,v1);};
        auto art=[&](unsigned h,float y,float w,float height,ReticleArtwork const& a){add(h,0,y,w,height,a.texture.Get(),0xffffffff,a.u0,a.v0,a.u1,a.v1);};
        auto number=[&](unsigned n,float x,float y,float size){char text[16];sprintf_s(text,"%u",n);
            for(char const* c=text;*c;++c,x+=size*.7f){unsigned digit=unsigned(*c-'0');add(0,x,y,size,size*1.3f,digits.Get(),0xffffffff,digit*24.f/256,0,(digit*24.f+20)/256,32.f/32);}};
        if(available[0]&&barReady&&bar.device==device){art(0,0,520,56,bar);
            for(unsigned i=0;i<10;++i){if(uploaded[i].icon!=items[i].icon||(!icons[i]&&items[i].icon)){
                    icons[i]=items[i].icon?Upload(device,items[i].icon->data(),64,64):Texture{};uploaded[i]=items[i];}
                float x=22+i*48.f;if(icons[i])add(0,x+7,12,32,32,icons[i].Get());
                if(digits){number(RadialHotkey(i),x+2,6,10);if(items[i].amount>1){unsigned n=unsigned(items[i].amount);unsigned count=unsigned(std::to_string(n).size());number(n,x+44-count*7.f,36,10);}}}}
        if(available[1]&&statusReady&&status.device==device){constexpr DWORD colors[]{4288616467u,4291274312u,4279005335u};
            for(unsigned i=0;i<3;++i){art(1,i*20.f,201,19,status);if(vitals[i]>0)add(1,12,i*20.f+4,176*vitals[i],12,nullptr,colors[i]);}}
        DrawHandHudQuads(device,view,projection,quads);
    }
};
}
