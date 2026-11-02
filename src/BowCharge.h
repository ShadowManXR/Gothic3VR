#pragma once
#include <algorithm>
#include <cmath>
namespace diagnostic { namespace bow {
// CP PowerAim uses 0.2 + 0.8 * charge, and Shoot uses 200 * that
// multiplier. Read these constants from the verified installed script at runtime.
struct ChargeProfile {
    double seconds=0,base=0,gain=0,velocity=0;
    bool Valid()const{return std::isfinite(seconds)&&seconds>0&&std::isfinite(base)&&base>0&&std::isfinite(gain)&&gain>0&&std::isfinite(velocity)&&velocity>0;}
    float Multiplier(float power)const{return Valid()&&std::isfinite(power)?float(base+gain*std::clamp(power,0.f,1.f)):0;}
    float Velocity(float power)const{return float(velocity)*Multiplier(power);}
};
template<class Projectile,class Damage>
bool ApplyArrowCharge(Projectile& projectile,Damage& damage,ChargeProfile const& profile,float power){
    if(!profile.Valid()||!std::isfinite(power))return false;
    projectile.SetShootVelocity(profile.Velocity(power));damage.SetDamageHitMultiplier(profile.Multiplier(power));return true;
}
// The installed PowerAim duration itself is fixed. VR adds draw-speed
// progression: 1.4x / 2.1x; Master Archer follows physical draw without a timer. It does not
// grant skills, change native damage bonuses or modify the physical draw length.
inline double DrawSpeed(int tier){return (tier==2?1.5:1.)*1.4;}
inline double ChargeSeconds(int tier,double nativeSeconds){return tier>=3?0:nativeSeconds/DrawSpeed(tier);}
inline float VisibleDraw(float power,float maximum){return std::isfinite(power)&&std::isfinite(maximum)&&maximum>0?std::clamp(power,0.f,1.f)*maximum:0.f;}
class Charge {
    double available=0;
public:
    void Reset(){available=0;}
    float Step(float physical,double scaledSeconds,double fullChargeSeconds){
        if(!std::isfinite(physical)||!std::isfinite(scaledSeconds)||scaledSeconds<0||!std::isfinite(fullChargeSeconds)||fullChargeSeconds<0){Reset();return 0;}
        // Zero duration is the geometry-only path used without a native profile.
        if(fullChargeSeconds==0)return std::clamp(physical,0.f,1.f);
        available=std::min(1.,available+scaledSeconds/fullChargeSeconds);
        return std::min(std::clamp(physical,0.f,1.f),float(available));
    }
};
} }
