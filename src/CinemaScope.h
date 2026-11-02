#pragma once
#include <cstdint>
namespace diagnostic {
class CinemaScope {
    uint32_t thread=0;unsigned depth=0;bool capturing=false;
public:
    bool Begin(uint32_t id){if(depth&&id!=thread)return false;thread=id;++depth;return true;}
    void End(uint32_t id){if(id==thread&&depth&&!--depth){thread=0;capturing=false;}}
    bool Active(uint32_t id)const{return depth&&thread==id;}
    bool Capture(uint32_t id){if(!Active(id)||capturing)return false;capturing=true;return true;}
    void Finish(){capturing=false;}
};
}
