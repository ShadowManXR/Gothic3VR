#include "StereoShadowView.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(value) do{if(!(value)){std::printf("FAIL line %d\n",__LINE__);return 1;}}while(0)

int main(){
    const float nearZ=13,farZ=10000;
    float fovs[2][4]{{-.95f,.65f,.8f,-.75f},{-.65f,.95f,.8f,-.75f}};
    // Rotate the head while the native body camera keeps facing forward.
    // Every eye corner must remain inside the shadow selection volume.
    for(float yaw:{0.f,.8f,1.57f,3.14f}){
        float angles[3]{.2f,yaw,.1f};
        auto center=arms::Rotation(NativeCameraRotation(angles));
        arms::Position(center,{1000,170,-300});
        arms::Mat eyes[2];
        for(unsigned eye=0;eye<2;++eye){
            float cant[3]{0,eye?.05f:-.05f,0};
            auto local=arms::Rotation(NativeCameraRotation(cant));
            arms::Position(local,{eye?3.2f:-3.2f,0,0});
            eyes[eye]=arms::Multiply(local,center);
        }
        float projection[16]{};
        CHECK(StereoShadowProjection(center,eyes,fovs,nearZ,farZ,projection));
        arms::Mat inverse;
        CHECK(arms::Inverse(center,inverse));
        for(unsigned eye=0;eye<2;++eye)for(float depth:{nearZ,farZ}){
            for(unsigned side=0;side<2;++side)for(unsigned height=2;height<4;++height){
                auto world=arms::Point(eyes[eye],{std::tan(fovs[eye][side])*depth,
                    std::tan(fovs[eye][height])*depth,depth});
                auto point=arms::Point(inverse,world);
                CHECK(std::fabs(point.x*projection[0]/point.z)<=1.0001f);
                CHECK(std::fabs(point.y*projection[5]/point.z)<=1.0001f);
            }
        }
        auto invalid=eyes[0];invalid.m[0]=NAN;
        arms::Mat broken[2]{invalid,eyes[1]};
        CHECK(!StereoShadowProjection(center,broken,fovs,nearZ,farZ,projection));
        CHECK(!StereoShadowProjection(center,eyes,fovs,0,farZ,projection));
    }
    std::puts("PASS both-eye shadow coverage under head rotation, asymmetric lenses and canted eyes");
    return 0;
}
