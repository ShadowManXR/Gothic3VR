#pragma once
#include "WireXr.h"
namespace diagnostic::meta_bridge {
inline void WriteFrame(Writer& w,XrFrameEndInfo const& info){
 Put(w,info.displayTime,info.environmentBlendMode,BoundedCount(info.layerCount,8));
 for(uint32_t i=0;i<info.layerCount;++i){auto* layer=info.layers[i];if(!layer)throw std::runtime_error("Missing Meta composition layer");Put(w,layer->type,layer->layerFlags,HandleBits(layer->space));
  if(layer->type==XR_TYPE_COMPOSITION_LAYER_PROJECTION){auto& p=*reinterpret_cast<XrCompositionLayerProjection const*>(layer);w.Pod(BoundedCount(p.viewCount,2));for(uint32_t v=0;v<p.viewCount;++v){Write(w,p.views[v].pose);Write(w,p.views[v].fov);Write(w,p.views[v].subImage);}}
  else if(layer->type==XR_TYPE_COMPOSITION_LAYER_QUAD){auto& q=*reinterpret_cast<XrCompositionLayerQuad const*>(layer);w.Pod(q.eyeVisibility);Write(w,q.subImage);Write(w,q.pose);Put(w,q.size.width,q.size.height);}
  else throw std::runtime_error("Unsupported Meta composition layer");
 }
}
struct FrameLayers {
 XrFrameEndInfo info{XR_TYPE_FRAME_END_INFO};
 std::vector<XrCompositionLayerProjection> projections;std::vector<std::vector<XrCompositionLayerProjectionView>> views;
 std::vector<XrCompositionLayerQuad> quads;std::vector<XrCompositionLayerBaseHeader const*> layers;
 explicit FrameLayers(Reader& r){
  Take(r,info.displayTime,info.environmentBlendMode);auto count=BoundedCount(r.Pod<uint32_t>(),8);projections.reserve(count);views.reserve(count);quads.reserve(count);layers.reserve(count);
  for(uint32_t i=0;i<count;++i){auto type=r.Pod<XrStructureType>();auto flags=r.Pod<XrCompositionLayerFlags>();auto space=FromHandle<XrSpace>(r.Pod<uint64_t>());
   if(type==XR_TYPE_COMPOSITION_LAYER_PROJECTION){XrCompositionLayerProjection p{type};p.layerFlags=flags;p.space=space;p.viewCount=BoundedCount(r.Pod<uint32_t>(),2);views.emplace_back(p.viewCount,XrCompositionLayerProjectionView{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});for(auto& v:views.back()){Read(r,v.pose);Read(r,v.fov);Read(r,v.subImage);}p.views=views.back().data();projections.push_back(p);layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader const*>(&projections.back()));}
   else if(type==XR_TYPE_COMPOSITION_LAYER_QUAD){XrCompositionLayerQuad q{type};q.layerFlags=flags;q.space=space;q.eyeVisibility=r.Pod<XrEyeVisibility>();Read(r,q.subImage);Read(r,q.pose);Take(r,q.size.width,q.size.height);quads.push_back(q);layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader const*>(&quads.back()));}
   else throw std::runtime_error("Unsupported Meta composition layer");
  }
  info.layerCount=count;info.layers=layers.data();
 }
 FrameLayers(FrameLayers const&)=delete;
};
}
