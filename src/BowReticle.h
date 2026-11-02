#pragma once
#include <g3sdk/Script.h>
#include <d3d9.h>
#include "PhysicalBow.h"
#include "WorldReticle.h"
#include "D3D9Reticle.h"
namespace diagnostic {
class BowReticle {
    arms::Vec point;bool active=false,target=false;NativeReticle artwork;
    static bool Owned(eCEntity* entity,eCEntity* player,eCEntity* weapon){
        for(unsigned i=0;i<8&&Readable(entity,sizeof(eCEntity));++i){
            if(entity==player||entity==weapon)return true;
            auto* interaction=static_cast<gCInteraction_PS*>(entity->GetPropertySet(eEPropertySetType_Interaction));
            if(Readable(interaction,sizeof(*interaction))&&interaction->GetOwner().GetEntity()==player)return true;
            entity=entity->GetParent();
        }return false;
    }
public:
    bool fault=false;uint64_t frames=0;float lastU[2]{},lastV[2]{};bool visible[2]{};
    void Reset(){active=false;visible[0]=visible[1]=false;artwork.ResetFrame();}
    bool Active()const{return active&&!fault;}
    void Prepare(bow::Sample const& sample,bool allowed){
        Reset();arms::Vec origin,direction;float separation;
        if(fault||!allowed||!sample.valid||GetTickCount64()<sample.time||GetTickCount64()-sample.time>100||!bow::Aim(sample,origin,direction,separation))return;
        auto* player=reinterpret_cast<eCEntity*>(sample.player);auto* weapon=reinterpret_cast<eCEntity*>(sample.weapon);
        if(!Readable(player,sizeof(eCEntity))||!Readable(weapon,sizeof(eCEntity)))return;
        point=origin+direction*(10*sample.units);target=false;
        auto& physics=eCPhysicsScene::GetInstance();
        constexpr unsigned hints=eETraceRayHints_TestStaticShapes|eETraceRayHints_TestDynamicShapes|eETraceRayHints_Impact|eETraceRayHints_Distance|eETraceRayHints_UserData;
        if(physics.TraceRayAllHits(bCVector(origin.x,origin.y,origin.z),bCVector(direction.x*50*sample.units,direction.y*50*sample.units,direction.z*50*sample.units),player,hints)){
            auto const& query=physics.GetQueryReport();int n=query.m_arrHitEntity.GetCount();
            if(n<0||n>4096||query.m_arrDistance.GetCount()!=n||query.m_arrWorldImpact.GetCount()!=n)return;
            float nearest=50*sample.units;
            for(int i=0;i<n;++i){auto* entity=query.m_arrHitEntity[i];float distance=query.m_arrDistance[i];
                if(!std::isfinite(distance)||distance<.05f*sample.units||distance>nearest||Owned(entity,player,weapon))continue;
                auto const& hit=query.m_arrWorldImpact[i];arms::Vec p{hit.m_fX,hit.m_fY,hit.m_fZ};if(!arms::Finite(p))continue;
                nearest=distance;point=p;target=Readable(entity,sizeof(eCEntity))&&entity->HasPropertySet(eEPropertySetType_NPC);
            }
        }
        artwork.Prepare();active=true;++frames;
    }
    // Render onto each eye's scene, not the fixed desktop HUD plane. Full D3D
    // state is restored before the game's next draw and no ray/laser is drawn.
    void Draw(IDirect3DDevice9* device,eCCameraBase const& camera,unsigned eye,float const* unjitteredProjection=nullptr){
        if(eye>1||!Active()||!device)return;
        float view[16],projection[16],u,v;
        for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col){view[row*4+col]=camera.GetViewMatrix()(row,col);projection[row*4+col]=camera.GetProjectionMatrix()(row,col);}
        if(unjitteredProjection)std::copy_n(unjitteredProjection,16,projection);
        if(!ProjectReticle(point,view,projection,u,v)){if(eye==1)artwork.ResetFrame();return;}
        visible[eye]=artwork.art.texture?DrawReticleArtwork(device,u,v,artwork.art):DrawReticle(device,u,v,target);lastU[eye]=u;lastV[eye]=v;
        if(eye==1)artwork.ResetFrame();
    }
};
}
