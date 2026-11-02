#pragma once
#include <stdint.h>
#include <unknwn.h>
// Private versioned COM ABI for the Gothic3VR DXVK 3.1.1 fork. No STL or owned
// memory crosses DLL boundaries. VkImageView is a uint64 handle on Win32.
struct G3VRFfrCaps {uint32_t size,version,supported,tileWidth,tileHeight,paletteSize;};
struct G3VRFfrState {uint32_t size,version;uint64_t view;uint32_t width,height;};
enum {G3VR_FFR_ABI=1};
MIDL_INTERFACE("2cd7bd6c-4302-4af8-9d2b-17be43bca028")
IG3VRFoveation : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetCaps(G3VRFfrCaps* caps)=0;
    virtual HRESULT STDMETHODCALLTYPE SetState(G3VRFfrState const* state)=0;
};
#if defined(__MINGW32__) && !defined(_MSC_VER)
__CRT_UUID_DECL(IG3VRFoveation,0x2cd7bd6c,0x4302,0x4af8,0x9d,0x2b,0x17,0xbe,0x43,0xbc,0xa0,0x28);
#endif
