#pragma once
#include "TrackedArmsMath.h"
#include <vector>
#include <string>
#include <cstring>

namespace diagnostic { namespace bow {
// Read the installed model/motion through the game's virtual filesystem. No
// redistributed game meshes and no writes to shared EMotionFX actor state.
class Model {
    struct Key {float time;Orientation rotation;};
    struct Node {std::string name,parentName;int parent=-1;arms::Mat local,bind,inverse;std::vector<Key> keys;};
    struct Vertex {unsigned original;arms::Vec position;};
    struct Weight {unsigned node;float amount;};
    std::vector<Node> nodes;float duration=0;bool crossbow=false;arms::Vec rest,nockParts[32];float nockWeights[32]{};
    static bool U32(unsigned char const* p,size_t n,size_t at,unsigned& value){if(at>n||n-at<4)return false;memcpy(&value,p+at,4);return true;}
    static bool Text(unsigned char const* p,size_t n,size_t& at,std::string& text){unsigned length;if(!U32(p,n,at,length)||length>160||at+4>n||length>n-at-4)return false;at+=4;text.assign(reinterpret_cast<char const*>(p+at),length);at+=length;return text.find('\0')==std::string::npos;}
    static bool Transform(unsigned char const* p,size_t n,arms::Mat& out){
        if(n<28)return false;float v[7];memcpy(v,p,sizeof(v));Orientation q{v[6],v[3],v[4],v[5]};
        if(!Normalize(q))return false;out=arms::Rotation(Matrix(q));arms::Position(out,{v[0],v[1],v[2]});return arms::Affine(out);
    }
    static size_t Header(std::vector<unsigned char> const& b,char const* magic,size_t skip){
        for(size_t i=0;i+skip<=b.size()&&i<128;++i)if(!memcmp(b.data()+i,magic,4))return i+skip;return b.size();
    }
    static bool End(std::vector<unsigned char> const& b,size_t at,size_t skip,size_t& end){
        end=b.size();if(at>=b.size())return false;
        if(b.size()>=8&&!memcmp(b.data(),"GENOMFLE",8)){unsigned length;size_t magic=at-skip;if(magic<4||!U32(b.data(),b.size(),magic-4,length)||length<skip||length>b.size()-magic)return false;end=magic+length;}
        return at<=end;
    }
    int Find(std::string const& name)const{for(unsigned i=0;i<nodes.size();++i)if(nodes[i].name==name)return int(i);return -1;}
    bool World(unsigned i,float time,arms::Mat& out,unsigned depth=0)const{
        if(i>=nodes.size()||depth>=nodes.size())return false;auto const& node=nodes[i];out=node.local;
        if(time>=0&&!node.keys.empty()){
            auto const& keys=node.keys;size_t high=0;while(high+1<keys.size()&&keys[high].time<time)++high;
            Orientation q=keys[high].rotation;
            if(high&&keys[high].time>keys[high-1].time){auto a=keys[high-1].rotation;auto b=q;float t=std::clamp((time-keys[high-1].time)/(keys[high].time-keys[high-1].time),0.f,1.f);
                if(a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z<0)b={-b.w,-b.x,-b.y,-b.z};
                q={a.w+(b.w-a.w)*t,a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};if(!Normalize(q))return false;
            }
            auto position=arms::Position(out);out=arms::Rotation(Matrix(q));arms::Position(out,position);
        }
        if(node.parent>=0){arms::Mat parent;if(!World(unsigned(node.parent),time,parent,depth+1))return false;out=arms::Multiply(out,parent);}return arms::Affine(out);
    }
public:
    bool valid=false;arms::Vec grip,nock,arrowSupport,arrowRest,foregrip;float maximumDraw=0;unsigned refusedAt=0;
    bool Refuse(unsigned line){refusedAt=line;return false;}
    void Clear(){*this=Model{};}
    bool Load(std::vector<unsigned char> const& actor,std::vector<unsigned char> const& motion,bool isCrossbow=false){
        Clear();crossbow=isCrossbow;std::vector<Vertex> vertices;std::vector<unsigned> triangles;std::vector<std::vector<Weight>> weights;unsigned originalCount=0;
        size_t at=Header(actor,"FXA ",6),end;bool mesh=false;if(!End(actor,at,6,end))return Refuse(__LINE__);
        while(at+12<=end){
            unsigned id,length,version;U32(actor.data(),actor.size(),at,id);U32(actor.data(),actor.size(),at+4,length);U32(actor.data(),actor.size(),at+8,version);at+=12;
            if(length>end-at)return Refuse(__LINE__);auto* p=actor.data()+at;size_t n=length;
            if(id==0){
                if(version!=3||nodes.size()>=32||n<76)return Refuse(__LINE__);Node node;size_t s=68;
                if(!Transform(p,n,node.local)||!Text(p,n,s,node.name)||!Text(p,n,s,node.parentName)||node.name.empty()||Find(node.name)>=0)return Refuse(__LINE__);
                float scale[3];memcpy(scale,p+44,12);for(float v:scale)if(!std::isfinite(v)||std::fabs(v-1)>.001f)return Refuse(__LINE__);
                nodes.push_back(node);
            }else if(id==3){
                if(version!=3||mesh||n<40)return Refuse(__LINE__);unsigned total,indices,submeshes,uv;
                U32(p,n,4,originalCount);U32(p,n,8,total);U32(p,n,12,indices);U32(p,n,16,submeshes);U32(p,n,20,uv);
                if(!originalCount||originalCount>20000||!total||total>40000||!submeshes||submeshes>8||uv!=1)return Refuse(__LINE__);
                size_t s=28;unsigned seen=0;
                for(unsigned part=0;part<submeshes;++part){unsigned partIndices,partVertices;if(!U32(p,n,s+4,partIndices)||!U32(p,n,s+8,partVertices)||partVertices>total-seen||partIndices%3)return Refuse(__LINE__);s+=12;
                    if(size_t(partVertices)*36+size_t(partIndices)*4>n-s)return Refuse(__LINE__);
                    for(unsigned i=0;i<partVertices;++i){Vertex vertex;U32(p,n,s,vertex.original);memcpy(&vertex.position,p+s+4,12);if(vertex.original>=originalCount||!arms::Finite(vertex.position))return Refuse(__LINE__);vertices.push_back(vertex);s+=36;}
                    for(unsigned i=0;i<partIndices;++i){unsigned index;if(!U32(p,n,s+i*4,index)||index>=partVertices)return Refuse(__LINE__);triangles.push_back(seen+index);}
                    s+=size_t(partIndices)*4;seen+=partVertices;
                }if(seen!=total)return Refuse(__LINE__);mesh=true;
            }else if(id==4){
                if(version!=1||!originalCount||!weights.empty()||n<4)return Refuse(__LINE__);size_t s=4;
                for(unsigned i=0;i<originalCount;++i){if(s>=n)return Refuse(__LINE__);unsigned count=p[s++];if(!count||count>8||count*8>n-s)return Refuse(__LINE__);std::vector<Weight> w;float sum=0;
                    for(unsigned j=0;j<count;++j){Weight a;a.node=p[s];memcpy(&a.amount,p+s+4,4);s+=8;if(!std::isfinite(a.amount)||a.amount<0||a.amount>1)return Refuse(__LINE__);sum+=a.amount;w.push_back(a);}if(std::fabs(sum-1)>.01f)return Refuse(__LINE__);weights.push_back(w);}
                if(s!=n)return Refuse(__LINE__);
            }
            at+=length;
        }
        int gripId=Find("Bone01"),upper=Find(crossbow?"Bone04":"Bone03"),lower=Find(crossbow?"Bone05":"Bone06");
        if(gripId<0||upper<0||lower<0||weights.size()!=originalCount)return Refuse(__LINE__);
        for(auto& node:nodes)if(!node.parentName.empty()){node.parent=Find(node.parentName);if(node.parent<0)return Refuse(__LINE__);}
        for(unsigned i=0;i<nodes.size();++i)if(!World(i,-1,nodes[i].bind)||!arms::Inverse(nodes[i].bind,nodes[i].inverse))return Refuse(__LINE__);
        grip=arms::Position(nodes[gripId].bind);
        // The native string's four centre vertices are equally weighted to the
        // two tip bones. Their skinned position is the actual nocking point.
        unsigned found=0;rest={};
        for(auto const& vertex:vertices){auto const& w=weights[vertex.original];float top=0,bottom=0;
            for(auto const& weight:w){if(weight.node>=nodes.size())return Refuse(__LINE__);if(weight.node==unsigned(upper))top=weight.amount;if(weight.node==unsigned(lower))bottom=weight.amount;}
            if(top>.35f&&top<.65f&&bottom>.35f&&bottom<.65f&&top+bottom>.9f){
                rest=rest+vertex.position;++found;for(auto const& weight:w){unsigned h=weight.node;nockParts[h]=nockParts[h]+arms::Point(nodes[h].inverse,vertex.position)*weight.amount;nockWeights[h]+=weight.amount;}}}
        if(crossbow?(found<4||found>32):found!=4)return Refuse(__LINE__);rest=rest*(1.f/found);for(unsigned i=0;i<nodes.size();++i)if(nockWeights[i]>0){nockParts[i]=nockParts[i]*(1.f/nockWeights[i]);nockWeights[i]/=float(found);}
        // Keep the native lower handle grip separate from the arrow's rest.
        // Holding at arrow height makes the arrow pass through the hand. Locate
        // the rigid bow cross-section at string height only as arrow support.
        // Reject string/limb triangles by their skinning weights.
        arms::Vec low{},high{};unsigned crossings=0;
        for(size_t t=0;t+2<triangles.size();t+=3){bool handle=true;
            for(unsigned j=0;j<3;++j){float weight=0;for(auto const& w:weights[vertices[triangles[t+j]].original])if(w.node==unsigned(gripId))weight+=w.amount;handle=handle&&weight>.8f;}
            if(!handle)continue;
            for(unsigned j=0;j<3;++j){auto a=vertices[triangles[t+j]].position,b=vertices[triangles[t+(j+1)%3]].position;float dz=b.z-a.z;
                float plane=crossbow?rest.z*.65f:rest.z;
                if(std::fabs(dz)<.00001f)continue;float f=(plane-a.z)/dz;if(f<0||f>1)continue;auto point=a+(b-a)*f;
                if(!crossings)low=high=point;else{low.x=std::min(low.x,point.x);low.y=std::min(low.y,point.y);high.x=std::max(high.x,point.x);high.y=std::max(high.y,point.y);}++crossings;
            }
        }
        if(crossings<4||high.x-low.x>30||high.y-low.y>30)return Refuse(__LINE__);
        arrowSupport={(low.x+high.x)*.5f,(low.y+high.y)*.5f,rest.z};
        // Model +Y becomes the shooter's right. Rest the shaft to the left
        // of the rigid handle, with clearance for its radius. Use the rear
        // edge so the segment from the string clears the entire handle width.
        arrowRest={low.x,low.y-.6f,rest.z};
        foregrip={(low.x+high.x)*.5f,low.y,rest.z*.65f};
        at=Header(motion,"LMA ",7);if(!End(motion,at,7,end))return Refuse(__LINE__);int track=-1;unsigned tracks=0;
        while(at+12<=end){
            unsigned id,length,version;U32(motion.data(),motion.size(),at,id);U32(motion.data(),motion.size(),at+4,length);U32(motion.data(),motion.size(),at+8,version);at+=12;
            if(length>end-at)return Refuse(__LINE__);auto* p=motion.data()+at;size_t n=length;
            if(id==1){std::string name;size_t s=80;if(version!=3||!Text(p,n,s,name))return Refuse(__LINE__);track=Find(name);if(track<0&&name.rfind("It_Weapon_",0)!=0&&!(crossbow&&name.rfind("G3_Weapons_",0)==0))return Refuse(__LINE__);}
            else if(id==2){unsigned count,kind;if(version!=1||track<0||!U32(p,n,0,count)||!U32(p,n,4,kind)||kind!=0x524c||!count||count>4096||8+size_t(count)*20!=n)return Refuse(__LINE__);
                auto& keys=nodes[track].keys;if(!keys.empty())return Refuse(__LINE__);
                for(unsigned i=0;i<count;++i){float v[5];memcpy(v,p+8+i*20,20);Key k{v[0],{v[4],v[1],v[2],v[3]}};
                    if(!std::isfinite(k.time)||k.time<0||(!keys.empty()&&k.time<=keys.back().time)||!Normalize(k.rotation))return Refuse(__LINE__);keys.push_back(k);duration=std::max(duration,k.time);}++tracks;
            }
            at+=length;
        }
        if(tracks!=4||duration<=0||duration>10||!NockAt(0,nock))return Refuse(__LINE__);
        arms::Vec full;if(!NockAt(duration,full))return Refuse(__LINE__);maximumDraw=crossbow?full.z-nock.z:nock.x-full.x;
        valid=maximumDraw>5&&maximumDraw<100&&arms::Length(nock-grip)>5&&arms::Length(nock-grip)<70;return valid;
    }
    bool NockAt(float time,arms::Vec& out)const{out={};for(unsigned h=0;h<nodes.size();++h)if(nockWeights[h]>0){arms::Mat pose;if(!World(h,time,pose))return false;out=out+arms::Point(pose,nockParts[h])*nockWeights[h];}return arms::Finite(out);}
    bool ArrowAt(float time,arms::Vec& drawn,arms::Vec& direction)const{
        if(!valid||!NockAt(time,drawn))return false;
        if(crossbow){direction={0,0,-1};return true;}
        direction=arrowRest-drawn;return direction.x>0&&arms::Unit(direction);
    }
    float TimeForDraw(float centimetres)const{
        float target=std::clamp(centimetres,0.f,maximumDraw),lo=0,hi=duration;
        for(unsigned i=0;i<16;++i){float mid=(lo+hi)*.5f;arms::Vec p;if(!NockAt(mid,p))return 0;if((crossbow?p.z-nock.z:nock.x-p.x)<target)lo=mid;else hi=mid;}return (lo+hi)*.5f;
    }
    bool Palette(char const* name,float time,arms::Mat const& world,arms::Mat& out)const{
        int i=Find(name);arms::Mat model;if(!valid||i<0||!World(unsigned(i),time,model))return false;
        out=arms::Multiply(arms::Multiply(nodes[i].inverse,model),world);return arms::Affine(out);
    }
};
}}
