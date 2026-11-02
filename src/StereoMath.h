#pragma once
#include <DirectXMath.h>
#include <openxr/openxr.h>
#include <cmath>
namespace stereo
{
inline DirectX::XMMATRIX View(XrPosef const& pose)
{
    using namespace DirectX;
    auto q=XMVectorSet(pose.orientation.x,pose.orientation.y,pose.orientation.z,pose.orientation.w);
    auto world=XMMatrixRotationQuaternion(q)*XMMatrixTranslation(pose.position.x,pose.position.y,pose.position.z);
    return XMMatrixInverse(nullptr,world);
}
inline DirectX::XMMATRIX Projection(XrFovf const& fov,float nearZ=.05f,float farZ=100.f)
{
    return DirectX::XMMatrixPerspectiveOffCenterRH(std::tan(fov.angleLeft)*nearZ,std::tan(fov.angleRight)*nearZ,
        std::tan(fov.angleDown)*nearZ,std::tan(fov.angleUp)*nearZ,nearZ,farZ);
}
inline bool ValidViews(XrViewStateFlags flags)
{
    constexpr XrViewStateFlags needed=XR_VIEW_STATE_ORIENTATION_VALID_BIT|XR_VIEW_STATE_POSITION_VALID_BIT|
        XR_VIEW_STATE_ORIENTATION_TRACKED_BIT|XR_VIEW_STATE_POSITION_TRACKED_BIT;
    return (flags&needed)==needed;
}
inline DirectX::XMMATRIX SceneAnchor(XrView const (&views)[2])
{
    using namespace DirectX;
    auto const& q=views[0].pose.orientation;
    auto forward=XMVector3Rotate(XMVectorSet(0,0,-1,0),XMVectorSet(q.x,q.y,q.z,q.w));
    float yaw=std::atan2(-XMVectorGetX(forward),-XMVectorGetZ(forward));
    return XMMatrixRotationY(yaw)*XMMatrixTranslation(
        (views[0].pose.position.x+views[1].pose.position.x)*.5f,
        (views[0].pose.position.y+views[1].pose.position.y)*.5f,
        (views[0].pose.position.z+views[1].pose.position.z)*.5f);
}
}
