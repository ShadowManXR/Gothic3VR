#include "NativeSceneAdapter.h"
#include "NativeMaterialCapture.h"
#include <windows.h>
#include <SharedBase.h>
#include <Engine/animation/ge_rendercontext.h>
#include <Engine/entity/ge_entity.h>
#include <Engine/ge_visualmeshstatic_ps.h>
#include <Engine/ge_resourceshadermaterial_ps.h>
#include <Engine/ge_vertexbufferpoolitemmain.h>
#include <Engine/ge_vertexbufferpoolitemlightmap.h>
#include <Engine/ge_lightmap.h>
#include <cstring>
#include <cmath>
namespace g3vr::dx11 {
namespace {
bool Readable(void const* p,size_t bytes)noexcept{
    uintptr_t first=reinterpret_cast<uintptr_t>(p),last=first+bytes;
    if(!first||last<first)return false;
    while(first<last){MEMORY_BASIC_INFORMATION m{};if(!VirtualQuery(reinterpret_cast<void*>(first),&m,sizeof(m))||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
        if(!(m.Protect&(PAGE_READONLY|PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return false;
        auto next=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;if(next<=first)return false;first=next;}
    return true;
}
bool CopyMaterialNameUnchecked(eCResourceShaderMaterial_PS* material,char (&output)[1024]){
    if(!Readable(material,sizeof(*material)))return false;
    auto name=material->GetNativeFileName();int n=name.GetLength();auto* text=name.GetText();
    if(n<=0||n>=sizeof(output)||!Readable(text,size_t(n)+1)||text[n])return false;
    std::memcpy(output,text,size_t(n)+1);return true;
}
}
bool __cdecl CopyNativeMaterialName(eCRenderOp2 const& operation,char (&name)[1024])noexcept{
    name[0]=0;__try{return CopyMaterialNameUnchecked(operation.GetMaterial(),name);}__except(EXCEPTION_EXECUTE_HANDLER){name[0]=0;return false;}
}
bool __cdecl CopyNativePoolMaterialName(eCVertexbufferPoolItemMain* pool,char (&name)[1024])noexcept{
    name[0]=0;__try{return pool&&Readable(pool,sizeof(void*))&&CopyMaterialNameUnchecked(pool->GetMaterial(),name);}__except(EXCEPTION_EXECUTE_HANDLER){name[0]=0;return false;}
}
namespace {
bool __cdecl CopyAdditional(void* source,void* output,unsigned vertices,unsigned bytes)noexcept{
    __try{
        if(!source||!output||!vertices||vertices>100000||bytes!=vertices*24||!Readable(source,sizeof(void*)))return false;
        auto* item=static_cast<eCVertexbufferPoolItemLightmap*>(source);
        auto* lightmap=item->GetLightmap();
        if(!Readable(lightmap,sizeof(void*))||unsigned(lightmap->GetLightmapType())!=1||
            lightmap->GetVertexCount()!=int(vertices)||!lightmap->GetLightmapTexture()||lightmap->IsDirty()||
            lightmap->GetBitmaps().GetCount()==0||lightmap->GetPerVertexColors().GetCount()!=int(vertices)||
            lightmap->GetPerVertexIncident().GetCount()!=int(vertices))return false;
        // CP1.75.14 FillVertices1d3520 compares argument2 with both per-vertex
        // array COUNTS. It chooses stride24 internally for texture lightmaps;
        // argument2 is not the byte stride. Sentinels detect an unwritten field.
        std::memset(output,0xff,bytes);
        lightmap->FillVertices(static_cast<char*>(output),int(vertices));
        // Color occupies the first DWORD. All other components are float32.
        for(unsigned i=0;i<vertices;++i)for(unsigned j=1;j<6;++j){float f;std::memcpy(&f,static_cast<char*>(output)+i*24+j*4,4);if(!std::isfinite(f))return false;}
        return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
NativeLightmapSource __cdecl NativePoolLightmapSource(eCVertexbufferPoolItemMain* pool,std::array<uint32_t,5> const& wanted)noexcept{
    __try{
        // CP1.75.14 AddInstance RVA1db250 stores {entity,+4 lightmap item,
        // +8 frustum info,+12 fade,+16 switch}. ExecuteInstance1dabf0 reads
        // the same20byte records and may sort them AFTER this call.
        if(!Readable(pool,0x74))return {};
        unsigned count;unsigned char* rows=nullptr;
        std::memcpy(&rows,reinterpret_cast<char*>(pool)+0x6c,4);
        std::memcpy(&count,reinterpret_cast<char*>(pool)+0x70,4);
        if(!count||count>1024||!Readable(rows,count*20))return {};
        void* result=nullptr;
        for(unsigned i=0;i<count;++i){eCEntity* entity=nullptr;std::memcpy(&entity,rows+i*20,4);
            if(!Readable(entity,sizeof(void*)))continue;auto const& id=entity->GetID();
            if(!id.IsValid()||std::memcmp(&id.GetCore(),wanted.data(),16)||id.GetCount()!=wanted[4])continue;
            if(result)return {};std::memcpy(&result,rows+i*20+4,4);
            if(!Readable(result,sizeof(void*)))return {};
        }
        return result?NativeLightmapSource{result,&CopyAdditional}:NativeLightmapSource{};
    }__except(EXCEPTION_EXECUTE_HANDLER){return {};}
}
bool __cdecl CopyNativeStatic(eCRenderContext* context,NativeStaticSnapshot& output)noexcept{
    output={};
    __try {
        if(!Readable(context,sizeof(*context))||!context->m_pEntity||!context->m_pEntityPropertySet)return false;
        auto* entity=context->m_pEntity;
        if(!Readable(entity,sizeof(void*))||entity->GetEntityFlags().Dynamic)return false;
        // A static visual can still have a complex/alpha/wind material; this
        // establishes snapshot type only, never material replacement permission.
        auto* visual=dynamic_cast<eCVisualMeshStatic_PS*>(context->m_pEntityPropertySet);
        if(!visual||!visual->IsResourceLoaded())return false;
        auto const& id=entity->GetID();if(!id.IsValid())return false;
        auto const& name=visual->GetResourceFileName();int length=name.GetLength();auto* text=name.GetText();
        if(length<=0||length>=sizeof(output.resource)||!Readable(text,size_t(length)+1)||text[length])return false;
        std::memcpy(output.identity.data(),&id.GetCore(),16);output.identity[4]=id.GetCount();
        static_assert(sizeof(bCMatrix)==sizeof(float)*16);std::memcpy(output.worldMeters.data(),&entity->GetWorldMatrix(),sizeof(bCMatrix));
        for(float f:output.worldMeters)if(!std::isfinite(f))return false;
        // Native mesh data and translation are centimeters. The linear 3x3
        // remains dimensionless after the importer converts vertices to meters.
        if(output.worldMeters[3]!=0||output.worldMeters[7]!=0||output.worldMeters[11]!=0||output.worldMeters[15]!=1)return false;
        for(unsigned i=12;i<15;++i)output.worldMeters[i]*=.01f;
        std::memcpy(output.resource,text,size_t(length));output.resource[length]=0;output.materialSwitch=visual->GetMaterialSwitch();return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){output={};return false;}
}
}
