#include "TemporalBufferVersions.h"
#include <cstdio>
#include <limits>
#include <memory>
using namespace diagnostic::temporal;
#define CHECK(x) do{if(!(x)){std::printf("FAIL line%d %s\n",__LINE__,#x);return 1;}}while(0)
InstanceEntity Entity(unsigned id,float x){InstanceEntity e;e.id.value={id,2,3,4,5};for(unsigned c=0;c<4;++c)e.world[c*4+c]=1;e.world[12]=x;return e;}
void Matrix(NativeGeometryPose& p,unsigned row,InstanceEntity const& e){for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)p.constants[(row*3+r)*4+c]=e.world[c*4+r];}
int main(){ std::array<uint8_t,17> empty{};
    auto versions=std::make_unique<BufferVersions>();CHECK(versions->Observe(100,1024,false)&&versions->Observe(200,256,true));auto original=versions->Geometry(100,200);CHECK(original.valid&&versions->Geometry(100,200).Same(original));
    versions->Lock(100,false,true);CHECK(!versions->Geometry(100,200).valid);versions->Unlock(100,true);CHECK(versions->Geometry(100,200).Same(original));
    versions->Lock(100,true,false);CHECK(versions->Geometry(100,200).Same(original));versions->Lock(100,true,true);CHECK(!versions->Geometry(100,200).valid);versions->Unlock(100,true);auto changed=versions->Geometry(100,200);CHECK(changed.valid&&!changed.Same(original));
    versions->Released(100,1);CHECK(versions->Geometry(100,200).Same(changed));versions->Released(100,0);CHECK(!versions->Geometry(100,200).valid);CHECK(versions->Observe(100,1024,false));CHECK(!versions->Geometry(100,200).Same(changed));
    versions->Created(100,1024,false);auto fresh=versions->Geometry(100,200);CHECK(fresh.valid);versions->Lock(200,true,true);versions->Unlock(200,false);CHECK(!versions->Geometry(100,200).valid);versions->Created(200,256,true);CHECK(versions->Geometry(100,200).valid);
    auto beforeReset=versions->Geometry(100,200);versions->Reset();CHECK(versions->Observe(100,1024,false)&&versions->Observe(200,256,true));CHECK(!versions->Geometry(100,200).Same(beforeReset));
    versions->Lock(100,false,true);versions->Lock(100,true,true);versions->Unlock(100,true);CHECK(!versions->Geometry(100,200).valid);versions->Created(100,1024,false);
    versions->Observe(300,1024,false);CHECK(!versions->Geometry(100,300).valid);CHECK(!versions->Observe(0,10,false));
    auto a=Entity(10,10),b=Entity(20,20),c=Entity(30,30);InstanceEntity entities[]{c,a,b};NativeGeometryPose old,now;Matrix(old,0,a);Matrix(old,1,b);Matrix(old,2,c);Matrix(now,0,c);Matrix(now,1,a);Matrix(now,2,b);
    for(unsigned j=0;j<4;++j){old.constants[(68+j)*4+j]=1;now.constants[(68+j)*4+j]=1;}old.constants[68*4+3]=-.25f;
    auto oldRows=IdentifyInstances(old,entities,3),currentRows=IdentifyInstances(now,entities,3);CHECK(oldRows.identified[0]&&oldRows.ids[0]==a.id&&currentRows.ids[0]==c.id);
    GeometryFrameStamp first{100,7,1000000000,0},second{101,7,1033333333,0};auto geometry=versions->Geometry(100,200);
    auto result=RebaseInstances(now,currentRows,old,oldRows,geometry,geometry,second,first);CHECK(result.valid[0]&&result.valid[1]&&result.valid[2]);CHECK(result.previous.constants[3]==30&&result.previous.constants[12+3]==10&&result.previous.constants[24+3]==20&&result.previous.constants[68*4+3]==-.25f);
    auto wrongEye=second;wrongEye.eye=1;CHECK(RebaseInstances(now,currentRows,old,oldRows,geometry,geometry,wrongEye,first).valid==empty);auto gap=second;gap.displayTimeNs+=1000000000;CHECK(RebaseInstances(now,currentRows,old,oldRows,geometry,geometry,gap,first).valid==empty);gap=second;gap.epoch++;CHECK(RebaseInstances(now,currentRows,old,oldRows,geometry,geometry,gap,first).valid==empty);
    auto altered=geometry;altered.vertexContent++;CHECK(RebaseInstances(now,currentRows,old,oldRows,altered,geometry,second,first).valid==empty);
    auto missing=currentRows;missing.ids[0]=Entity(99,30).id;CHECK(!RebaseInstances(now,missing,old,oldRows,geometry,geometry,second,first).valid[0]);
    InstanceEntity duplicates[]{a,Entity(99,10)};CHECK(!IdentifyInstances(old,duplicates,2).identified[0]);NativeGeometryPose repeated=old;Matrix(repeated,1,a);Matrix(repeated,2,a);auto repeatedRows=IdentifyInstances(repeated,&a,1);CHECK(!repeatedRows.identified[0]&&!repeatedRows.identified[1]&&!repeatedRows.identified[2]);
    auto teleported=now;teleported.constants[3]+=1001;CHECK(!RebaseInstances(teleported,currentRows,old,oldRows,geometry,geometry,second,first).valid[0]);
    auto nan=entities[0];nan.world[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!IdentifyInstances(now,&nan,1).identified[0]);CHECK(!IdentifyInstances(now,nullptr,3).identified[0]);
    versions->Reset();for(unsigned i=0;i<2048;++i)CHECK(versions->Observe(100000+i*16,128,false));CHECK(!versions->Observe(999999,128,false)&&versions->overflow==1);CHECK(versions->Observe(100000,128,false));
    std::puts("PASS instance ID rebasing, reordered native matrices, ambiguous/missing IDs, same-eye continuity, teleports, buffer writes/read locks, reuse/release/reset/poison invalidation. Native hook installation remains caller responsibility.");return 0;
}
