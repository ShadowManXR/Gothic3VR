#pragma once
#include "TemporalNativeGeometry.h"
namespace diagnostic::temporal {
struct EntityIdentity {std::array<uint32_t,5> value{};bool Valid()const noexcept{return (value[0]|value[1]|value[2]|value[3])!=0;}bool operator==(EntityIdentity const& b)const noexcept{return value==b.value;}};
struct InstanceEntity {EntityIdentity id;std::array<float,16> world{};};
struct InstanceRows {
    std::array<EntityIdentity,17> ids;
    std::array<uint8_t,17> identified{};
};
// Native ExecuteInstance sorts its 20-byte entity records before filling 17
// model matrices. Match the CURRENT matrices to copied IDs/world transforms;
// array order and native pointer addresses are not temporal identity.
inline InstanceRows IdentifyInstances(NativeGeometryPose const& pose,InstanceEntity const* entities,unsigned count)noexcept{
    InstanceRows result;if((count&&!entities)||count>1024||!pose.Valid())return result;
    for(unsigned row=0;row<17;++row){bool ambiguous=false;EntityIdentity match;
        for(unsigned i=0;i<count;++i){auto const& e=entities[i];if(!e.id.Valid()||e.world[3]!=0||e.world[7]!=0||e.world[11]!=0||e.world[15]!=1)continue;bool equal=true;
            for(unsigned r=0;r<3&&equal;++r)for(unsigned c=0;c<4;++c){float value=e.world[c*4+r];if(!std::isfinite(value)||value!=pose.constants[(row*3+r)*4+c]){equal=false;break;}}
            if(!equal)continue;if(match.Valid()&&!(match==e.id)){ambiguous=true;break;}match=e.id;
        }
        if(match.Valid()&&!ambiguous){result.ids[row]=match;result.identified[row]=1;}
    }
    // A repeated entity in two rows cannot prove which previous copy is the
    // corresponding geometry. Conservatively invalidate those rows.
    std::array<bool,17> duplicate{};for(unsigned i=0;i<17;++i)for(unsigned j=i+1;j<17;++j)if(result.identified[i]&&result.identified[j]&&result.ids[i]==result.ids[j])duplicate[i]=duplicate[j]=true;
    for(unsigned i=0;i<17;++i)if(duplicate[i])result.identified[i]=0;
    return result;
}
struct GeometryVersion {
    uint64_t vertexObject=0,vertexContent=0,indexObject=0,indexContent=0;
    bool valid=false;
    bool Same(GeometryVersion const& b)const noexcept{return valid&&b.valid&&vertexObject&&indexObject&&vertexObject==b.vertexObject&&vertexContent==b.vertexContent&&indexObject==b.indexObject&&indexContent==b.indexContent;}
};
struct InstanceReprojection {NativeGeometryPose previous;std::array<uint8_t,17> valid{};};
struct GeometryFrameStamp {uint64_t frame=0,epoch=0,displayTimeNs=0;unsigned eye=2;
    bool Follows(GeometryFrameStamp const& before)const noexcept{return eye<2&&eye==before.eye&&frame&&before.frame&&before.frame!=UINT64_MAX&&frame==before.frame+1&&epoch&&epoch==before.epoch&&displayTimeNs>before.displayTimeNs&&before.displayTimeNs&&displayTimeNs-before.displayTimeNs<=250000000ull;}
};
inline InstanceReprojection RebaseInstances(NativeGeometryPose const& now,InstanceRows const& current,NativeGeometryPose const& before,InstanceRows const& old,GeometryVersion const& currentGeometry,GeometryVersion const& previousGeometry,GeometryFrameStamp const& currentStamp,GeometryFrameStamp const& previousStamp)noexcept{
    InstanceReprojection result;result.previous=now;
    if(!currentStamp.Follows(previousStamp)||!currentGeometry.Same(previousGeometry)||!now.Valid()||!before.Valid())return result;
    // Previous camera projection must belong to the old eye frame, while
    // model rows are rebased to CURRENT vertex instance indices.
    std::copy_n(before.constants.begin()+68*4,16,result.previous.constants.begin()+68*4);
    for(unsigned i=0;i<17;++i){if(!current.identified[i])continue;int found=-1;
        bool duplicate=false;for(unsigned k=0;k<17;++k)if(k!=i&&current.identified[k]&&current.ids[k]==current.ids[i])duplicate=true;if(duplicate)continue;
        for(unsigned j=0;j<17;++j)if(old.identified[j]&&current.ids[i]==old.ids[j]){if(found>=0){found=-2;break;}found=int(j);}
        if(found<0)continue;double distance=0;for(unsigned c=0;c<3;++c){double delta=now.constants[(i*3+c)*4+3]-before.constants[(unsigned(found)*3+c)*4+3];distance+=delta*delta;}
        if(distance>1000.*1000.)continue;
        std::copy_n(before.constants.begin()+unsigned(found)*12,12,result.previous.constants.begin()+i*12);result.valid[i]=1;
    }
    return result;
}
}
