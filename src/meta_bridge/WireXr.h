#pragma once
#include "Protocol.h"
#include <openxr/openxr.h>
#include <tuple>
namespace diagnostic::meta_bridge {
template<class...T>void Put(Writer& w,T const&...v){(w.Pod(v),...);}
template<class...T>void Take(Reader& r,T&...v){((v=r.Pod<T>()),...);}
inline uint32_t BoundedCount(uint32_t n,uint32_t limit=256){if(n>limit)throw std::runtime_error("Meta bridge array too large");return n;}
template<size_t N>void TakeText(Reader& r,char (&out)[N]){auto s=r.Text();if(s.size()>=N)throw std::runtime_error("Meta bridge text does not fit");memcpy(out,s.c_str(),s.size()+1);}
inline void Write(Writer& w,XrPosef const& v){static_assert(sizeof(v)==28);w.Pod(v);}
inline void Read(Reader& r,XrPosef& v){v=r.Pod<XrPosef>();}
inline void Write(Writer& w,XrFovf const& v){static_assert(sizeof(v)==16);w.Pod(v);}
inline void Read(Reader& r,XrFovf& v){v=r.Pod<XrFovf>();}
inline void Write(Writer& w,XrSwapchainSubImage const& v){Put(w,HandleBits(v.swapchain),v.imageRect.offset.x,v.imageRect.offset.y,v.imageRect.extent.width,v.imageRect.extent.height,v.imageArrayIndex);}
inline void Read(Reader& r,XrSwapchainSubImage& v){v.swapchain=FromHandle<XrSwapchain>(r.Pod<uint64_t>());Take(r,v.imageRect.offset.x,v.imageRect.offset.y,v.imageRect.extent.width,v.imageRect.extent.height,v.imageArrayIndex);}
inline void Write(Writer& w,XrView const& v){Write(w,v.pose);Write(w,v.fov);}
inline void Read(Reader& r,XrView& v){Read(r,v.pose);Read(r,v.fov);}
inline void Write(Writer& w,XrViewConfigurationView const& v){Put(w,v.recommendedImageRectWidth,v.maxImageRectWidth,v.recommendedImageRectHeight,v.maxImageRectHeight,v.recommendedSwapchainSampleCount,v.maxSwapchainSampleCount);}
inline void Read(Reader& r,XrViewConfigurationView& v){Take(r,v.recommendedImageRectWidth,v.maxImageRectWidth,v.recommendedImageRectHeight,v.maxImageRectHeight,v.recommendedSwapchainSampleCount,v.maxSwapchainSampleCount);}
inline void Write(Writer& w,XrSwapchainCreateInfo const& v){Put(w,v.createFlags,v.usageFlags,v.format,v.sampleCount,v.width,v.height,v.faceCount,v.arraySize,v.mipCount);}
inline void Read(Reader& r,XrSwapchainCreateInfo& v){Take(r,v.createFlags,v.usageFlags,v.format,v.sampleCount,v.width,v.height,v.faceCount,v.arraySize,v.mipCount);}
inline void Write(Writer& w,XrFrameState const& v){Put(w,v.predictedDisplayTime,v.predictedDisplayPeriod,v.shouldRender);}
inline void Read(Reader& r,XrFrameState& v){Take(r,v.predictedDisplayTime,v.predictedDisplayPeriod,v.shouldRender);}
inline void Write(Writer& w,XrSpaceLocation const& v){w.Pod(v.locationFlags);Write(w,v.pose);}
inline void Read(Reader& r,XrSpaceLocation& v){v.locationFlags=r.Pod<XrSpaceLocationFlags>();Read(r,v.pose);}
inline void Write(Writer& w,XrActionStatePose const& v){w.Pod(v.isActive);}
inline void Read(Reader& r,XrActionStatePose& v){v.isActive=r.Pod<XrBool32>();}
inline void Write(Writer& w,XrActionStateBoolean const& v){Put(w,v.currentState,v.changedSinceLastSync,v.lastChangeTime,v.isActive);}
inline void Read(Reader& r,XrActionStateBoolean& v){Take(r,v.currentState,v.changedSinceLastSync,v.lastChangeTime,v.isActive);}
inline void Write(Writer& w,XrActionStateFloat const& v){Put(w,v.currentState,v.changedSinceLastSync,v.lastChangeTime,v.isActive);}
inline void Read(Reader& r,XrActionStateFloat& v){Take(r,v.currentState,v.changedSinceLastSync,v.lastChangeTime,v.isActive);}
inline void Write(Writer& w,XrActionStateVector2f const& v){Put(w,v.currentState.x,v.currentState.y,v.changedSinceLastSync,v.lastChangeTime,v.isActive);}
inline void Read(Reader& r,XrActionStateVector2f& v){Take(r,v.currentState.x,v.currentState.y,v.changedSinceLastSync,v.lastChangeTime,v.isActive);}
inline void WriteEvent(Writer& w,XrEventDataBuffer const& event){
 w.Pod(event.type);
 switch(event.type){
 case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:{auto& e=reinterpret_cast<XrEventDataSessionStateChanged const&>(event);Put(w,HandleBits(e.session),e.state,e.time);break;}
 case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:{auto& e=reinterpret_cast<XrEventDataReferenceSpaceChangePending const&>(event);Put(w,HandleBits(e.session),e.referenceSpaceType,e.changeTime,e.poseValid);Write(w,e.poseInPreviousSpace);break;}
 case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:w.Pod(reinterpret_cast<XrEventDataInstanceLossPending const&>(event).lossTime);break;
 case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:w.Pod(HandleBits(reinterpret_cast<XrEventDataInteractionProfileChanged const&>(event).session));break;
 default:break;
 }
}
inline void ReadEvent(Reader& r,XrEventDataBuffer& event){
 event={XR_TYPE_EVENT_DATA_BUFFER};event.type=r.Pod<XrStructureType>();
 switch(event.type){
 case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:{auto& e=reinterpret_cast<XrEventDataSessionStateChanged&>(event);e.session=FromHandle<XrSession>(r.Pod<uint64_t>());Take(r,e.state,e.time);break;}
 case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:{auto& e=reinterpret_cast<XrEventDataReferenceSpaceChangePending&>(event);e.session=FromHandle<XrSession>(r.Pod<uint64_t>());Take(r,e.referenceSpaceType,e.changeTime,e.poseValid);Read(r,e.poseInPreviousSpace);break;}
 case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:reinterpret_cast<XrEventDataInstanceLossPending&>(event).lossTime=r.Pod<XrTime>();break;
 case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:reinterpret_cast<XrEventDataInteractionProfileChanged&>(event).session=FromHandle<XrSession>(r.Pod<uint64_t>());break;
 default:break;
 }
}
}
