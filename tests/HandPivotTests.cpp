#include "DualWield.h"
#include "StaffMelee.h"
#include "PhysicalBow.h"
#include "MeleeHandling.h"
#include <cstdio>
using namespace diagnostic;
using namespace diagnostic::arms;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
bool Near(Vec a,Vec b,float tolerance=.025f){return Length(a-b)<tolerance;}
Mat Rotate(float x,float y,float z){float angles[3]{x,y,z};return Rotation(NativeCameraRotation(angles));}
int main(){
    // Landmarks captured from the actual73-bone Hero rig. Test both handed
    // bind orientations: the palm pivot is not a fixed controller-space axis.
    for(unsigned side=0;side<2;++side){
        float mirror=side?1.f:-1.f;
        Mat bind=Rotate(.24f,-.45f,.18f);Position(bind,{mirror*56.6f,120.9f,-.6f});
        Vec index{mirror*62.4f,113.8f,4.3f},pinky{mirror*60.2f,112.f,-3.4f};
        Vec palm;Mat calibration;CHECK(GripCalibration(bind,index,pinky,calibration)&&bow::PalmLocal(bind,index,pinky,palm));
        Vec centre{40000,135,-8000},knuckles=(index+pinky)*.5f;
        CHECK(Near(Point(bind,palm),(Position(bind)+knuckles)*.5f));
        float legacyError=0;Mat attachment=Rotate(.3f,-.4f,.2f);Position(attachment,{2,3,-4});
        for(float units:{75.f,100.f,150.f})for(int profile=0;profile<3;++profile){
            int settings[6]{},angles[3]{profile==0?-30:profile==1?-50:20,profile==2?35:0,profile==1?15:0};
            melee::MeleeHandling weight;uint64_t time=1000;
            for(int axis=0;axis<3;++axis)for(int step=0;step<73;++step){
                float a[3]{};a[axis]=step*.0872664626f;Mat raw=Rotation(NativeCameraRotation(a));Position(raw,centre);
                auto grip=raw;settings[3]=profile==1?55:0;settings[4]=profile==2?-70:0;settings[5]=profile==1?40:0;
                CHECK(CalibrateGrip(grip,side,settings,units));
                auto hand=HandAtPalm(grip,calibration,palm);
                CHECK(Near(Point(hand,palm),centre));
                auto legacy=Multiply(calibration,grip);Position(legacy,Point(grip,{0,0,-.08f*units}));
                legacyError=std::max(legacyError,Length(Point(legacy,palm)-centre));
                // Skinning the actual bind palm, including large world coords,
                // agrees with the planned palm; the IK still reaches exactly.
                Mat inverse;CHECK(Inverse(bind,inverse));auto delta=Multiply(inverse,hand);
                CHECK(Near(Point(delta,Point(bind,palm)),centre));
                Solution solved;CHECK(SolveTracked({40000,130,-8030},{40010,125,-8020},{40020,130,-8010},Position(hand),{mirror*.6f,-1,-.3f},solved));
                CHECK(Near(solved.wrist,Position(hand)));
                // Melee angle corrections must rotate about the same palm,
                // with pre-cull mesh equal to its final skinning attachment.
                melee::OneHandPose weapon;
                CHECK(weapon.PrepareAttachment(attachment,calibration,{{0,0,0},{0,120,0}},grip,angles,units,palm));
                auto armedHand=HandAtPalm(weapon.grip,calibration,palm);
                CHECK(Near(Point(armedHand,palm),centre));
                CHECK(Near(Point(armedHand,palm),centre));
                for(int j=0;j<16;++j)CHECK(std::fabs(weapon.grip.m[j]-grip.m[j])<.0001f);
                auto mesh=Multiply(weapon.attachment,armedHand);
                CHECK(Near(Position(mesh),Position(weapon.mesh)));
                // Compare to the previous weapon-only world pose: keep its
                // established direction/contact while freeing the hand from
                // both automatic blade alignment and per-weapon tilt.
                Mat legacyGrip;CHECK(melee::SwordGrip(attachment,calibration,{{0,0,0},{0,120,0}},grip,angles,legacyGrip));
                auto legacyMesh=Multiply(attachment,HandAtPalm(legacyGrip,calibration,palm));
                for(int j=0;j<16;++j)CHECK(std::fabs(legacyMesh.m[j]-weapon.mesh.m[j])<.035f);
                // Orientation inertia may lag rotation, never circle the hand
                // around the controller when its position is stationary.
                Mat anchor,tracking,filtered,weighted;Position(anchor,centre);
                CHECK(melee::HandlingToTracking(weapon.grip,anchor,units,tracking));
                CHECK(weight.Step(10,1,time+=11,melee::HandlingProfile(profile),tracking,100,20,filtered));
                CHECK(melee::HandlingToWorld(filtered,anchor,units,weighted));
                CHECK(Near(Point(HandAtPalm(weighted,calibration,palm),palm),centre));
                // Live positional offsets are exactly the requested lever,
                // mirrored once. Angular calibration adds no hidden orbit.
                int offset[6]{3,-2,4,55,-70,40};auto adjusted=raw;
                CHECK(CalibrateGrip(adjusted,side,offset,units));
                Vec expected=Point(raw,{mirror*.03f*units,-.02f*units,.04f*units});
                CHECK(Near(Point(HandAtPalm(adjusted,calibration,palm),palm),expected));
                // Ranged and spell contacts keep the same relative palm
                // offsets while the anatomical pivot is corrected.
                int contactOffset[3]{6,2,0};auto bowPose=bow::ResolvedBowGrip(grip,calibration,Position(hand),palm,contactOffset,units,side);
                CHECK(Near(Position(bowPose),centre+Vector(grip,bow::GripContactOffset(contactOffset,units,side))));
            }
        }
        CHECK(legacyError>8); // Old implementation fails the stationary-palm invariant.
        printf("PASS hand%u: real rig pivot,360-degree pitch/yaw/roll, live angles/offsets, weapon calibration, weights, IK, palette and ranged contact; old orbit up to%.2fcm\n",side,legacyError);
        // Standard2H support belongs at the knuckle crease,2cm toward the
        // palm surface, rather than the halfway wrist/knuckle landmark.
        Vec fist;CHECK(melee::SupportFistContact(calibration,palm,side,100,fist));
        Vec relative=Vector(calibration,fist-palm*2);
        CHECK(Near(relative,{side?-2.f:2.f,0,0},.001f));
        CHECK(Length(fist-palm)>4&&Length(fist)>Length(palm));
        for(int type:{3,52}){
            melee::Blade handle;CHECK(melee::HandleSegment({-2,-2,-30},{2,2,150},100,type,handle));
            Mat mesh=Rotate(.2f,.7f,-.4f);Position(mesh,{40000,135,-8000});
            Vec contact=Point(mesh,(handle.root+handle.tip)*.5f);Mat grip=Rotate(.4f,-.3f,.2f);Position(grip,contact);
            melee::SupportAttachment support;CHECK(support.Capture(mesh,handle,contact,grip));
            for(int i=0;i<240;++i){auto motion=Rotate(i*.032f,-i*.043f,i*.016f);Position(motion,{float(i),10,-20});
                auto moved=Multiply(mesh,motion);Mat attached;Vec wrist,actual;
                CHECK(support.Resolve(moved,calibration,fist,attached,wrist,actual));
                auto shown=Multiply(calibration,attached);Position(shown,wrist);
                CHECK(Near(Point(shown,fist),Point(moved,support.LocalPoint())));
                CHECK(Length(Point(shown,palm)-actual)>4); // hilt is beyond heel, under curled fingers
                CHECK(Length(wrist-actual)>8); // cannot attach at wrist
            }
        }
    }
    Vec refused;CHECK(!melee::SupportFistContact(Mat{}, {},2,100,refused));
    CHECK(!melee::SupportFistContact(Mat{}, {},0,0,refused));
    puts("PASS sword/axe support contact remains in closed palm during large swings and movement; staff contact settings unchanged");
    return 0;
}
