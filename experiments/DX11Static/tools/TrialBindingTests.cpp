#include "NativeTrialBinding.h"
#include "NativeTrialSelection.h"
#include "NativeStereoProof.h"
#include "../../../src/NativeTrialMode.h"
#include <iostream>
#include <limits>
using namespace g3vr::dx11;
using namespace DirectX;
void Need(bool b,char const* text){if(!b)throw std::runtime_error(text);}
template<class F>void Reject(F&& fn){bool bad=false;try{fn();}catch(std::exception const&){bad=true;}Need(bad,"Unsupported camera accepted");}
int main(){try{
    NativeStereoProof pair;Need(!pair.Allows(1),"Unproven stereo pair accepted");
    pair.Record(0,1,true);Need(!pair.Allows(2),"One-eye approval enabled stereo replacement");
    pair.Record(1,1,true);Need(!pair.Allows(1)&&pair.Allows(2),"Stereo proof activated halfway through its frame");
    pair.Invalidate();Need(!pair.Allows(3),"Invalidated stereo proof remained active");
    for(unsigned failedEye=0;failedEye<2;++failedEye){NativeStereoProof rejected;rejected.Record(failedEye,5,false);rejected.Record(failedEye^1,5,true);Need(!rejected.Allows(6),"Asymmetric proof omitted one native eye");rejected.Record(failedEye,6,true);Need(!rejected.Allows(7),"Failed proof was silently re-enabled");}
    NativeStereoProof staggered;staggered.Record(1,8,true);staggered.Record(0,9,true);Need(!staggered.Allows(9)&&staggered.Allows(10),"Staggered eye approval activated mid-pair");
    for(unsigned i=1;i<=5;++i){wchar_t value[2]{wchar_t(L'0'+i),0};Need(diagnostic::NativeTrialMode(value,1)==i,"Native/runtime trial mode mismatch");Need(diagnostic::NativeTrialMode(value,1,true)==(i==5?5u:0u),"Validation build accepted a legacy replacement/profiling mode");}
    for(auto value:{L"",L"0",L"6",L"-1",L"04",L"3extra"})Need(diagnostic::NativeTrialMode(value,unsigned(wcslen(value)))==0,"Malformed trial mode enabled");
    wchar_t unterminatedMode[2]{L'4',L'x'};Need(!diagnostic::NativeTrialMode(unterminatedMode,1)&&!diagnostic::NativeTrialMode(nullptr,1),"Truncated/null mode enabled");
    Need(NativeResourceKey("G3_Myrtana_City_Ardea_01\\G3_Myrtana_Ardea_House_2_Story_01.xcmsh")=="g3_myrtana_ardea_house_2_story_01.g3mesh","Native resource did not match imported resource");
    char unterminated[1024];memset(unterminated,'x',sizeof(unterminated));Need(NativeResourceKey(unterminated).empty()&&NativeResourceKey(nullptr).empty(),"Unbounded native string accepted");
    auto view=XMMatrixLookAtLH(XMVectorSet(3500,700,-2800,1),XMVectorSet(4000,400,0,1),XMVectorSet(0,1,0,0));auto projection=XMMatrixPerspectiveFovLH(XMConvertToRadians(70),1.8f,5,500000);
    XMFLOAT4X4 v,p;XMStoreFloat4x4(&v,view);XMStoreFloat4x4(&p,projection);auto camera=NativeCameraMeters(reinterpret_cast<float*>(&v),reinterpret_cast<float*>(&p));
    for(auto point:{XMVectorSet(4000,400,100,1),XMVectorSet(3600,800,-1500,1),XMVectorSet(-5000,200,8000,1)}){
        auto native=XMVector3TransformCoord(point,view*projection);auto meters=XMVectorSet(XMVectorGetX(point)*.01f,XMVectorGetY(point)*.01f,XMVectorGetZ(point)*.01f,1);auto converted=XMVector3TransformCoord(meters,XMLoadFloat4x4(&camera.viewProjection));
        Need(XMVectorGetX(XMVector3Length(native-converted))<.00002f,"Centimeter camera conversion changed NDC coordinates");}
    auto signature=NativeCameraSignature(camera);Need(signature&&signature==NativeCameraSignature(NativeCameraMeters(reinterpret_cast<float*>(&v),reinterpret_cast<float*>(&p))),"Unstable native camera stamp");
    auto raster=NativeRasterCameraMeters(reinterpret_cast<float*>(&v),reinterpret_cast<float*>(&p),3840,2160);
    auto point=XMVectorSet(40,4,1,1);auto a=XMVector3TransformCoord(point,XMLoadFloat4x4(&camera.viewProjection));auto b=XMVector3TransformCoord(point,XMLoadFloat4x4(&raster.viewProjection));
    Need(std::abs((XMVectorGetX(b)-XMVectorGetX(a))*3840/2-.5f)<.001f&&std::abs((XMVectorGetY(b)-XMVectorGetY(a))*2160/2+.5f)<.001f,"Raster pixel-center correction changed sign/scale");
    Need(std::abs(XMVectorGetZ(a)-XMVectorGetZ(b))<1e-7f,"Raster correction changed depth");
    Reject([&]{NativeRasterCameraMeters(reinterpret_cast<float*>(&v),reinterpret_cast<float*>(&p),0,2160);});
    XMFLOAT4X4 farProjection=p;farProjection._43*=2;Need(!SameNativePass(reinterpret_cast<float*>(&p),reinterpret_cast<float*>(&farProjection)),"Lowpoly projection change was ignored");
    auto reverse=p;reverse._43=5;Reject([&]{NativeCameraMeters(reinterpret_cast<float*>(&v),reinterpret_cast<float*>(&reverse));});auto corrupt=v;corrupt._11=std::numeric_limits<float>::quiet_NaN();Reject([&]{NativeCameraMeters(reinterpret_cast<float*>(&corrupt),reinterpret_cast<float*>(&p));});
    NativeStaticSnapshot s{};Need(!ValidNativeIdentity(s),"Empty native identity accepted");s.identity[0]=1;Need(ValidNativeIdentity(s),"Valid native identity refused");s.materialSwitch=1;Need(!ValidNativeIdentity(s),"Unresolved material variant accepted");
    NativeBounds bounds{{-1,-1,-1},{1,1,1}};NativeStaticSnapshot central{};central.identity[0]=1;central.worldMeters[0]=central.worldMeters[5]=central.worldMeters[10]=central.worldMeters[15]=1;central.worldMeters[14]=10;
    XMFLOAT4X4 identity,selectionProjection;XMStoreFloat4x4(&identity,XMMatrixIdentity());XMStoreFloat4x4(&selectionProjection,XMMatrixPerspectiveFovLH(XMConvertToRadians(70),1.8f,1,100));
    Camera simple;simple.viewProjection=selectionProjection;float centralScore=NativeCandidateScore(bounds,central,simple);Need(centralScore>0,"Central candidate rejected");
    auto edge=central;edge.worldMeters[12]=100;Need(NativeCandidateScore(bounds,edge,simple)==0,"Offscreen candidate admitted");auto behind=central;behind.worldMeters[14]=-10;Need(NativeCandidateScore(bounds,behind,simple)==0,"Behind-camera candidate admitted");
    NativeTrialSelection picker;picker.Offer(11,central,simple,reinterpret_cast<float*>(&identity),reinterpret_cast<float*>(&selectionProjection),centralScore);picker.Offer(22,central,simple,reinterpret_cast<float*>(&identity),reinterpret_cast<float*>(&selectionProjection),centralScore/2);
    Need(picker.Best(reinterpret_cast<float*>(&identity),reinterpret_cast<float*>(&selectionProjection))->cookie==11,"First/last callback overrode better central candidate");
    auto different=selectionProjection;different._43*=2;Need(!picker.Best(reinterpret_cast<float*>(&identity),reinterpret_cast<float*>(&different)),"Candidate crossed projection passes");picker.Reset();Need(!picker.Best(reinterpret_cast<float*>(&identity),reinterpret_cast<float*>(&selectionProjection)),"Candidate survived eye reset");
    for(unsigned i=0;i<4096;++i)Need(picker.Probe(),"Candidate query budget too short");Need(!picker.Probe(),"Candidate query budget not bounded");
    std::cout<<"PASS real/native resource identity, centimeter camera conversion, pass isolation, malformed inputs\n";return 0;
}catch(std::exception const& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
