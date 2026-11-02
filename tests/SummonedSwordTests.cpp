#include "HeldWeaponVisuals.h"
#include "LockedMeleeAttachment.h"
#include "RangedVisualScope.h"
#include "DualWield.h"
#include "SummonedSwordPose.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
struct Node {bool live=true,visible=false;float alpha=0;Node* child=nullptr;};
struct Access {
    using Entity=Node;
    bool Valid(Node* n)const{return n&&n->live;}
    bool Visible(Node* n)const{return n->visible;}
    float Alpha(Node* n)const{return n->alpha;}
    int Children(Node* n)const{return n->child?1:0;}
    Node* Child(Node* n,int)const{return n->child;}
    void SetVisible(Node* n,bool v)const{n->visible=v;if(n->child)SetVisible(n->child,v);}
    void SetAlpha(Node* n,float a)const{n->alpha=a;}
};
bool Same(arms::Mat const& a,arms::Mat const& b){for(unsigned i=0;i<16;++i)if(std::fabs(a.m[i]-b.m[i])>.001f)return false;return true;}
int main(){
    CHECK(visual::SummonedFlameSword("It_1H_MagicFlameSword_01",2));
    CHECK(visual::SummonedFlameSword("it_1h_magicflamesword_01",2));
    for(auto name:{"It_1H_FlameSword_01","It_Scroll_Flamesword","Spl_Flamesword","VFX_Magic_Weapons_FlameSword_01","It_1H_Katana_01"})CHECK(!visual::SummonedFlameSword(name,2));
    CHECK(!visual::SummonedFlameSword(nullptr,2)&&!visual::SummonedFlameSword("It_1H_MagicFlameSword_01",15));
    // Regression: .83 accepted a 68.4 cm transient native offset and kept it
    // permanently. Bind the actual handle in hand-local space, so neither
    // spawn translation, controller rotation nor calibrated wrist can orbit it.
    for(float units:{50.f,100.f,200.f})for(unsigned hand=0;hand<2;++hand){
        float anglesNative[3]{.7f,-.9f,.35f};auto calibration=arms::Rotation(NativeCameraRotation(anglesNative));
        auto fixed=calibration;arms::Position(fixed,{.15f*units,-.2f*units,.6364723f*units});
        int angles[3]{-45,15,12};arms::Vec palm{.012f*units,-.045f*units,.01f*units};
        melee::Blade blade{{0,0,10.f*units/100},{0,0,.9056f*units}};
        for(unsigned frame=0;frame<240;++frame){
            float a[3]{float(frame)*.031f,float(frame)*-.024f,float(frame)*.017f};
            auto grip=arms::Rotation(NativeCameraRotation(a));
            arms::Position(grip,{1000*units+frame*.1f*units,1.4f*units,-650*units});
            melee::OneHandPose pose;CHECK(pose.PrepareAttachment(fixed,calibration,blade,grip,angles,units,palm));
            auto before=pose;CHECK(melee::AnchorSummonedSword("It_1H_MagicFlameSword_01",2,hand,pose));
            CHECK(Same(before.grip,pose.grip)&&Same(before.calibration,pose.calibration));
            for(unsigned i:{0u,1u,2u,4u,5u,6u,8u,9u,10u})CHECK(before.attachment.m[i]==pose.attachment.m[i]);
            arms::Vec contact;CHECK(melee::SupportFistContact(calibration,palm,hand,units,contact));
            for(unsigned eye=0;eye<2;++eye){
                auto wrist=arms::WristAtPalm(grip,calibration,palm);
                auto desiredHand=arms::Multiply(calibration,grip);arms::Position(desiredHand,wrist);
                auto mesh=pose.AtWrist(wrist);auto handle=arms::Point(mesh,melee::SummonedSwordHandle(units));
                CHECK(arms::Length(handle-arms::Point(desiredHand,contact))<.001f*units);
                CHECK(arms::Length(arms::Point(mesh,blade.tip)-handle)>.9f*units);
            }
            for(auto name:{"It_1H_FlameSword_01","It_1H_Katana_01","It_Scroll_Flamesword"}){
                auto ordinary=before;CHECK(melee::AnchorSummonedSword(name,2,hand,ordinary));
                CHECK(!memcmp(&ordinary,&before,sizeof(before)));
            }
        }
    }
    for(float units:{50.f,100.f,200.f}){
        melee::LockedMeleeAttachment binding;arms::Mat hand,spawn;
        arms::Position(hand,{1000*units,1.4f*units,-650*units}); // far from the newly created entity's origin
        for(unsigned i=0;i<3;++i)CHECK(!binding.Capture(101,2,spawn,hand,.75f*units)&&!binding.Matches(101,2));
        auto attached=hand;arms::Position(attached,arms::Position(hand)+arms::Vec{.02f*units,-.03f*units,.04f*units});
        CHECK(binding.Capture(101,2,attached,hand,.75f*units));auto fixed=binding.Attachment();
        // A later native cast/draw/carry animation must not recalibrate the
        // established rigid grip, even if it momentarily moves the item away.
        for(unsigned frame=0;frame<200;++frame){CHECK(binding.Capture(101,2,spawn,hand,.75f*units));CHECK(Same(fixed,binding.Attachment()));}
        CHECK(!binding.Capture(102,2,spawn,hand,.75f*units)&&!binding.Matches(102,2));
        CHECK(binding.Capture(102,2,attached,hand,.75f*units));
        melee::Blade blade{{0,0,0},{0,0,units}};arms::Mat grip;arms::Position(grip,{21,145,47});int angles[3]{};
        melee::OneHandPose tracked;CHECK(tracked.PrepareAttachment(binding.Attachment(),arms::Mat{},blade,grip,angles,units,{0,-.04f*units,0}));
        CHECK(arms::Length(arms::Position(tracked.mesh)-arms::Position(grip))<.2f*units);
        // Existing weapon classes still use their original, unrestricted
        // native attachment capture. No global grip or world transform change.
        for(int type:{2,3,12,51,52}){melee::LockedMeleeAttachment ordinary;CHECK(ordinary.Capture(42,type,spawn,hand));}
    }
    Node blade,flame;blade.child=&flame;flame.visible=true;flame.alpha=.35f;
    ranged::VisualScope<Access> visibility;
    // The conjured inventory instance need not yet be a descendant of the
    // hero. Its own tree gets reversible visibility before each eye's culling.
    for(unsigned frame=0;frame<200;++frame)for(unsigned eye=0;eye<2;++eye){
        CHECK(!visibility.Active()&&visibility.Capture(&blade)&&visibility.Active());
        for(unsigned pass=0;pass<4;++pass){visibility.Show();CHECK(blade.visible&&flame.visible&&blade.alpha==1&&flame.alpha==1);}
        visibility.Restore(true);CHECK(!visibility.Active());CHECK(!blade.visible&&blade.alpha==0&&flame.visible&&flame.alpha==.35f);
    }
    CHECK(visibility.Capture(&blade));visibility.Show();blade.live=false;visibility.Restore(false);CHECK(!visibility.Active());
    blade.live=true;blade.visible=false;blade.alpha=0;CHECK(visibility.Capture(&blade));visibility.Abandon();CHECK(!visibility.Active());
    puts("PASS 68.4cm spawn-offset regression, asset handle bound to calibrated fist at three scales through 240 wide rotations and both eyes, orientation/angles unchanged, exact ordinary-pose preservation, late attachment and reversible independent visibility");return 0;
}
