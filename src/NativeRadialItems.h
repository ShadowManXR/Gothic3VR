#pragma once
#include "RadialItems.h"
#include "NativeReticle.h"
#include "RadialTextureCopy.h"
#include <map>
#include <string>

namespace diagnostic {
struct RadialInventoryAccess : gCInventoryStack {
    static void IconNames(gCInventoryStack const& stack,bCString& active,bCString& inactive){
        auto method=&RadialInventoryAccess::GetIconFileNames;
        (stack.*method)(active,inactive);
    }
};
// Copies the installed game's own icon atlas into a small CPU snapshot. The
// engine already loads d3dx9_40; its converter handles compressed atlases.
// No game artwork, stack pointers or D3D resources are kept by the XR layer.
inline std::shared_ptr<RadialIcon const> CopyRadialIcon(bCString const& name){
    auto& images=eCImageAdmin::GetInstance();int group=-1,index=-1;
    if(!images.FindImageIndex(name,group,index))return {};
    NativeReticleImageInfo info;
    if(!images.GetImageInfo(group,index,reinterpret_cast<eSImage&>(info)))return {};
    auto* node=images.GetTexture(info.texture);if(!Readable(node,sizeof(void*)))return {};
    eCGfxImageData* data=nullptr;memcpy(&data,node,sizeof(data));
    if(!Readable(data,sizeof(*data)))return {};
    auto* api=static_cast<eCDX9Texture*>(data->m_pAPITexture);
    if(!Readable(api,sizeof(*api))||!api->m_pTexture||!api->m_pDirect3DDevice)return {};
    D3DSURFACE_DESC desc{};if(FAILED(api->m_pTexture->GetLevelDesc(0,&desc)))return {};
    RECT source{info.rect.GetLeft(),info.rect.GetTop(),info.rect.GetRight(),info.rect.GetBottom()};
    if(source.left<0||source.top<0||source.right<=source.left||source.bottom<=source.top||
       unsigned(source.right)>desc.Width||unsigned(source.bottom)>desc.Height)return {};
    return CopyRadialTexture(api->m_pTexture,source);
}
class NativeRadialItems {
    struct Cached {std::shared_ptr<RadialIcon const> icon;uint64_t retry=0;};
    std::map<std::string,Cached> cache;
    uintptr_t owner=0;uint64_t nextRead=0;bool wasVisible=false;
    RadialItems items;
public:
    void Reset(){owner=0;nextRead=0;wasVisible=false;items={};cache.clear();}
    RadialItems const& Update(eCEntity* player,bool visible,uint64_t now){
        if(!Readable(player,sizeof(eCEntity))){Reset();return items;}
        if(owner!=reinterpret_cast<uintptr_t>(player)){Reset();owner=reinterpret_cast<uintptr_t>(player);}
        bool opening=visible&&!wasVisible;wasVisible=visible;
        if(!visible||(!opening&&now<nextRead))return items;
        nextRead=now+250;
        auto* inventory=static_cast<gCInventory_PS*>(player->GetPropertySet(eEPropertySetType_Inventory));
        items={};if(!Readable(inventory,sizeof(*inventory)))return items;
        // Bound memory even after many equipment/quick-slot changes.
        if(cache.size()>64)cache.clear();
        for(unsigned sector=0;sector<items.size();++sector){
            int index=inventory->FindStackIndexByHotKey(RadialHotkey(sector));
            if(index<0||index>=inventory->GetStackCount())continue;
            auto* stack=inventory->GetStack(index);if(!Readable(stack,sizeof(*stack))||stack->GetAmount()<=0)continue;
            auto& item=items[sector];item.amount=stack->GetAmount();
            // GetIcon performs the same lazy resource loading as the quick bar.
            stack->GetIcon();bCString active,inactive;RadialInventoryAccess::IconNames(*stack,active,inactive);
            auto const& name=stack->GetLearned()||stack->GetActivationCount()>0?active:inactive;
            if(!name.GetText()||!*name.GetText())continue;
            auto& entry=cache[std::string(name.GetText())];
            if(!entry.icon&&now>=entry.retry){entry.retry=now+2000;entry.icon=CopyRadialIcon(name);}
            item.icon=entry.icon;
        }
        return items;
    }
};
}
