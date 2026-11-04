#include "CinemaScope.h"
#include "RadialTextureCopy.h"
// Exercise the actual compositor submission code against a fake EndFrame.
// No runtime, headset or game process is started by this test.
#include "../src/StereoRuntime.cpp"
#include <cstdio>
struct Submitted
{
    unsigned calls=0,count=0;XrTime time=0;
    XrStructureType types[3]{};XrCompositionLayerQuad quad{},radial{};
    XrCompositionLayerProjection projection{};XrCompositionLayerProjectionView eyes[2]{};
    bool fail=false;
} submitted;
std::vector<XrEventDataBuffer> pendingEvents;unsigned eventIndex=0;
unsigned radialAcquires=0,radialReleases=0;
XrResult XRAPI_CALL AcquireRadialFixture(XrSwapchain,XrSwapchainImageAcquireInfo const*,uint32_t* out){++radialAcquires;*out=0;return XR_SUCCESS;}
XrResult XRAPI_CALL WaitRadialFixture(XrSwapchain,XrSwapchainImageWaitInfo const*){return XR_SUCCESS;}
XrResult XRAPI_CALL ReleaseRadialFixture(XrSwapchain,XrSwapchainImageReleaseInfo const*){++radialReleases;return XR_SUCCESS;}
XrResult XRAPI_CALL PollEventFixture(XrInstance,XrEventDataBuffer* output) {if(eventIndex>=pendingEvents.size())return XR_EVENT_UNAVAILABLE;*output=pendingEvents[eventIndex++];return XR_SUCCESS;}
XrResult XRAPI_CALL EndFrameFixture(XrSession,XrFrameEndInfo const* frame)
{
    ++submitted.calls;submitted.count=frame->layerCount;submitted.time=frame->displayTime;
    for(unsigned i=0;i<frame->layerCount;++i)
    {
        submitted.types[i]=frame->layers[i]->type;
        if(frame->layers[i]->type==XR_TYPE_COMPOSITION_LAYER_QUAD){auto const& quad=*reinterpret_cast<XrCompositionLayerQuad const*>(frame->layers[i]);if(quad.subImage.swapchain==16)submitted.radial=quad;else submitted.quad=quad;}
        if(frame->layers[i]->type==XR_TYPE_COMPOSITION_LAYER_PROJECTION)
        {
            submitted.projection=*reinterpret_cast<XrCompositionLayerProjection const*>(frame->layers[i]);
            for(unsigned eye=0;eye<2;++eye)submitted.eyes[eye]=submitted.projection.views[eye];
        }
    }
    return submitted.fail?XR_ERROR_RUNTIME_FAILURE:XR_SUCCESS;
}
int main()
{
    try
    {
        wchar_t directory[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,directory);GetTempFileNameW(directory,L"g3u",0,path);
        unsigned failures=0;auto check=[&](bool ok,char const* name){printf("%s %s\n",ok?"PASS":"FAIL",name);if(!ok)++failures;};
        {
            diagnostic::Runtime runtime(path);runtime.api.EndFrame=EndFrameFixture;
            runtime.api.PollEvent=PollEventFixture;runtime.session=21;
            auto queueChange=[&](XrSession session,XrReferenceSpaceType space,XrTime time){XrEventDataReferenceSpaceChangePending change{XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING};change.session=session;change.referenceSpaceType=space;change.changeTime=time;change.poseValid=XR_FALSE;XrEventDataBuffer buffer{};memcpy(&buffer,&change,sizeof(change));pendingEvents.push_back(buffer);};
            queueChange(99,XR_REFERENCE_SPACE_TYPE_LOCAL,10);queueChange(21,XR_REFERENCE_SPACE_TYPE_VIEW,20);
            queueChange(21,XR_REFERENCE_SPACE_TYPE_LOCAL,100);queueChange(21,XR_REFERENCE_SPACE_TYPE_LOCAL,300);
            runtime.Events();check(runtime.referenceChanges.size()==2&&!runtime.ReferenceChangeAt(99),"only this session's LOCAL origin events affect game camera, never before changeTime");
            check(runtime.ReferenceChangeAt(100)&&runtime.referenceChanges.size()==1&&!runtime.ReferenceChangeAt(101),"reference reset fires once at the first predicted display time using the new origin, even without poseInPreviousSpace");
            check(!runtime.ReferenceChangeAt(299)&&runtime.ReferenceChangeAt(300)&&runtime.referenceChanges.empty(),"multiple pending reference changes retain their independent timing");
            runtime.session=XR_NULL_HANDLE;
            runtime.width=2560;runtime.height=1440;runtime.space=11;runtime.viewSpace=12;runtime.uiSwapchain=13;
            runtime.eyes[0].swapchain=14;runtime.eyes[1].swapchain=15;runtime.frame.predictedDisplayTime=123456;
            runtime.views[0].pose.position.x=-.032f;runtime.views[1].pose.position.x=.032f;
            runtime.begun=true;runtime.captured=3;runtime.uiCaptured=true;runtime.End(true);
            check(submitted.count==2&&submitted.types[0]==XR_TYPE_COMPOSITION_LAYER_PROJECTION&&submitted.types[1]==XR_TYPE_COMPOSITION_LAYER_QUAD&&
                submitted.projection.viewCount==2&&submitted.eyes[0].subImage.swapchain==14&&submitted.eyes[1].subImage.swapchain==15&&
                submitted.eyes[0].pose.position.x<0&&submitted.eyes[1].pose.position.x>0,"world remains two geometric eyes, UI follows world in layer order");
            check(submitted.quad.space==12&&submitted.quad.eyeVisibility==XR_EYE_VISIBILITY_BOTH&&submitted.quad.pose.orientation.w==1&&
                submitted.quad.pose.position.z==-2&&submitted.quad.pose.position.y==-.3f&&submitted.quad.size.width==2.6f&&std::fabs(submitted.quad.size.height-1.4625f)<.0001f&&
                submitted.quad.layerFlags==XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT&&submitted.quad.subImage.imageRect.extent.width==2560&&
                submitted.time==123456,"head-relative finite-distance panel, both eyes, original aspect and premultiplied alpha");
            runtime.radial={true,2,0};runtime.radialValid=true;runtime.radialSwapchain=16;runtime.begun=true;runtime.End(true);
            check(submitted.count==3&&submitted.radial.space==12&&submitted.radial.subImage.swapchain==16&&std::fabs(submitted.radial.size.width-.528f)<.0001f&&submitted.radial.pose.position.z==-1.3f&&submitted.radial.subImage.imageRect.extent.width==768,"smaller radial is a separate both-eye VIEW layer after world and native UI, independent of 4K menu scaling");
            runtime.radial.visible=false;runtime.radialSwapchain=XR_NULL_HANDLE;--runtime.submitted;
            diagnostic::VrSettings settings;settings.numbers[3]=20;settings.numbers[4]=80;settings.numbers[5]=300;
            runtime.uiLayout=diagnostic::PanelLayout(settings,true);
            runtime.begun=true;runtime.captured=0;runtime.uiCaptured=true;runtime.End(true);
            check(submitted.count==1&&submitted.types[0]==XR_TYPE_COMPOSITION_LAYER_QUAD&&runtime.uiOnlyFrames==1&&runtime.submitted==2,"UI-only paused/menu frame submits without requiring a perspective camera");
            check(std::fabs(submitted.quad.pose.position.y-.2f)<.0001f&&submitted.quad.pose.position.z==-3&&std::fabs(submitted.quad.size.width-2.08f)<.0001f&&
                std::fabs(submitted.quad.size.height/submitted.quad.size.width-1440.f/2560.f)<.0001f,"configured menu height, scale and distance reach actual compositor without changing aspect");
            auto oldSubmitted=submitted;auto oldCount=runtime.submitted;auto oldUi=runtime.uiOnlyFrames;
            auto oldCaptured=runtime.captured;auto oldOutW=runtime.outputWidth,oldOutH=runtime.outputHeight;
            runtime.captured=3;
            for(unsigned square:{2560u,2880u,3200u}){
                runtime.width=runtime.height=runtime.outputWidth=runtime.outputHeight=square;
                runtime.begun=true;runtime.End(true);
                auto canvas=diagnostic::MenuCanvas(square,square,true);
                check(canvas.width==square&&canvas.height==square*9/16&&
                      std::fabs(submitted.quad.size.width-2.08f)<.0001f&&
                      std::fabs(submitted.quad.size.height-1.17f)<.0001f&&
                      submitted.quad.subImage.imageRect.extent.width==int(square)&&
                      submitted.quad.subImage.imageRect.extent.height==int(square*9/16)&&
                      submitted.eyes[0].subImage.imageRect.extent.height==int(square)&&
                      submitted.eyes[1].subImage.imageRect.extent.width==int(square),
                      "native landscape menu subrect, full menu width, square geometric eye targets");
            }
            runtime.uiLayout=diagnostic::PanelLayout(settings,false);runtime.begun=true;runtime.End(true);
            check(submitted.quad.subImage.imageRect.extent.height==3200&&
                  submitted.quad.size.height==runtime.uiLayout.widthMeters,
                  "square HUD is neither cropped nor resized");
            runtime.uiLayout=diagnostic::PanelLayout(settings,true);runtime.cinemaCaptured=true;runtime.begun=true;runtime.End(true);
            check(submitted.quad.subImage.imageRect.extent.height==3200&&
                  submitted.quad.size.height==runtime.uiLayout.widthMeters,
                  "cinema keeps its original full image independent of landscape menus");
            runtime.cinemaCaptured=false;runtime.width=2560;runtime.height=1440;
            runtime.outputWidth=oldOutW;runtime.outputHeight=oldOutH;runtime.captured=oldCaptured;
            runtime.submitted=oldCount;runtime.uiOnlyFrames=oldUi;submitted=oldSubmitted;

            runtime.begun=true;runtime.captured=0;runtime.uiCaptured=true;runtime.cinemaCaptured=true;runtime.End(true);
            check(submitted.count==1&&submitted.quad.eyeVisibility==XR_EYE_VISIBILITY_BOTH&&submitted.quad.layerFlags==0&&runtime.uiOnlyFrames==2,"cinema uses one opaque finite-distance quad in both eyes even if video source alpha is zero");
            runtime.cinemaCaptured=false;
            diagnostic::CinemaScope cinema;check(cinema.Begin(17)&&cinema.Active(17)&&!cinema.Active(18)&&cinema.Capture(17)&&!cinema.Capture(17),"movie capture is scoped to its native thread and rejects reentrant presentation");cinema.Finish();
            check(!cinema.Begin(18)&&cinema.Begin(17)&&cinema.Capture(17),"only same-thread nested native videos extend scope");cinema.Finish();cinema.End(17);check(cinema.Active(17),"inner native video return retains outer scope");cinema.End(17);check(!cinema.Active(17)&&!cinema.Capture(17),"movie exit disables capture on normal gameplay Present");
            runtime.begun=true;runtime.captured=1;runtime.uiCaptured=true;runtime.End(false);
            check(submitted.count==0&&runtime.submitted==3&&!runtime.begun,"cancelled partial frame sends zero layers");
            auto calls=submitted.calls;runtime.End(false);check(submitted.calls==calls,"already-ended frame is not ended twice");
            submitted.fail=true;runtime.begun=true;runtime.captured=3;bool failed=false;
            try{runtime.End(true);}catch(std::exception const&){failed=true;}
            check(failed&&!runtime.begun&&runtime.submitted==3,"EndFrame error preserves completed-submission count and cannot double-end");
            auto d3dx=LoadLibraryExW(L"d3dx9_40.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
            check(d3dx!=nullptr,"installed native D3DX texture converter is available");
            Microsoft::WRL::ComPtr<IDirect3D9Ex> d9;Microsoft::WRL::ComPtr<IDirect3DDevice9Ex> device9;
            stereo::Hr(Direct3DCreate9Ex(D3D_SDK_VERSION,&d9),"icon D3D9Ex factory");
            D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=GetDesktopWindow();pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_X8R8G8B8;
            stereo::Hr(d9->CreateDeviceEx(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,pp.hDeviceWindow,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,nullptr,&device9),"icon D3D9Ex device");
            Microsoft::WRL::ComPtr<IDirect3DTexture9> atlas,staging;
            stereo::Hr(device9->CreateTexture(128,64,1,0,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&staging,nullptr),"icon atlas staging");
            stereo::Hr(device9->CreateTexture(128,64,1,0,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&atlas,nullptr),"native default pool icon atlas");
            D3DLOCKED_RECT iconLock{};stereo::Hr(staging->LockRect(0,&iconLock,nullptr,0),"icon atlas lock");
            for(unsigned y=0;y<64;++y)for(unsigned x=0;x<128;++x)reinterpret_cast<uint32_t*>(static_cast<char*>(iconLock.pBits)+y*iconLock.Pitch)[x]=x<64?0xffff0000:0x802080e0;
            staging->UnlockRect(0);stereo::Hr(device9->UpdateTexture(staging.Get(),atlas.Get()),"upload icon atlas");
            auto nativeIcon=diagnostic::CopyRadialTexture(atlas.Get(),RECT{64,0,128,64});
            check(nativeIcon&&(*nativeIcon)[32*64+32]==0x802080e0,"native default-pool atlas is cropped with alpha intact for the radial item icon");
            check(!diagnostic::CopyRadialTexture(atlas.Get(),RECT{-1,0,64,64}),"invalid icon atlas rectangle is rejected");
            Microsoft::WRL::ComPtr<IDirect3DTexture9> compressed,compressedStaging;
            stereo::Hr(device9->CreateTexture(128,64,1,0,D3DFMT_DXT3,D3DPOOL_SYSTEMMEM,&compressedStaging,nullptr),"compressed icon staging");
            stereo::Hr(device9->CreateTexture(128,64,1,0,D3DFMT_DXT3,D3DPOOL_DEFAULT,&compressed,nullptr),"native compressed icon atlas");
            stereo::Hr(compressedStaging->LockRect(0,&iconLock,nullptr,0),"compressed atlas lock");
            for(unsigned y=0;y<16;++y)for(unsigned x=0;x<32;++x){auto* block=static_cast<unsigned char*>(iconLock.pBits)+y*iconLock.Pitch+x*16;memset(block,0,16);memset(block,0x88,8);block[8]=0xe0;block[9]=7;block[10]=0xe0;block[11]=7;}
            compressedStaging->UnlockRect(0);stereo::Hr(device9->UpdateTexture(compressedStaging.Get(),compressed.Get()),"compressed icon upload");
            D3DVIEWPORT9 originalIconViewport{3,5,40,42,.2f,.8f};RECT originalScissor{7,9,31,35};
            device9->SetViewport(&originalIconViewport);device9->SetScissorRect(&originalScissor);
            device9->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);device9->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
            device9->SetTexture(0,atlas.Get());device9->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE);
            Microsoft::WRL::ComPtr<IDirect3DSurface9> originalIconTarget;device9->GetRenderTarget(0,&originalIconTarget);
            auto compressedIcon=diagnostic::CopyRadialTexture(compressed.Get(),RECT{64,0,128,64});
            D3DVIEWPORT9 restoredViewport{};RECT restoredScissor{};DWORD restoredScissorEnabled=0,restoredBlend=0,restoredFvf=0;
            device9->GetViewport(&restoredViewport);device9->GetScissorRect(&restoredScissor);device9->GetRenderState(D3DRS_SCISSORTESTENABLE,&restoredScissorEnabled);
            device9->GetRenderState(D3DRS_ALPHABLENDENABLE,&restoredBlend);device9->GetFVF(&restoredFvf);
            Microsoft::WRL::ComPtr<IDirect3DSurface9> restoredTarget;Microsoft::WRL::ComPtr<IDirect3DBaseTexture9> restoredTexture;
            device9->GetRenderTarget(0,&restoredTarget);device9->GetTexture(0,&restoredTexture);
            check(!memcmp(&restoredViewport,&originalIconViewport,sizeof(restoredViewport))&&EqualRect(&restoredScissor,&originalScissor)&&restoredScissorEnabled&&restoredBlend&&
                  restoredFvf==(D3DFVF_XYZ|D3DFVF_DIFFUSE)&&restoredTarget==originalIconTarget&&restoredTexture.Get()==atlas.Get(),"compressed icon conversion restores native viewport, target, texture, vertex format and render states");
            stereo::Hr(device9->BeginScene(),"existing native scene");auto inSceneIcon=diagnostic::CopyRadialTexture(compressed.Get(),RECT{64,0,128,64});
            check(inSceneIcon&&(*inSceneIcon)[32*64+32]==0x8800ff00&&SUCCEEDED(device9->EndScene()),"icon conversion preserves an already active native scene");
            check(compressedIcon&&(*compressedIcon)[32*64+32]==0x8800ff00,"compressed native DXT3 atlas converts to BGRA with translucent item pixels intact");
            if(d3dx)FreeLibrary(d3dx);
            runtime.renderer.Initialize();D3D11_QUERY_DESC query{D3D11_QUERY_EVENT,0};stereo::Hr(runtime.renderer.device->CreateQuery(&query,&runtime.fence11),"radial GPU fence");
            D3D11_TEXTURE2D_DESC desc{};desc.Width=desc.Height=diagnostic::RadialPixels;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
            stereo::ComPtr<ID3D11Texture2D> target,readback;stereo::Hr(runtime.renderer.device->CreateTexture2D(&desc,nullptr,&target),"radial GPU target");
            desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;stereo::Hr(runtime.renderer.device->CreateTexture2D(&desc,nullptr,&readback),"radial GPU readback");
            runtime.radialSwapchain=16;runtime.radialImages.push_back({XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR,nullptr,target.Get()});runtime.radialValid=false;runtime.radial={true,2,0};
            runtime.api.AcquireSwapchainImage=AcquireRadialFixture;runtime.api.WaitSwapchainImage=WaitRadialFixture;runtime.api.ReleaseSwapchainImage=ReleaseRadialFixture;
            runtime.CaptureRadial();check(radialAcquires==1&&radialReleases==0&&runtime.pendingCopies.Count()==1,"radial destination remains acquired until same-frame completion fence");runtime.CompleteCopies();
            runtime.renderer.context->CopyResource(readback.Get(),target.Get());D3D11_MAPPED_SUBRESOURCE mapped{};stereo::Hr(runtime.renderer.context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped),"radial GPU map");
            auto expected=diagnostic::RadialImage(runtime.radial);bool matches=true;for(unsigned y=0;y<diagnostic::RadialPixels;++y)matches&=!memcmp(static_cast<char*>(mapped.pData)+y*mapped.RowPitch,expected.data()+y*diagnostic::RadialPixels,diagnostic::RadialPixels*4);runtime.renderer.context->Unmap(readback.Get(),0);
            check(matches&&radialAcquires==1&&radialReleases==1,"actual radial image survives D3D11 upload with exact premultiplied alpha and all labels");
            runtime.CaptureRadial();check(radialAcquires==1,"unchanged radial reuses last released XR image without rerasterizing or acquiring");
            runtime.radial.selected=3;runtime.CaptureRadial();runtime.CompleteCopies();check(radialAcquires==2&&radialReleases==2&&runtime.radialUploaded.selected==3,"selection change uploads and releases a fresh image");
            auto icon=std::make_shared<diagnostic::RadialIcon>();icon->fill(0xff40a0e0);
            runtime.radialItems[0]={7,icon};runtime.CaptureRadial();runtime.CompleteCopies();
            check(radialAcquires==3&&runtime.radialItemsUploaded[0].amount==7,"new item icon invalidates the radial image without a selection change");
            runtime.radialItems[0].amount=6;runtime.CaptureRadial();runtime.CompleteCopies();
            check(radialAcquires==4&&runtime.radialItemsUploaded[0].amount==6,"quantity changes invalidate the radial upload cache");
            runtime.CaptureRadial();check(radialAcquires==4,"unchanged items and counts reuse the radial image");
            runtime.radialItems[0]={};runtime.CaptureRadial();runtime.CompleteCopies();
            check(radialAcquires==5&&!runtime.radialItemsUploaded[0].icon,"cleared quick slots cannot retain a stale uploaded icon");
            // Restore the fixture's acquisition baseline for status-layer checks.
            radialAcquires-=3;radialReleases-=3;
            runtime.radial.visible=false;runtime.CaptureRadial();check(radialAcquires==2,"hidden radial allocates and uploads nothing");
            settings.numbers[1]=-70;settings.numbers[2]=100;settings.numbers[5]=200;runtime.uiLayout=diagnostic::PanelLayout(settings,false);
            runtime.worldStatus=1;runtime.CaptureRadial();check(radialAcquires==3&&runtime.uploadedStatus==1,"native sneak status uploads despite radial being hidden");
            runtime.begun=true;runtime.captured=3;runtime.uiCaptured=true;submitted.fail=false;runtime.End(true);
            check(submitted.count==3&&submitted.radial.subImage.imageRect.extent.width==diagnostic::artwork::StatusIconPixels&&submitted.radial.subImage.imageRect.extent.height==diagnostic::artwork::StatusIconPixels&&submitted.radial.size.height==.07f&&
                submitted.radial.pose.position.x==0&&std::fabs(submitted.radial.pose.position.y+.737225f)<.00001f&&submitted.radial.eyeVisibility==XR_EYE_VISIBILITY_BOTH,"sneak icon is centered directly above the lowered HUD in both eyes");
            auto sharedStatusPose=submitted.radial.pose;
            runtime.CaptureRadial();check(radialAcquires==3,"unchanged sneak state reuses texture");runtime.worldStatus=2;runtime.CaptureRadial();check(radialAcquires==4&&runtime.uploadedStatus==2,"finisher-ready badge replaces sneak without a second status layer");
            runtime.begun=true;runtime.captured=3;runtime.uiCaptured=true;runtime.End(true);
            check(submitted.count==3&&submitted.radial.subImage.imageRect.extent.width==diagnostic::artwork::StatusIconPixels&&submitted.radial.size.width==.07f&&
                submitted.radial.pose.position.x==sharedStatusPose.position.x&&submitted.radial.pose.position.y==sharedStatusPose.position.y&&submitted.radial.pose.position.z==sharedStatusPose.position.z,"red skull uses exactly the same compact layer and position as Sneak");
            runtime.renderer.context->CopyResource(readback.Get(),target.Get());stereo::Hr(runtime.renderer.context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped),"skull GPU map");
            auto skull=diagnostic::WorldStatusImage(2);matches=true;for(unsigned y=0;y<diagnostic::RadialPixels;++y)matches&=!memcmp(static_cast<char*>(mapped.pData)+y*mapped.RowPitch,skull.data()+y*diagnostic::RadialPixels,diagnostic::RadialPixels*4);runtime.renderer.context->Unmap(readback.Get(),0);
            check(matches,"actual red skull texture and transparent background survive compositor upload");
            runtime.worldStatus=4;runtime.CaptureRadial();runtime.begun=true;runtime.captured=3;runtime.uiCaptured=true;runtime.End(true);
            check(submitted.count==3&&submitted.radial.subImage.imageRect.extent.width==diagnostic::artwork::StatusIconPixels&&submitted.radial.size.width==.07f&&submitted.radial.size.height==.07f&&
                submitted.radial.pose.position.x==sharedStatusPose.position.x&&submitted.radial.pose.position.y==sharedStatusPose.position.y&&submitted.radial.pose.position.z==sharedStatusPose.position.z,"Block shares the same icon size and position without adding another layer");
            runtime.worldStatus=0;runtime.begun=true;runtime.captured=3;runtime.uiCaptured=true;runtime.End(true);check(submitted.count==2,"native sneak exit and no target remove status immediately");
            runtime.radialSwapchain=XR_NULL_HANDLE;
        }
        DeleteFileW(path);return failures?1:0;
    }
    catch(std::exception const& e){fprintf(stderr,"FAIL %s\n",e.what());return 1;}
}
