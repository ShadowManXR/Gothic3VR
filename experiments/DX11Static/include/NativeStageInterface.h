#pragma once
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <wrl/client.h>
#include <cstring>
namespace g3vr::dx11 {
// SM3 semantic remapping is not a substitute for an SM5 stage interface.
// Preserve register positions even for intervening unused native varyings.
inline bool NativeStageInterfaceCompatible(ID3DBlob* vertex,ID3DBlob* pixel)noexcept{
    if(!vertex||!pixel)return false;
    Microsoft::WRL::ComPtr<ID3D11ShaderReflection> vs,ps;
    if(FAILED(D3DReflect(vertex->GetBufferPointer(),vertex->GetBufferSize(),IID_PPV_ARGS(&vs)))||
       FAILED(D3DReflect(pixel->GetBufferPointer(),pixel->GetBufferSize(),IID_PPV_ARGS(&ps))))return false;
    D3D11_SHADER_DESC vd{},pd{};if(FAILED(vs->GetDesc(&vd))||FAILED(ps->GetDesc(&pd))||vd.OutputParameters>32||pd.InputParameters>32)return false;
    for(unsigned i=0;i<pd.InputParameters;++i){D3D11_SIGNATURE_PARAMETER_DESC input{};if(FAILED(ps->GetInputParameterDesc(i,&input)))return false;if(!input.ReadWriteMask)continue;bool found=false;
        for(unsigned j=0;j<vd.OutputParameters;++j){D3D11_SIGNATURE_PARAMETER_DESC output{};if(FAILED(vs->GetOutputParameterDesc(j,&output)))return false;
            if(_stricmp(output.SemanticName,input.SemanticName)||output.SemanticIndex!=input.SemanticIndex)continue;
            if(found||output.Register!=input.Register||output.ComponentType!=input.ComponentType||(output.Mask&input.ReadWriteMask)!=input.ReadWriteMask)return false;found=true;}
        if(!found)return false;
    }return true;
}
}
