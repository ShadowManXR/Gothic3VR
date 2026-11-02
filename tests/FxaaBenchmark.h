#pragma once
// Offline GPU timestamps only. Never linked into the game/plugin.
inline void FxaaBenchmark(ID3D11Device* device,ID3D11DeviceContext* context){
    constexpr unsigned width=3840,height=2160,frames=32;
    ComPtr<ID3D11Texture2D> inputs[2],outputs[2];
    D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    for(auto& image:inputs)Hr(device->CreateTexture2D(&desc,nullptr,&image));
    desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;desc.BindFlags=D3D11_BIND_RENDER_TARGET;for(auto& image:outputs)Hr(device->CreateTexture2D(&desc,nullptr,&image));
    auto wait=[&](ID3D11Asynchronous* query,void* result,unsigned size){auto started=GetTickCount64();HRESULT hr;while((hr=context->GetData(query,result,size,D3D11_ASYNC_GETDATA_DONOTFLUSH))==S_FALSE){Require(GetTickCount64()-started<10000,"GPU timing timeout");Sleep(1);}Hr(hr);};
    for(bool edges:{false,true}){
        for(unsigned eye=0;eye<2;++eye){std::vector<uint32_t> pixels(width*height);
            for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){unsigned v=edges?((x/9+y/9)%2?210:35):80+eye*25;pixels[y*width+x]=0xff000000|(v<<16)|(v<<8)|v;}
            context->UpdateSubresource(inputs[eye].Get(),0,nullptr,pixels.data(),width*4,0);
        }
        double times[5]{};
        for(unsigned aa=0;aa<5;++aa){GpuUpscaler filter;filter.Initialize(device,MakeUpscalePlan(width,height,UpscaleMethod::Off,100,0,true,aa==1,aa>=2,aa>=2?int(aa)-2:0));
            for(unsigned warm=0;warm<8;++warm)for(unsigned eye=0;eye<2;++eye)filter.Process(context,outputs[eye].Get(),inputs[eye].Get());
            D3D11_QUERY_DESC q{D3D11_QUERY_TIMESTAMP_DISJOINT,0};ComPtr<ID3D11Query> disjoint,start,end;Hr(device->CreateQuery(&q,&disjoint));q.Query=D3D11_QUERY_TIMESTAMP;Hr(device->CreateQuery(&q,&start));Hr(device->CreateQuery(&q,&end));
            context->Begin(disjoint.Get());context->End(start.Get());for(unsigned frame=0;frame<frames;++frame)for(unsigned eye=0;eye<2;++eye)filter.Process(context,outputs[eye].Get(),inputs[eye].Get());context->End(end.Get());context->End(disjoint.Get());context->Flush();
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT clock{};uint64_t a=0,b=0;wait(disjoint.Get(),&clock,sizeof(clock));wait(start.Get(),&a,sizeof(a));wait(end.Get(),&b,sizeof(b));Require(!clock.Disjoint&&clock.Frequency>0,"Disjoint GPU timing");times[aa]=1000.*double(b-a)/double(clock.Frequency)/frames;
        }
        std::printf("{\"type\":\"fxaa_offline_gpu_benchmark\",\"pattern\":\"%s\",\"width_per_eye\":%u,\"height_per_eye\":%u,\"eyes\":2,\"frames\":%u,\"copy_gpu_ms\":%.4f,\"fxaa_gpu_ms\":%.4f,\"smaa_gpu_ms\":%.4f,\"smaa_high_gpu_ms\":%.4f,\"smaa_ultra_gpu_ms\":%.4f,\"smaa_additional_gpu_ms\":%.4f,\"additional_gpu_ms\":%.4f,\"headset_fps_measurement\":false}\n",edges?"dense_checker_edges":"flat",width,height,frames,times[0],times[1],times[2],times[3],times[4],times[2]-times[0],times[1]-times[0]);
    }
}
