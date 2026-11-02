#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
namespace diagnostic {
// Locator markers share the native HUD texture, which OpenXR presents on a
// view-space quad. Project into that quad's actual dimensions, using the
// rendered eye-center view rather than the restored desktop camera/lens.
class LocatorProjection {
    float matrix[16]{};uint64_t updated=0;
    static void Product(float const* a,float const* b,float* out){
        for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){out[r*4+c]=0;for(unsigned k=0;k<4;++k)out[r*4+c]+=a[r*4+k]*b[k*4+c];}
    }
public:
    void Reset(){updated=0;}
    bool Publish(float const (&left)[16],float const (&right)[16],unsigned width,unsigned height,float metersWide,float distance,float vertical,uint64_t now){
        Reset();if(!now||width<32||height<32||width>16384||height>16384||!std::isfinite(metersWide)||!std::isfinite(distance)||!std::isfinite(vertical)||metersWide<=0||distance<=0)return false;
        float center[16]{};for(unsigned i=0;i<16;++i){if(!std::isfinite(left[i])||!std::isfinite(right[i]))return false;center[i]=(left[i]+right[i])*.5f;}
        // Averaging eye translations cancels IPD. Only matching eye rotations
        // may share this view; unknown/canted layouts keep the native matrix.
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)if(std::fabs(left[r*4+c]-right[r*4+c])>.002f)return false;
        float metersHigh=metersWide*float(height)/float(width),projection[16]{},viewport[16]{},vp[16]{};
        projection[0]=2*distance/metersWide;projection[5]=2*distance/metersHigh;
        projection[9]=-2*vertical/metersHigh;projection[10]=projection[11]=1;
        viewport[0]=float(width)*.5f;viewport[5]=-float(height)*.5f;viewport[10]=viewport[15]=1;
        viewport[12]=float(width)*.5f;viewport[13]=float(height)*.5f;
        Product(center,projection,vp);Product(vp,viewport,matrix);
        for(auto value:matrix)if(!std::isfinite(value))return false;updated=now;return true;
    }
    bool Read(uint64_t now,float (&out)[16])const{
        if(!updated||now<updated||now-updated>150)return false;std::copy_n(matrix,16,out);return true;
    }
};
}
