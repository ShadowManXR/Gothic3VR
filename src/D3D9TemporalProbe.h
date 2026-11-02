#pragma once
#include <d3d9.h>
#include <wrl/client.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <ostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <memory>
#include "D3D9AnimatedMotionProbe.h"
#include "TemporalInstanceIdentity.h"
#ifdef G3VR_NATIVE_MOTION
#include "D3D9NativeMotionCapture.h"
#endif
#ifdef G3VR_NATIVE_SKY
#include "D3D9SkyMotion.h"
#endif

namespace g3vr {
// Opt-in, bounded metadata observation. No setters, GPU readbacks, waits,
// allocation, shader replacement or I/O in Observe (called before native draws).
class D3D9TemporalProbe {
    using MicrosoftSurface=Microsoft::WRL::ComPtr<IDirect3DSurface9>;
    struct Surface {
        uintptr_t identity=0,texture=0;HRESULT result=D3DERR_NOTFOUND;D3DSURFACE_DESC desc{};
        void Read(IDirect3DSurface9* value,HRESULT hr){
            result=hr;if(FAILED(hr)||!value)return;identity=reinterpret_cast<uintptr_t>(value);result=value->GetDesc(&desc);
            Microsoft::WRL::ComPtr<IDirect3DTexture9> container;
            if(SUCCEEDED(value->GetContainer(IID_PPV_ARGS(&container))))texture=reinterpret_cast<uintptr_t>(container.Get());
        }
    };
    struct Pass {Surface target,depth;D3DVIEWPORT9 viewport{};DWORD z=0,zwrite=0,alpha=0,colorWrite=0;uintptr_t vertexShader=0,pixelShader=0;unsigned drawIndex=0;};
    struct Shader {
        bool vertex=false;uintptr_t identity=0;unsigned bytes=0;uint64_t hash=0;
        std::array<unsigned char,32768> code{};std::array<float,1024> constants{};HRESULT constantsResult=E_FAIL;
    };
    struct Eye {
        uint64_t observationTicks=0;
        uint64_t frame=0,displayTime=0;unsigned width=0,height=0,draws=0,queries=0,passes=0,shaderCount=0;bool active=false,complete=false,matrices=false;
        float view[16]{},projection[16]{};std::array<float,2> jitter{};std::array<Pass,24> pass{};std::unique_ptr<std::array<Shader,64>> shaders;
    };
    std::array<Eye,2> eyes;int current=-1;
    bool metadata=true;
    std::unique_ptr<D3D9AnimatedMotionProbe> motion;
#ifdef G3VR_NATIVE_SKY
    std::unique_ptr<D3D9SkyMotion> sky;
#endif
#ifdef G3VR_NATIVE_MOTION
    std::unique_ptr<D3D9NativeMotionCapture> native;
#endif
    static uint64_t Hash(unsigned char const* code,unsigned size){uint64_t h=14695981039346656037ull;for(unsigned i=0;i<size;++i){h^=code[i];h*=1099511628211ull;}return h;}
    template<class T>static void CaptureShader(Shader& out,T* shader,bool vertex,IDirect3DDevice9* device){
        out.bytes=0;out.constantsResult=E_FAIL;if(!shader)return;UINT bytes=0;if(FAILED(shader->GetFunction(nullptr,&bytes))||!bytes||bytes>out.code.size())return;
        UINT capacity=bytes;if(FAILED(shader->GetFunction(out.code.data(),&capacity))||capacity!=bytes)return;
        out.bytes=bytes;out.vertex=vertex;out.identity=reinterpret_cast<uintptr_t>(shader);out.hash=Hash(out.code.data(),bytes);
        if(vertex)out.constantsResult=device->GetVertexShaderConstantF(0,out.constants.data(),256);
    }
    static void Floats(std::ostream& out,float const* values,unsigned count){out<<'[';for(unsigned i=0;i<count;++i){if(i)out<<',';if(std::isfinite(values[i]))out<<values[i];else out<<"null";}out<<']';}
    static void WriteSurface(std::ostream& out,Surface const& s){out<<"{\"identity\":"<<s.identity<<",\"texture_container_identity\":"<<s.texture<<",\"hresult\":"<<int32_t(s.result)<<",\"width\":"<<s.desc.Width<<",\"height\":"<<s.desc.Height<<",\"format\":"<<unsigned(s.desc.Format)<<",\"usage\":"<<s.desc.Usage<<",\"pool\":"<<unsigned(s.desc.Pool)<<",\"msaa\":"<<unsigned(s.desc.MultiSampleType)<<'}';}
public:
    void SetMetadata(bool enabled)noexcept{metadata=enabled;
#ifdef G3VR_NATIVE_MOTION
        if(native)native->SetSurveyEnabled(enabled);
#endif
    }
    void AddObservationTicks(uint64_t ticks)noexcept{if(Active())eyes[unsigned(current)].observationTicks+=ticks;}
    uint64_t ObservationTicks(unsigned index)const noexcept{return index<2?eyes[index].observationTicks:0;}
    void Prepare(){for(auto& e:eyes)if(!e.shaders)e.shaders=std::make_unique<std::array<Shader,64>>();if(!motion)motion=std::make_unique<D3D9AnimatedMotionProbe>();
#ifdef G3VR_NATIVE_SKY
        if(!sky)sky=std::make_unique<D3D9SkyMotion>();
#endif
#ifdef G3VR_NATIVE_MOTION
        if(!native){native=std::make_unique<D3D9NativeMotionCapture>();native->Prepare();}
        motion->UseBufferObserver(native->Buffers());
#endif
    }
    void ReleaseStorage()noexcept {Discard();for(auto& e:eyes)e.shaders.reset();motion.reset();
#ifdef G3VR_NATIVE_SKY
        sky.reset();
#endif
#ifdef G3VR_NATIVE_MOTION
        native.reset();
#endif
    }
    bool Active()const noexcept {return current>=0;}
    bool Capturing(unsigned eye,uint64_t frame)const noexcept {return eye<2&&current==int(eye)&&eyes[eye].active&&eyes[eye].frame==frame&&eyes[eye].matrices&&eyes[eye].queries;}
    void Begin(unsigned eye,uint64_t frame,unsigned width,unsigned height,uint64_t displayTime=0,std::array<float,2> jitter={}){if(eye>1)return;if(!eyes[eye].shaders||!motion)Prepare();current=int(eye);auto& e=eyes[eye];e.frame=frame;e.displayTime=displayTime;e.width=width;e.height=height;e.jitter=jitter;e.draws=e.queries=e.passes=e.shaderCount=0;e.complete=e.matrices=false;e.active=true;motion->width=width;motion->height=height;motion->Begin(eye,frame,displayTime,jitter);
#ifdef G3VR_NATIVE_MOTION
        if(native)native->Begin(eye,frame,displayTime,width,height,jitter);
#endif
        e.observationTicks=0;
#ifdef G3VR_NATIVE_SKY
        if(metadata&&sky&&displayTime){diagnostic::temporal::GeometryFrameStamp stamp{frame,1,displayTime,eye};
#ifdef G3VR_NATIVE_MOTION
            if(native)stamp=native->CurrentStamp(eye);
#endif
            try{sky->Begin(eye,stamp,width,height,jitter);}catch(...){sky->End();}}
#endif
    }
    void Matrices(float const* view,float const* projection){if(!Active()||!view||!projection)return;auto& e=eyes[unsigned(current)];std::memcpy(e.view,view,sizeof(e.view));std::memcpy(e.projection,projection,sizeof(e.projection));e.matrices=true;}
    void End(unsigned eye){if(eye>1||current!=int(eye))return;auto& e=eyes[eye];e.active=false;e.complete=true;Stop();}
    void Stop(){if(Active())eyes[unsigned(current)].active=false;current=-1;if(motion)motion->Stop();
#ifdef G3VR_NATIVE_SKY
        if(sky)sky->End();
#endif
#ifdef G3VR_NATIVE_MOTION
        if(native)native->Stop();
#endif
    }
    void Discard(){Stop();for(auto& e:eyes){e.active=e.complete=false;}}
    unsigned PushObject(diagnostic::temporal::Object const& object)noexcept {return Active()&&motion?motion->Push(object):UINT_MAX;}
    void PopObject(unsigned token)noexcept {if(motion&&token!=UINT_MAX)motion->Pop(token);}
    void ResetObjectHistory()noexcept {if(motion)motion->ResetHistory();
#ifdef G3VR_NATIVE_SKY
        if(sky)sky->ResetHistory();
#endif
#ifdef G3VR_NATIVE_MOTION
        if(native)native->Reset();
#endif
    }
    unsigned PushInstances(diagnostic::temporal::InstanceEntity const* entities,unsigned count)noexcept {
#ifdef G3VR_NATIVE_MOTION
        if(Active()&&native)return native->Push(entities,count);
#endif
        return UINT_MAX;
    }
    void PopInstances(unsigned token)noexcept {
#ifdef G3VR_NATIVE_MOTION
        if(native)native->Pop(token);
#endif
    }
    void ObserveMotion(IDirect3DDevice9* d,diagnostic::temporal::Geometry const& geometry)noexcept {if(Active()&&motion)motion->Observe(d,geometry);
#ifdef G3VR_NATIVE_MOTION
        if(Active()&&native)native->Observe(d,geometry);
#endif
    }
    void ObserveAuxiliaryDraw(IDirect3DDevice9* d,diagnostic::temporal::Geometry const& geometry,unsigned kind)noexcept {
#ifdef G3VR_NATIVE_MOTION
        if(Active()&&native)native->SurveyDraw(d,geometry,kind);
#endif
    }
    void CaptureMotion(IDirect3DDevice9* d,IDirect3DSurface9* depth)noexcept {if(Active()&&motion)motion->CaptureRaster(d,depth);
#ifdef G3VR_NATIVE_MOTION
        if(Active()&&native)native->Capture(d,depth);
#endif
#ifdef G3VR_NATIVE_SKY
        if(metadata&&Active()&&sky)try{sky->OpaqueBoundary(d,depth);}catch(...){sky->End();}
#endif
    }
    void AfterDraw(IDirect3DDevice9* d,diagnostic::temporal::Geometry const& geometry)noexcept {
#ifdef G3VR_NATIVE_SKY
        if(metadata&&Active()&&sky)try{sky->Replay(d,geometry);}catch(...){sky->End();}
#endif
    }
    IDirect3DSurface9* SkySurface(unsigned index)const noexcept {
#ifdef G3VR_NATIVE_SKY
        return metadata&&sky?sky->Surface(index):nullptr;
#else
        return nullptr;
#endif
    }
    bool SkyHistory(unsigned index)const noexcept {
#ifdef G3VR_NATIVE_SKY
        return metadata&&sky&&index<2&&sky->Status(index).draws==1&&!sky->Status(index).rejected&&sky->Status(index).history;
#else
        return false;
#endif
    }
    IDirect3DSurface9* SkinSurface(unsigned index)const noexcept{return motion?motion->Surface(index):nullptr;}
    IDirect3DSurface9* NativeSurface(unsigned index)const noexcept{
#ifdef G3VR_NATIVE_MOTION
        if(native){auto* r=native->Renderer(index);return r?r->Surface():nullptr;}
#endif
        return nullptr;
    }
    bool Camera(unsigned index,uint64_t frame,float const*& view,float const*& projection,diagnostic::temporal::GeometryFrameStamp& stamp)const noexcept {
        if(index>1||!eyes[index].matrices||eyes[index].frame!=frame)return false;
        view=eyes[index].view;projection=eyes[index].projection;
#ifdef G3VR_NATIVE_MOTION
        if(native){stamp=native->CurrentStamp(index);return stamp.frame==frame;}
#endif
        return false;
    }
    bool Ready(uint64_t frame)const {return eyes[0].complete&&eyes[1].complete&&eyes[0].frame==frame&&eyes[1].frame==frame&&eyes[0].queries&&eyes[1].queries&&eyes[0].matrices&&eyes[1].matrices;}
    unsigned Queries(unsigned eye)const{return eye<2?eyes[eye].queries:0;}
    bool Observed(unsigned eye,uint64_t frame)const {return eye<2&&eyes[eye].complete&&eyes[eye].frame==frame&&eyes[eye].queries&&eyes[eye].matrices;}
    void Observe(IDirect3DDevice9* d) noexcept {
        if(!Active()||!d)return;auto& e=eyes[unsigned(current)];unsigned draw=e.draws++;
        if(!metadata){e.queries=1;return;}
        // First draw and every 32nd draw, with a hard 128-query cap per eye.
        if(draw%32||e.queries>=128)return;++e.queries;
        Pass sample;sample.drawIndex=draw;MicrosoftSurface target,depth;
        auto targetResult=d->GetRenderTarget(0,&target),depthResult=d->GetDepthStencilSurface(&depth);
        sample.target.Read(target.Get(),targetResult);sample.depth.Read(depth.Get(),depthResult);
        d->GetViewport(&sample.viewport);d->GetRenderState(D3DRS_ZENABLE,&sample.z);d->GetRenderState(D3DRS_ZWRITEENABLE,&sample.zwrite);d->GetRenderState(D3DRS_ALPHABLENDENABLE,&sample.alpha);
        d->GetRenderState(D3DRS_COLORWRITEENABLE,&sample.colorWrite);
        Microsoft::WRL::ComPtr<IDirect3DVertexShader9> vs;Microsoft::WRL::ComPtr<IDirect3DPixelShader9> ps;
        d->GetVertexShader(&vs);d->GetPixelShader(&ps);sample.vertexShader=reinterpret_cast<uintptr_t>(vs.Get());sample.pixelShader=reinterpret_cast<uintptr_t>(ps.Get());
        bool seen=false;for(unsigned i=0;i<e.passes;++i){auto const& p=e.pass[i];if(p.target.identity==sample.target.identity&&p.depth.identity==sample.depth.identity&&p.viewport.Width==sample.viewport.Width&&p.viewport.Height==sample.viewport.Height&&p.z==sample.z&&p.zwrite==sample.zwrite&&p.colorWrite==sample.colorWrite){seen=true;break;}}
        if(!seen&&e.passes<e.pass.size())e.pass[e.passes++]=sample;
        // Include visible depth-tested color passes even when the prepass has
        // already written depth. Keep postprocessing with depth disabled out.
        bool geometry=sample.z&&(sample.zwrite||(sample.colorWrite&7)==7)&&SUCCEEDED(sample.depth.result)&&sample.depth.desc.Width==e.width&&sample.depth.desc.Height==e.height&&sample.viewport.Width==e.width&&sample.viewport.Height==e.height;
        if(geometry){
            auto wanted=[&](uintptr_t identity,bool vertex){if(!identity||!e.shaders||e.shaderCount>=e.shaders->size())return false;for(unsigned i=0;i<e.shaderCount;++i)if((*e.shaders)[i].identity==identity&&(*e.shaders)[i].vertex==vertex)return false;return true;};
            if(wanted(sample.vertexShader,true)){auto& s=(*e.shaders)[e.shaderCount];CaptureShader(s,vs.Get(),true,d);if(s.bytes)++e.shaderCount;}
            if(wanted(sample.pixelShader,false)){auto& s=(*e.shaders)[e.shaderCount];CaptureShader(s,ps.Get(),false,d);if(s.bytes)++e.shaderCount;}
        }
    }
    void Write(std::ostream& out,unsigned eye,std::filesystem::path const& directory,std::string const& prefix="temporal",bool pixels=true)const {
        if(eye>1)return;auto const& e=eyes[eye];out<<std::setprecision(9)<<"{\"type\":\"vr_temporal_probe_eye\",\"xr_frame_serial\":"<<e.frame<<",\"eye\":"<<eye<<",\"metadata_only\":true,\"width\":"<<e.width<<",\"height\":"<<e.height<<",\"draws\":"<<e.draws<<",\"sampled_draw_states\":"<<e.queries<<",\"matrix_order\":\"SDK row-major\",\"matrices_valid\":"<<(e.matrices?"true":"false")<<",\"view\":";Floats(out,e.view,16);out<<",\"projection\":";Floats(out,e.projection,16);out<<",\"passes\":[";
        for(unsigned i=0;i<e.passes;++i){if(i)out<<',';auto const& p=e.pass[i];out<<"{\"draw_index\":"<<p.drawIndex<<",\"target\":";WriteSurface(out,p.target);out<<",\"depth\":";WriteSurface(out,p.depth);out<<",\"viewport\":["<<p.viewport.X<<','<<p.viewport.Y<<','<<p.viewport.Width<<','<<p.viewport.Height<<"],\"viewport_depth_range\":["<<p.viewport.MinZ<<','<<p.viewport.MaxZ<<"],\"z_enable\":"<<p.z<<",\"z_write\":"<<p.zwrite<<",\"alpha_blend\":"<<p.alpha<<",\"color_write_mask\":"<<p.colorWrite<<",\"vertex_shader_identity\":"<<p.vertexShader<<",\"pixel_shader_identity\":"<<p.pixelShader<<'}';}
        out<<"],\"geometry_shaders\":[";
        for(unsigned i=0;e.shaders&&i<e.shaderCount;++i){if(i)out<<',';auto const& s=(*e.shaders)[i];std::ostringstream name;name<<"temporal-"<<(s.vertex?"vs-":"ps-")<<std::hex<<s.hash<<".cso";
            std::ofstream file(directory/name.str(),std::ios::binary);file.write(reinterpret_cast<char const*>(s.code.data()),s.bytes);file.close();
            out<<"{\"stage\":\""<<(s.vertex?"vertex":"pixel")<<"\",\"identity\":"<<s.identity<<",\"bytes\":"<<s.bytes<<",\"bytecode_file\":\""<<name.str()<<"\",\"file_written\":"<<(file?"true":"false")<<",\"constants_hresult\":"<<int32_t(s.constantsResult);
            if(s.vertex&&SUCCEEDED(s.constantsResult)){out<<",\"constant_registers_c0_c255\":";Floats(out,s.constants.data(),unsigned(s.constants.size()));}out<<'}';
        }
        out<<"],\"xr_display_time_ns\":"<<e.displayTime<<",\"animated_motion_history\":";if(motion)motion->Write(out,eye);else out<<"null";
        out<<",\"raster_jitter_pixels\":["<<e.jitter[0]<<','<<e.jitter[1]<<"],\"geometry_motion_jitter_removed_by_native_companions\":true";
        out<<",\"motion_raster_snapshot\":";bool raster=false;if(pixels&&motion)raster=motion->WriteRaster(out,eye,directory,prefix);else out<<"null";
#ifdef G3VR_NATIVE_MOTION
        out<<",\"native_geometry_motion\":";if(native)native->Write(out,eye,directory,prefix,pixels);else out<<"null";
#endif
#ifdef G3VR_NATIVE_SKY
        out<<",\"late_sky_motion\":";if(sky)sky->Write(out,eye,directory,prefix,pixels);else out<<"null";
#endif
        out<<",\"depth_pixels_captured\":false,\"motion_vectors_captured\":"<<(raster?"true":"false")<<",\"note\":\"Bounded supported skin geometry only; not complete scene temporal inputs or DLAA.\"}";
    }
};
}
