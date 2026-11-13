#include "GuardPose.h"
#include "PressOnce.h"
#include "InteractionTargetBox.h"
#include "ParticleAttachment.h"
#include "MagicHandEffects.h"
#include "ControllerControls.h"
#include "MeleeSpecialActions.h"
#include "ShieldPose.h"
#include <cstdio>
#include <vector>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
struct ParticleNode {bool valid=true,projectile=false;uintptr_t particle=0;ParticleNode* parent=nullptr;arms::Mat world;std::vector<ParticleNode*> children;};
struct ParticleTree {
    using Entity=ParticleNode;
    bool Valid(Entity* p)const{return p&&p->valid;}
    Entity* Parent(Entity* p)const{return p->parent;}
    int Children(Entity* p)const{return int(p->children.size());}
    Entity* Child(Entity* p,int i)const{return p->children[i];}
    bool Projectile(Entity* p)const{return p->projectile;}
    uintptr_t PrivateParticle(Entity* p)const{return p->particle;}
    arms::Mat World(Entity* p)const{return p->world;}
};
struct HandEffectNode {arms::Mat local,world;bool valid=true;unsigned writes=0;};
struct HandEffectTree {
    using Entity=HandEffectNode;
    bool Valid(Entity* e)const{return e&&e->valid;}
    arms::Mat Local(Entity* e)const{return e->local;}
    arms::Mat World(Entity* e)const{return e->world;}
    void SetLocal(Entity* e,arms::Mat const& m)const{e->local=e->world=m;++e->writes;}
    void SetWorld(Entity* e,arms::Mat const& m)const{e->world=m;++e->writes;}
};
int main(){
    ControllerFrame frame;frame.focused=true;frame.timeMs=1000;
    for(auto& h:frame.hands){h.active=true;h.grip.valid=true;}
    GuardPose guard;
    frame.hands[0].grip.position[1]=.9f;CHECK(!guard.Step(frame,0,1.7f,1000,true));
    frame.hands[0].grip.position[1]=1.3f;CHECK(guard.Step(frame,0,1.7f,1000,true));
    frame.hands[0].grip.position[1]=1.2f;CHECK(guard.Step(frame,0,1.7f,1000,true));
    frame.hands[0].grip.position[1]=1.1f;CHECK(!guard.Step(frame,0,1.7f,1000,true));
    frame.hands[1].grip.position[1]=.5f;CHECK(guard.Step(frame,1,.9f,1000,true)); // seated
    CHECK(!guard.Step(frame,1,.9f,1101,true));CHECK(!guard.Step(frame,1,.9f,1000,false));
    frame.hands[1].grip.valid=false;CHECK(!guard.Step(frame,1,.9f,1000,true));
    PressOnce heal;CHECK(!heal.Step(1,true,true,1000));CHECK(!heal.Step(1,true,false,1010));CHECK(heal.Step(1,true,true,1020));
    for(unsigned i=0;i<100;++i)CHECK(!heal.Step(1,true,true,1030+i));
    CHECK(!heal.Step(1,true,false,1140)&&heal.Step(1,true,true,1150));
    CHECK(!heal.Step(1,false,true,1160)&&!heal.Step(1,true,true,1170));
    CHECK(!heal.Step(2,true,true,1180)); // a new save cannot consume a held trigger
    float root[3]{100,200,300},lo[3]{-10000,-10000,-10000},hi[3]{10000,10000,10000},reach=250;
    CampfireInteractionBox(88,root,100,lo,hi);CHECK(lo[0]==45&&hi[0]==155&&lo[2]==245&&hi[2]==355&&reach==250);
    float unchanged=hi[0];CampfireInteractionBox(2,root,100,lo,hi);CHECK(hi[0]==unchanged);
    WorldAim aim;aim.origin[0]=100;aim.origin[1]=230;aim.origin[2]=0;aim.direction[2]=1;float rating=0;
    CHECK(!WorldAimBox(aim,lo,hi,100,100,rating));aim.origin[2]=200;CHECK(WorldAimBox(aim,lo,hi,reach,100,rating));
    float eye[3]{0,170,0},forward[3]{0,0,1},sign[3]{0,170,500};
    CHECK(SignpostName("Sign_Ardea")&&!SignpostName("Signboard_01_Inn")&&!SignpostName("NPC_Ardea"));
    // A world-label batch must never evaluate the expensive VR/menu extension
    // for ordinary entities, even when thousands of names are processed.
    unsigned signpostContextQueries=0;
    auto extension=[&](){++signpostContextQueries;return true;};
    char const* ordinaryLabels[]={"NPC_Orc_Elite","It_1H_Sword","G3_World_Mesh","Signboard_01_Inn","Sign_","",nullptr};
    for(unsigned i=0;i<10000;++i){
        for(char const* name:ordinaryLabels)
            CHECK(!ExtraSignpostFocus(name,extension));
    }
    CHECK(signpostContextQueries==0);
    CHECK(ExtraSignpostFocus("Sign_Ardea",extension)&&signpostContextQueries==1);
    CHECK(!ExtraSignpostFocus("Sign_Reddock",[&](){++signpostContextQueries;return false;})&&signpostContextQueries==2);
    CHECK(SignpostInView(eye,forward,sign,100));sign[2]=900;CHECK(!SignpostInView(eye,forward,sign,100));
    sign[2]=-100;CHECK(!SignpostInView(eye,forward,sign,100));sign[0]=400;sign[2]=400;CHECK(!SignpostInView(eye,forward,sign,100));
    visual::ParticleAttachmentFrame particles;arms::Mat native;arms::Position(native,{10,20,30});CHECK(particles.Add(123,native));
    float angles[3]{0,1.2f,0};auto desired=arms::Rotation(NativeCameraRotation(angles));arms::Position(desired,{60,70,80});arms::Mat delta;
    CHECK(particles.Delta(123,desired,delta));auto nativeParticle=arms::Point(native,{0,2,0}),rendered=arms::Point(delta,nativeParticle);
    CHECK(arms::Length(rendered-arms::Point(desired,{0,2,0}))<.001f);
    visual::ParticleWorldScope draw;CHECK(draw.Begin(delta)&&!draw.Begin(delta));arms::Mat identity,out;
    CHECK(draw.Rewrite(identity,out));CHECK(!draw.Rewrite(identity,out)); // native restoration is untouched
    draw.End();CHECK(!draw.Active()&&!draw.Rewrite(identity,out));CHECK(draw.Begin(delta)&&draw.Rewrite(identity,out));draw.End(); // second eye
    particles.Reset();CHECK(!particles.Contains(123)&&!particles.Delta(123,desired,delta));
    // A private emitter follows the held item, even when the engine has not
    // propagated the item's VR render transform down its native child tree.
    ParticleNode held,attached,projectile,projectileTail,unrelated;
    arms::Position(held.world,{10,20,30});attached.parent=projectile.parent=&held;
    attached.particle=201;projectile.projectile=true;projectile.particle=202;projectileTail.parent=&projectile;projectileTail.particle=203;
    held.children={&attached,&projectile};projectile.children={&projectileTail};unrelated.particle=204;
    CHECK(visual::CollectHeldParticles(ParticleTree{},&held,particles));
    CHECK(particles.Contains(201)&&!particles.Contains(202)&&!particles.Contains(203)&&!particles.Contains(204));
    CHECK(particles.Anchor(201)==reinterpret_cast<uintptr_t>(&held));
    CHECK(particles.Delta(201,desired,delta));
    CHECK(arms::Length(arms::Point(delta,arms::Point(held.world,{2,3,4}))-arms::Point(desired,{2,3,4}))<.001f);
    // Both eyes use the same captured native root; no accumulating displacement.
    for(unsigned eye=0;eye<2;++eye){CHECK(draw.Begin(delta)&&draw.Rewrite(identity,out));CHECK(!draw.Rewrite(identity,out));draw.End();}
    particles.Reset();
    CHECK(visual::CollectHeldParticles(ParticleTree{},&projectile,particles,true));
    CHECK(particles.Contains(202)&&particles.Contains(203)&&!particles.Contains(201));
    CHECK(particles.Anchor(203)==reinterpret_cast<uintptr_t>(&projectile));
    CHECK(particles.Delta(203,desired,delta));
    CHECK(arms::Length(arms::Point(delta,arms::Position(projectile.world))-arms::Position(desired))<.001f);
    particles.Reset();attached.children={&held};CHECK(!visual::CollectHeldParticles(ParticleTree{},&held,particles));attached.children.clear();
    particles.Reset();attached.parent=&unrelated;CHECK(!visual::CollectHeldParticles(ParticleTree{},&held,particles));attached.parent=&held;
    for(int type:{12,15,24,51})CHECK(visual::HeldParticleRoot(type,false));
    CHECK(visual::HeldParticleRoot(2,true)&&!visual::HeldParticleRoot(2,false));
    for(int type:{0,4,5,6,7,9,3,52})CHECK(!visual::HeldParticleRoot(type,false));
    particles.Reset();CHECK(!particles.Anchor(201)&&!particles.Contains(201));
    // All schools share the bone attachment path, not a Fire-only whitelist.
    for(char const* name:{"VFX_Magic_Ice_Lance_Hold_01","VFX_Magic_Lightning_Load_01","VFX_Magic_Heal_Hold_01","VFX_Magic_FireBall_01","vfx_magic_charm_01"})CHECK(visual::MagicHandEffect(name));
    CHECK(!visual::MagicHandEffect("It_Scroll_IceLance")&&!visual::MagicHandEffect("VFX_Env_Fire_01")&&!visual::MagicHandEffect(""));
    CHECK(visual::EffectHand("Hero_Left_Hand_Hand_1")==0&&visual::EffectHand("Hero_Right_Hand_Hand_1")==1);
    CHECK(visual::EffectHand("Hero_Head_Head_1")==-1&&visual::EffectHand("Hero_Spine_Spine_2")==-1);
    visual::HandEffectScope<HandEffectTree> effects;HandEffectNode healLeft,iceRight,worldEffect;
    arms::Position(healLeft.local,{10,20,30});healLeft.world=healLeft.local;
    arms::Position(iceRight.local,{-20,30,40});iceRight.world=iceRight.local;
    CHECK(effects.Capture(&healLeft,0)&&effects.Capture(&iceRight,1)&&effects.Capture(&healLeft,0)&&effects.Count()==2);
    CHECK(!effects.Capture(&healLeft,1)&&!effects.Capture(&worldEffect,2));
    arms::Mat hands[2];arms::Position(hands[0],{100,20,0});float yaw[3]{0,.7f,0};hands[1]=arms::Rotation(NativeCameraRotation(yaw));arms::Position(hands[1],{-50,0,90});
    bool handsReady[2]{true,true};auto leftBefore=healLeft.local,rightBefore=iceRight.local;
    auto expectedLeft=arms::Multiply(leftBefore,hands[0]),expectedRight=arms::Multiply(rightBefore,hands[1]);
    for(unsigned eye=0;eye<2;++eye){effects.Apply(hands,handsReady);
        CHECK(arms::Length(arms::Position(healLeft.world)-arms::Position(expectedLeft))<.001f);
        CHECK(arms::Length(arms::Position(iceRight.world)-arms::Position(expectedRight))<.001f);
        effects.Apply(hands,handsReady); // extra palette/draw cannot accumulate the hand delta
        CHECK(arms::Length(arms::Position(healLeft.world)-arms::Position(expectedLeft))<.001f);
        effects.Restore();CHECK(!memcmp(&healLeft.local,&leftBefore,sizeof(leftBefore))&&!memcmp(&iceRight.local,&rightBefore,sizeof(rightBefore)));
    }
    CHECK(worldEffect.writes==0);
    handsReady[1]=false;effects.Apply(hands,handsReady);CHECK(!memcmp(&iceRight.world,&rightBefore,sizeof(rightBefore)));effects.Reset();CHECK(!effects.Count());
    CHECK(!memcmp(&healLeft.world,&leftBefore,sizeof(leftBefore)));effects.Apply(hands,handsReady);CHECK(!effects.Count());
    ControllerTranslator input;frame={};frame.focused=true;for(auto& h:frame.hands)h.active=true;uint64_t t=2000;
    auto step=[&](){frame.timeMs=t+=10;return input.Translate(frame,t,true,false,2,1,2,0);};
    step();frame.hands[1].secondary=true;auto c=step();CHECK(c.held[Weapon]&&!c.held[Inventory]);frame.hands[1].secondary=false;step();
    frame.hands[0].primary=true;c=step();CHECK(c.held[Inventory]&&!c.held[Weapon]);frame.hands[0].primary=false;step();
    frame.hands[0].squeeze=1;c=step();CHECK(!c.quickHeal&&c.shieldGrip&&!c.held[Use2]);
    ParryLoadout shieldLoadout{2,9},crossbowLoadout{6,0},twoHandLoadout{3,0};
    CHECK(shieldLoadout.Pressed(c.held[Use2],c.shieldGrip));
    CHECK(!crossbowLoadout.Pressed(c.held[Use2],c.shieldGrip)&&!twoHandLoadout.Pressed(c.held[Use2],c.shieldGrip));
    frame.hands[0].grip.valid=true;frame.hands[0].grip.position[1]=.9f;
    CHECK(!(guard.Step(frame,0,1.7f,t,true)&&shieldLoadout.Pressed(c.held[Use2],c.shieldGrip))); // shield lowered
    frame.hands[0].grip.position[1]=1.3f;
    CHECK(guard.Step(frame,0,1.7f,t,true)&&shieldLoadout.Pressed(c.held[Use2],c.shieldGrip)); // shield raised
    frame.hands[0].trigger=1;c=step();CHECK(!c.quickHeal&&!c.held[Use2]);for(int i=0;i<4;++i)step();
    frame.hands[0].trigger=0;for(int i=0;i<4;++i)step();frame.hands[0].trigger=1;c=step();CHECK(c.quickHeal&&!c.held[Use2]&&frame.hands[0].squeeze==1);CHECK(!step().quickHeal);frame.hands[0].trigger=0;step();
    frame.hands[1].stickClick=true;c=step();CHECK(c.radial.visible&&!c.shieldGrip);frame.hands[1].stickClick=false;step();
    frame.hands[1].secondary=true;c=step();CHECK(c.radial.visible&&c.radial.page==1&&!c.held[Weapon]);frame.hands[1].secondary=false;step();
    frame.hands[0].secondary=true;c=step();CHECK(c.radial.visible&&c.radial.page==1&&!c.sneakToggleButton);frame.hands[0].secondary=false;step();
    frame.hands[0].primary=true;c=step();CHECK(!c.radial.visible&&c.captured&&!c.held[Inventory]);frame.hands[0].primary=false;step();
    frame.hands[0].stick[1]=1;step();frame.hands[0].stickClick=true;step();frame.hands[0].stickClick=false;c=step();CHECK(c.held[Sprint]&&c.held[Forward]&&!c.held[Backward]);
    frame.hands[0].trigger=0;c=step();CHECK(c.held[Sprint]&&c.held[Forward]);frame.hands[0].stick[1]=0;c=step();CHECK(!c.held[Sprint]);frame.hands[0].stick[1]=1;c=step();CHECK(!c.held[Sprint]&&c.held[Forward]);
    frame.hands[0].stickClick=true;step();frame.hands[0].stickClick=false;c=step();CHECK(c.held[Sprint]);
    CHECK(melee::PhysicalAttackAction(false,true,2,2,0)==11);CHECK(melee::PhysicalAttackAction(true,false,2,2,0)==2);
    frame.timeMs=t+=10;c=input.Translate(frame,t,true,true,2,1,2,0);CHECK(!c.shieldGrip); // menu never emits shield block
    frame.focused=false;frame.timeMs=t+=10;c=input.Translate(frame,t,true,false,2,1,2,0);CHECK(!c.shieldGrip&&!c.held[Use2]);
    frame.focused=true;frame.hands[0].active=false;frame.timeMs=t+=10;c=input.Translate(frame,t,true,false,2,1,2,0);CHECK(!c.shieldGrip);
    puts("PASS raised shield/sword guard, native quick-heal button edges, bounded hearth, native sign cone, rigid particle draw/restoration, remapped context controls, L3 Sprint until stop and double-trigger heal and native Pierce");return 0;
}
