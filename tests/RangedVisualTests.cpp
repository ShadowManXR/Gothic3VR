#include <windows.h>
#include "RangedVisualScope.h"
#include <cstdio>
#include <limits>
#include <initializer_list>
using namespace diagnostic::ranged;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct Visual {
    bool visible=false,live=true,flying=false,decayed=false;float alpha=0,speed=2000,damage=70;
    Visual* children[4]{};int count=0,writes=0;
};
struct Access {
    using Entity=Visual;
    bool Valid(Visual* p)const{return p&&p->live;}
    bool Visible(Visual* p)const{return p->visible;}
    float Alpha(Visual* p)const{return p->alpha;}
    int Children(Visual* p)const{return p->count;}
    Visual* Child(Visual* p,int i)const{return p->children[i];}
    // Reproduce the installed Engine's recursive EnableRendering, while
    // SetRenderAlphaValue(value,false) affects only the addressed entity.
    void SetVisible(Visual* p,bool value)const{p->visible=value;++p->writes;for(int i=0;i<p->count;++i)SetVisible(p->children[i],value);}
    void SetAlpha(Visual* p,float value)const{p->alpha=value;++p->writes;}
    void SetAlphaTree(Visual* p,float value)const{SetAlpha(p,value);for(int i=0;i<p->count;++i)SetAlphaTree(p->children[i],value);}
};
struct Trial {Visual loaded,effect;VisualScope<Access> scope;};
void FaultEye(Trial& t){
    __try {t.scope.Capture(&t.loaded);t.scope.Show();RaiseException(0xe0345678,0,0,nullptr);}
    __finally {t.scope.Restore(true);}
}
bool CatchEye(Trial& t){__try{FaultEye(t);}__except(EXCEPTION_EXECUTE_HANDLER){return true;}return false;}
int main(){
    for(int type:{4,7}){
        NockedIdentity bound{1,2,3,type};CHECK(bound.Same(bound,false));CHECK(!bound.Same(bound,true));
        CHECK(!bound.Same({9,2,3,type},false)&&!bound.Same({1,9,3,type},false));
        CHECK(!bound.Same({1,2,9,type},false)&&!bound.Same({1,2,3,type==4?7:4},false));
        Visual loaded,effect;effect.visible=true;effect.alpha=.4f;loaded.children[0]=&effect;loaded.count=1;
        VisualScope<Access> scope;
        for(int shot=0;shot<12;++shot){
            // No animation factory/hero palette callback is required, even
            // when damage poses hide the native held model before each eye.
            for(int eye=0;eye<2;++eye){
                CHECK(scope.Capture(&loaded));scope.Show();CHECK(loaded.visible&&loaded.alpha==1&&effect.visible&&effect.alpha==1);
                loaded.visible=false;loaded.alpha=0;scope.Show();CHECK(loaded.visible&&loaded.alpha==1);
                scope.Restore(bound.Same(bound,false));CHECK(!loaded.visible&&loaded.alpha==0&&effect.visible&&effect.alpha==.4f);
                scope.Restore(true);CHECK(!loaded.visible&&effect.visible);
            }
            Visual projectile;projectile.children[0]=&effect;projectile.count=1;
            float speed=projectile.speed,damage=projectile.damage;
            CHECK(RevealLaunch<Access>(&projectile,true,false));CHECK(projectile.visible&&projectile.alpha==1&&effect.visible&&effect.alpha==1);
            CHECK(projectile.speed==speed&&projectile.damage==damage&&!projectile.decayed);
            projectile.flying=true;
            CHECK(!bound.Same(bound,projectile.flying));CHECK(!RevealLaunch<Access>(&projectile,true,true));
            // A scope for the loaded entity never hides the different launched
            // projectile; native hit/decay remains free to retire that entity.
            CHECK(scope.Capture(&loaded));scope.Show();scope.Restore(true);CHECK(projectile.visible);
            projectile.decayed=true;projectile.alpha=0;int writes=projectile.writes;
            CHECK(!RevealLaunch<Access>(&projectile,false,false)&&projectile.writes==writes&&projectile.alpha==0);
            // Native Shoot destroys/replaces the held entity. Never restore
            // a stale snapshot into the next slot or into an airborne arrow.
            CHECK(scope.Capture(&loaded));scope.Show();writes=loaded.writes;loaded.live=false;
            CHECK(!bound.Same({1,2,uintptr_t(10+shot),type},false));scope.Restore(false);CHECK(loaded.writes==writes);
            scope.Abandon();loaded=Visual{};loaded.children[0]=&effect;loaded.count=1;effect.visible=true;effect.alpha=.4f;
        }
    }
    CHECK((!NockedIdentity{1,2,3,5}.Same({1,2,3,5},false)));
    // Some native attachments put ammo beneath the ranged weapon. Borrowing
    // the child after the parent's recursive show must unwind child first.
    Visual weapon,ammo,trail;ammo.visible=true;ammo.alpha=.2f;trail.alpha=.7f;
    weapon.children[0]=&ammo;weapon.count=1;ammo.children[0]=&trail;ammo.count=1;
    VisualScope<Access> outer,inner;
    for(int eye=0;eye<2;++eye){
        CHECK(outer.Capture(&weapon));outer.Show();CHECK(inner.Capture(&ammo));inner.Show();
        inner.Restore(true);outer.Restore(true);
        CHECK(!weapon.visible&&weapon.alpha==0&&ammo.visible&&ammo.alpha==.2f&&!trail.visible&&trail.alpha==.7f);
    }
    Visual root,child;root.children[0]=&child;root.count=1;
    VisualScope<Access,1> bounded;CHECK(!bounded.Capture(&root)&&root.writes==0&&child.writes==0);
    child.children[0]=&root;child.count=1;VisualScope<Access> scope;CHECK(!scope.Capture(&root)&&root.writes==0);
    child.count=0;child.alpha=std::numeric_limits<float>::quiet_NaN();CHECK(!scope.Capture(&root)&&root.writes==0);
    child.alpha=.3f;CHECK(scope.Capture(&root));scope.Abandon();scope.Show();CHECK(root.writes==0);
    Trial fault;fault.effect.visible=true;fault.effect.alpha=.6f;fault.loaded.children[0]=&fault.effect;fault.loaded.count=1;
    CHECK(CatchEye(fault)&&!fault.loaded.visible&&fault.loaded.alpha==0&&fault.effect.visible&&fault.effect.alpha==.6f);
    Visual npc;CHECK(!RevealLaunch<Access>(&npc,false,false)&&!npc.visible&&npc.alpha==0&&npc.writes==0);
    puts("PASS static held arrows/bolts in both eyes, recursive visibility/alpha restoration, repeated native replacement, launch isolation, NPC/flight/decay gates and render fault cleanup");
    return 0;
}
