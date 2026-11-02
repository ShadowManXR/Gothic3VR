#include "StereoMath.h"
#include <cstdio>
using namespace DirectX;
XMFLOAT3 Project(XrPosef pose,XrFovf fov,float x,float y,float z)
{
    XMFLOAT3 result;
    XMStoreFloat3(&result,XMVector3TransformCoord(XMVectorSet(x,y,z,1),stereo::View(pose)*stereo::Projection(fov)));
    return result;
}
int main()
{
    unsigned checks=0,failed=0;
    auto check=[&](bool ok,char const* label){++checks;if(!ok)++failed;std::printf("%s %s\n",ok?"PASS":"FAIL",label);};
    auto close=[](float a,float b){return std::fabs(a-b)<.0001f;};
    XrFovf fov{-.75f,.75f,.75f,-.75f};XrPosef left{{0,0,0,1},{-.032f,0,0}},right=left;right.position.x=.032f;
    auto l=Project(left,fov,0,0,-1),r=Project(right,fov,0,0,-1);
    check(l.x>0&&r.x<0&&close(l.x,-r.x),"per-eye view produces opposite horizontal disparities");
    auto far=Project(left,fov,0,0,-4);
    check(std::fabs(l.x)>std::fabs(far.x)*3.9f,"near object has greater geometric stereo disparity");
    auto center=Project({{0,0,0,1},{0,0,0}},fov,0,0,-2);
    check(close(center.x,0)&&close(center.y,0)&&center.z>0&&center.z<1,"OpenXR -Z forward maps to D3D visible depth");
    auto n=Project({{0,0,0,1},{0,0,0}},fov,0,0,-.05f),f=Project({{0,0,0,1},{0,0,0}},fov,0,0,-100);
    check(close(n.z,0)&&close(f.z,1),"right-handed projection maps near/far to D3D 0..1");
    XrFovf asymmetric{-.9f,.6f,.8f,-.5f};
    auto edgeL=Project({{0,0,0,1},{0,0,0}},asymmetric,std::tan(asymmetric.angleLeft),0,-1);
    auto edgeR=Project({{0,0,0,1},{0,0,0}},asymmetric,std::tan(asymmetric.angleRight),0,-1);
    check(close(edgeL.x,-1)&&close(edgeR.x,1),"runtime asymmetric horizontal FOV preserved");
    auto edgeU=Project({{0,0,0,1},{0,0,0}},asymmetric,0,std::tan(asymmetric.angleUp),-1);
    auto edgeD=Project({{0,0,0,1},{0,0,0}},asymmetric,0,std::tan(asymmetric.angleDown),-1);
    check(close(edgeU.y,1)&&close(edgeD.y,-1),"runtime vertical FOV maps without image inversion");
    XrPosef translated{{0,0,0,1},{1,2,3}};
    auto relative=Project(translated,fov,1,2,1);
    check(close(relative.x,0)&&close(relative.y,0),"head translation changes view rather than scene origin");
    XrPosef turned{{0,std::sin(.4f),0,std::cos(.4f)},{0,0,0}};
    auto rot=XMMatrixRotationQuaternion(XMVectorSet(turned.orientation.x,turned.orientation.y,turned.orientation.z,turned.orientation.w));
    XMFLOAT3 point;XMStoreFloat3(&point,XMVector3TransformCoord(XMVectorSet(0,0,-2,1),rot));
    auto looked=Project(turned,fov,point.x,point.y,point.z);
    check(close(looked.x,0)&&close(looked.y,0),"head quaternion is inverted for camera view");
    check(!stereo::ValidViews(3)&&stereo::ValidViews(15),"valid flags alone do not count as tracked stereo views");
    XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};views[0].pose=left;views[1].pose=right;
    XMFLOAT3 origin;XMStoreFloat3(&origin,XMVector3TransformCoord(XMVectorZero(),stereo::SceneAnchor(views)));
    check(close(origin.x,0)&&close(origin.z,0),"scene anchor uses center of eyes and stays independent of IPD");
    std::printf("%u checks, %u failures\n",checks,failed);return failed?1:0;
}
