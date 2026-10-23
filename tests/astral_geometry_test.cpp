// MIT. Structural tests exercise closed volume and the gate's actual rules.
#include "../Source/HALVETHRealms/Public/AstralGeometry.h"
#include <array>
#include <cassert>
#include <iostream>
#include <map>
#include <tuple>

using Key=std::array<long long,3>;
Key key(Astral::V p){return{std::llround(p.x*1000000),std::llround(p.y*1000000),std::llround(p.z*1000000)};}
bool closed(const Astral::Mesh& m){
    std::map<std::pair<Key,Key>,int> edges;
    for(size_t i=0;i<m.t.size();i+=3)for(int j=0;j<3;j++){
        Key a=key(m.p[m.t[i+j]]),b=key(m.p[m.t[i+(j+1)%3]]);if(b<a)std::swap(a,b);edges[{a,b}]++;
    }
    for(auto e:edges){if(e.second!=2)return false;}return !edges.empty();
}
double volume(const Astral::Mesh& m){double v=0;for(size_t i=0;i<m.t.size();i+=3)v+=Astral::dot(m.p[m.t[i]],Astral::cross(m.p[m.t[i+1]],m.p[m.t[i+2]]))/6;return v;}
int main(){
    int checks=0;
    for(auto mesh:{Astral::block({40,70,100},3),Astral::ring(250,3),Astral::sector(365,415,.1,.2,90),Astral::crystal(30,125),Astral::strand({{0,0,0},{0,100,0},{0,200,50},{30,300,80}},2.7)}){
        assert(closed(mesh));checks++;
        assert(volume(mesh)>0);checks++;
        for(size_t i=0;i<mesh.t.size();i+=3){auto n=Astral::cross(mesh.p[mesh.t[i+1]]-mesh.p[mesh.t[i]],mesh.p[mesh.t[i+2]]-mesh.p[mesh.t[i]]);assert(Astral::dot(n,n)>1e-12);}
        checks++;
        mesh.t.resize(mesh.t.size()-3);assert(!closed(mesh));checks++;
    }
    bool rejected=false;try{Astral::block({1,1,1},2);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);checks++;
    Astral::Gate a,b;assert(!a.ready());assert(a.align());assert(!a.align());checks+=3;
    for(int i=0;i<2;i++){a.tick(.6);b.tick(.6);assert(a.align());}
    for(int i=0;i<30;i++)a.tick(1./30);
    b.aligned=3;for(int i=0;i<144;i++)b.tick(1./144);
    assert(std::abs(a.openness-b.openness)<1e-12);checks++;
    assert(!a.ready());a.tick(1);assert(a.ready());checks+=2;
    const double before=a.openness;a.tick(-1);a.tick(NAN);assert(a.openness==before);checks++;
    rejected=false;try{Astral::strand({{0,0,0},{0,0,0}},2);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);checks++;
    const Astral::V source{0,889,560},destination{0,1960,1500};
    for(auto eye:{Astral::V{-440,-130,328},Astral::V{250,100,410},Astral::V{0,600,310}}){
        const auto linked=Astral::linkedObserver(eye,source,destination);
        for(int i=0;i<32;i++){
            const double angle=2*Astral::Pi*i/32;const auto pointA=source+Astral::V{250*std::cos(angle),0,250*std::sin(angle)};
            const auto pointB=destination+(pointA-source),rayA=pointA-eye,rayB=pointB-linked;
            assert(std::abs(rayA.x/rayA.y-rayB.x/rayB.y)<1e-12);
            assert(std::abs(rayA.z/rayA.y-rayB.z/rayB.y)<1e-12);
            assert(std::abs(rayA.z/rayA.y-(rayB.z-10)/rayB.y)>1e-4);
        }checks++;
    }
    std::cout<<"ASTRAL_PORTABLE_PASS checks="<<checks<<" closed_volumes=5 negative_hole_controls=5 fps=30_vs_144 linked_rays=96 negative_camera_controls=96\n";
}
