#pragma once
#include <d3d9.h>
namespace g3vr {
class DxvkFoveation {
    void* state=nullptr;
    char const* reason="FFR disabled";
public:
    DxvkFoveation()=default;~DxvkFoveation();
    DxvkFoveation(DxvkFoveation const&)=delete;DxvkFoveation& operator=(DxvkFoveation const&)=delete;
    bool Initialize(IDirect3DDevice9Ex* native);
    void Configure(unsigned width,unsigned height,int mode,int diameter,float leftX=.5f,float leftY=.5f,float rightX=.5f,float rightY=.5f);
    bool Ready()const noexcept{return state!=nullptr;}
    char const* Error()const noexcept{return reason;}
    void BeginEye(unsigned eye);
    void EndEye();
    void Reset() noexcept;
};
}
