#include "meta_bridge/WireXr.h"
#include "meta_bridge/Layers.h"
#include <iostream>
#include <fstream>
using namespace diagnostic::meta_bridge;
void Check(bool b,char const* why){if(!b)throw std::runtime_error(why);}
int main(int argc,char** argv){try{
 Writer w;w.Text("Gothic 3 VR Meta bridge");w.Wide(L"Local\\G3VR-shared-123");Put(w,uint64_t(0x123456789abcdef0),uint32_t(0x76543210));
 XrView view{XR_TYPE_VIEW};view.pose={{.1f,.2f,.3f,.9f},{-.032f,1.65f,-.1f}};view.fov={-.8f,.9f,1.f,-.7f};Write(w,view);
 XrSwapchainCreateInfo sc{XR_TYPE_SWAPCHAIN_CREATE_INFO};sc.format=91;sc.usageFlags=1;sc.sampleCount=sc.faceCount=sc.arraySize=sc.mipCount=1;sc.width=3840;sc.height=2160;Write(w,sc);
 XrEventDataReferenceSpaceChangePending event{XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING};event.session=FromHandle<XrSession>(0x12345678);event.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL;event.changeTime=1234567890123;event.poseValid=1;event.poseInPreviousSpace=view.pose;XrEventDataBuffer buffer{XR_TYPE_EVENT_DATA_BUFFER};memcpy(&buffer,&event,sizeof(event));WriteEvent(w,buffer);
 auto bytes=w.bytes;Reader r(bytes);Check(r.Text()=="Gothic 3 VR Meta bridge","text mismatch");Check(r.Wide()==L"Local\\G3VR-shared-123","path mismatch");Check(r.Pod<uint64_t>()==0x123456789abcdef0&&r.Pod<uint32_t>()==0x76543210,"scalar mismatch");
 XrView readView{XR_TYPE_VIEW};Read(r,readView);Check(!memcmp(&readView.pose,&view.pose,sizeof(XrPosef))&&!memcmp(&readView.fov,&view.fov,sizeof(XrFovf)),"view mismatch");
 XrSwapchainCreateInfo readSc{XR_TYPE_SWAPCHAIN_CREATE_INFO};Read(r,readSc);Check(readSc.width==3840&&readSc.height==2160&&readSc.format==91&&readSc.arraySize==1,"swapchain mismatch");
 XrEventDataBuffer readEvent{XR_TYPE_EVENT_DATA_BUFFER};ReadEvent(r,readEvent);auto& re=reinterpret_cast<XrEventDataReferenceSpaceChangePending const&>(readEvent);Check(re.type==event.type&&HandleBits(re.session)==0x12345678&&re.changeTime==event.changeTime&&re.poseInPreviousSpace.position.y==view.pose.position.y,"reference event mismatch");r.Complete();
 bool refused=false;try{r.Pod<uint64_t>();}catch(std::exception const&){refused=true;}Check(refused,"truncated input accepted");refused=false;try{Writer bad;bad.Text(std::string(32769,'x'));}catch(std::exception const&){refused=true;}Check(refused,"oversized string accepted");
 XrCompositionLayerProjectionView pv[2]={{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW},{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}};for(unsigned n=0;n<2;++n){pv[n].pose=view.pose;pv[n].fov=view.fov;pv[n].subImage.swapchain=FromHandle<XrSwapchain>(0x1234567890+n);pv[n].subImage.imageRect={{1,2},{3840,2160}};}
 XrCompositionLayerProjection projection{XR_TYPE_COMPOSITION_LAYER_PROJECTION};projection.space=FromHandle<XrSpace>(0x1234567898);projection.viewCount=2;projection.views=pv;XrCompositionLayerQuad quad{XR_TYPE_COMPOSITION_LAYER_QUAD};quad.space=projection.space;quad.layerFlags=XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;quad.eyeVisibility=XR_EYE_VISIBILITY_BOTH;quad.pose=view.pose;quad.size={2.f,1.125f};quad.subImage=pv[1].subImage;
 XrCompositionLayerBaseHeader const* layers[]={reinterpret_cast<XrCompositionLayerBaseHeader*>(&projection),reinterpret_cast<XrCompositionLayerBaseHeader*>(&quad)};XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO};end.displayTime=1234567890123;end.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;end.layerCount=2;end.layers=layers;Writer fw;WriteFrame(fw,end);Reader fr(fw.bytes);FrameLayers owned(fr);fr.Complete();Check(owned.info.displayTime==end.displayTime&&owned.info.layerCount==2&&owned.projections[0].viewCount==2&&HandleBits(owned.projections[0].views[0].subImage.swapchain)==0x1234567890&&owned.quads[0].size.width==2.f&&HandleBits(owned.quads[0].space)==0x1234567898,"projection/UI layer cross-architecture mismatch");bytes.insert(bytes.end(),fw.bytes.begin(),fw.bytes.end());
 if(argc==2){std::ofstream out(argv[1],std::ios::binary);out.write(reinterpret_cast<char const*>(bytes.data()),bytes.size());Check(bool(out),"fixture output failed");}
 std::cout<<"PASS Meta wire records, x"<<sizeof(void*)*8<<", bytes="<<bytes.size()<<", bounds and reference event\n";return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
