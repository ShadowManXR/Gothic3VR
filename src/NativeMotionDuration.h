#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <cctype>

namespace diagnostic { namespace crossbow {
inline bool MotionAliasMatch(std::string alias,std::string file){
    auto base=[](std::string s){auto p=s.find_last_of("/\\");if(p!=std::string::npos)s=s.substr(p+1);for(auto& c:s)c=char(std::tolower(static_cast<unsigned char>(c)));return s;};
    alias=base(alias);file=base(file);
    if(alias.empty()||file.size()<5||file.substr(file.size()-5)!=".xmot")return false;
    if(alias.size()>5&&alias.substr(alias.size()-5)==".xmot")return file==alias;
    return file.compare(0,alias.size(),alias)==0&&(file.size()==alias.size()+5||file[alias.size()]=='%'||file[alias.size()]=='_');
}
// EMotionFX's LMA duration is the latest key in the motion, in seconds.
// Parse the selected native hero Cock motion, not the weapon/string motion.
inline bool MotionDuration(std::vector<unsigned char> const& bytes,double& seconds){
    seconds=0;size_t magic=bytes.size();
    for(size_t i=0;i+7<=bytes.size()&&i<128;++i)if(!memcmp(bytes.data()+i,"LMA ",4)){magic=i;break;}
    if(magic==bytes.size()||bytes[magic+4]!=1||bytes[magic+5]!=1)return false;
    auto read=[&](size_t p,uint32_t& v){if(p>bytes.size()||bytes.size()-p<4)return false;memcpy(&v,bytes.data()+p,4);return true;};
    size_t end=bytes.size(),at=magic+7;
    if(bytes.size()>=8&&!memcmp(bytes.data(),"GENOMFLE",8)){
        uint32_t length;if(magic<4||!read(magic-4,length)||length<7||length>bytes.size()-magic)return false;end=magic+length;
    }
    bool node=false,keys=false;
    while(at+12<=end){
        uint32_t id,n,version;read(at,id);read(at+4,n);read(at+8,version);at+=12;if(n>end-at)return false;
        if(id==1){if(version!=3||n<84)return false;node=true;}
        else if(id==2||id==3||id==4){
            uint32_t count,kind;
            if(!node||version!=1||n<8||!read(at,count)||!read(at+4,kind)||!count||count>65536)return false;
            // Compiled hero motions use chunk 2 for both LR quaternion and
            // LP vector tracks. The track format, not its chunk ID, fixes stride.
            size_t stride=kind==0x524c?20:16;
            if((kind!=0x524c&&kind!=0x504c&&kind!=0x534c)||size_t(count)*stride+8!=n)return false;
            float previous=-1;
            for(uint32_t i=0;i<count;++i){float time;memcpy(&time,bytes.data()+at+8+size_t(i)*stride,4);
                if(!std::isfinite(time)||time<0||time<=previous||time>120)return false;previous=time;seconds=std::max(seconds,double(time));}
            keys=true;
        }else return false; // Unknown motion formats must not silently shorten reload.
        at+=n;
    }
    return at==end&&keys&&seconds>0;
}
class Reload {
    double remaining=0;
public:
    bool Ready()const{return remaining<=.000001;}
    double Remaining()const{return remaining;}
    bool Commit(double motionSeconds,double nativeSpeed){
        if(!std::isfinite(motionSeconds)||motionSeconds<=0||!std::isfinite(nativeSpeed)||nativeSpeed<=0)return false;
        remaining=motionSeconds/nativeSpeed;return std::isfinite(remaining);
    }
    void Advance(double scaledSeconds,bool paused){
        if(!paused&&std::isfinite(scaledSeconds)&&scaledSeconds>0)remaining=std::max(0.,remaining-scaledSeconds);
    }
};
} }
