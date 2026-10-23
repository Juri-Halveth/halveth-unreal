// Copyright (c) 2026 Juri Janovski. MIT. Units: centimetres, radians.
// Portable closed-solid geometry, shared by the game and offline checks.
#pragma once
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace Astral {
constexpr double Pi=3.14159265358979323846;
struct V { double x=0,y=0,z=0; V operator+(V b)const{return{x+b.x,y+b.y,z+b.z};}
    V operator-(V b)const{return{x-b.x,y-b.y,z-b.z};} V operator*(double s)const{return{x*s,y*s,z*s};} };
inline V cross(V a,V b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline double dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
// Both interfaces must have the same orientation. Rotation/mirrors require a
// different operator; this one preserves the observer-to-corresponding-point ray.
inline V linkedObserver(V observer,V source,V destination){return observer+destination-source;}
struct Mesh { std::vector<V> p; std::vector<int> t; std::vector<std::vector<V>> convexParts;
    void face(std::vector<V> q,V inside){
        if(q.size()<3)return;
        V n=cross(q[1]-q[0],q[2]-q[0]); V mid{};for(V v:q)mid=mid+v;mid=mid*(1./q.size());
        const bool reverse=dot(n,mid-inside)<0; const int base=int(p.size());
        p.insert(p.end(),q.begin(),q.end());
        for(int i=1;i<int(q.size())-1;i++){t.push_back(base);t.push_back(base+(reverse?i+1:i));t.push_back(base+(reverse?i:i+1));}
    }
};
inline Mesh block(V h,double b=3){
    if(h.x<=0||h.y<=0||h.z<=0||b<=0||b>=h.x||b>=h.y||b>=h.z)throw std::invalid_argument("invalid bevelled block");
    Mesh m;const double hs[3]={h.x,h.y,h.z};
    auto point=[](const double* p){return V{p[0],p[1],p[2]};};
    for(int axis=0;axis<3;axis++)for(int s:{-1,1}){
        const int a=(axis+1)%3,c=(axis+2)%3;std::vector<V> q;
        for(auto uv:std::vector<V>{{-1,-1,0},{1,-1,0},{1,1,0},{-1,1,0}}){double p[3]={};p[axis]=s*hs[axis];p[a]=uv.x*(hs[a]-b);p[c]=uv.y*(hs[c]-b);q.push_back(point(p));}m.face(q,{});
    }
    for(int axis=0;axis<3;axis++)for(int s:{-1,1})for(int t:{-1,1}){
        int a=(axis+1)%3,c=(axis+2)%3;double p[4][3]={};
        for(int k=0;k<4;k++){p[k][axis]=(k<2?-1:1)*(hs[axis]-b);p[k][a]=s*(hs[a]-(k%3==0?0:b));p[k][c]=t*(hs[c]-(k%3==0?b:0));}
        m.face({point(p[0]),point(p[1]),point(p[2]),point(p[3])},{});
    }
    for(int x:{-1,1})for(int y:{-1,1})for(int z:{-1,1})m.face({{x*h.x,y*(h.y-b),z*(h.z-b)},{x*(h.x-b),y*h.y,z*(h.z-b)},{x*(h.x-b),y*(h.y-b),z*h.z}},{});
    m.convexParts.push_back(m.p);return m;
}
inline Mesh ring(double radius,double tube,int n=128,int sides=8){
    if(radius<=tube||tube<=0||n<12||sides<3)throw std::invalid_argument("invalid torus");
    Mesh m;
    for(int i=0;i<n;i++)for(int j=0;j<sides;j++){double a=2*Pi*i/n,b=2*Pi*j/sides;m.p.push_back({(radius+tube*std::cos(b))*std::cos(a),(radius+tube*std::cos(b))*std::sin(a),tube*std::sin(b)});}
    for(int i=0;i<n;i++){
        for(int j=0;j<sides;j++){int a=i*sides+j,b=((i+1)%n)*sides+j,c=((i+1)%n)*sides+(j+1)%sides,d=i*sides+(j+1)%sides;m.t.insert(m.t.end(),{a,b,c,a,c,d});}
    }
    return m;
}
inline Mesh sector(double inner,double outer,double a,double b,double depth){
    if(inner<0||outer<=inner||b<=a||b-a>=Pi||depth<=0)throw std::invalid_argument("invalid arch sector");
    Mesh m;V p[8];int k=0;for(double z:{-depth/2,depth/2})for(double angle:{a,b})for(double r:{inner,outer})p[k++]={r*std::cos(angle),r*std::sin(angle),z};
    V center{};for(V v:p)center=center+v;center=center*(1./8);
    m.face({p[0],p[1],p[3],p[2]},center);m.face({p[4],p[6],p[7],p[5]},center);
    m.face({p[0],p[4],p[5],p[1]},center);m.face({p[2],p[3],p[7],p[6]},center);
    m.face({p[0],p[2],p[6],p[4]},center);m.face({p[1],p[5],p[7],p[3]},center);m.convexParts.push_back(m.p);return m;
}
inline Mesh crystal(double radius,double height,int sides=6){
    if(radius<=0||height<=0||sides<3)throw std::invalid_argument("invalid crystal");
    Mesh m;V inside{0,0,height*.4};
    for(int i=0;i<sides;i++){double a=2*Pi*i/sides,b=2*Pi*(i+1)/sides;V p{radius*std::cos(a),radius*std::sin(a),height*.15},q{radius*std::cos(b),radius*std::sin(b),height*.15};V r{p.x*.78,p.y*.78,height*.72},s{q.x*.78,q.y*.78,height*.72};m.face({p,q,s,r},inside);m.face({r,s,{0,0,height}},inside);m.face({q,p,{0,0,0}},inside);}
    m.convexParts.push_back(m.p);return m;
}
// A bounded movement trail. End caps close the tube; no collision hull is made.
inline Mesh strand(const std::vector<V>& path,double radius){
    if(path.size()<2||path.size()>64||radius<=0)throw std::invalid_argument("invalid trail");
    Mesh m;constexpr int sides=6;
    for(size_t i=0;i<path.size();i++){
        V t=path[i==path.size()-1?i:i+1]-path[i==0?i:i-1];double length=std::sqrt(dot(t,t));
        if(length<1e-8)throw std::invalid_argument("duplicate trail sample");
        t=t*(1/length);
        V normal=cross(t,std::abs(t.z)>.9?V{0,1,0}:V{0,0,1});normal=normal*(1/std::sqrt(dot(normal,normal)));V binormal=cross(t,normal);
        double r=radius*(.15+.85*double(i)/(path.size()-1));
        for(int j=0;j<sides;j++){double a=2*Pi*j/sides;m.p.push_back(path[i]+normal*(r*std::cos(a))+binormal*(r*std::sin(a)));}
    }
    for(int i=0;i<int(path.size())-1;i++)for(int j=0;j<sides;j++){int a=i*sides+j,b=(i+1)*sides+j,c=(i+1)*sides+(j+1)%sides,d=i*sides+(j+1)%sides;m.t.insert(m.t.end(),{a,c,b,a,d,c});}
    for(int j=1;j<sides-1;j++){m.t.insert(m.t.end(),{0,j+1,j});const int n=(int(path.size())-1)*sides;m.t.insert(m.t.end(),{n,n+j,n+j+1});}return m;
}
// Monotonic stable approach: no overshoot when frame rate changes.
inline double approach(double from,double to,double rate,double dt){return from+(to-from)*(-std::expm1(-rate*dt));}
struct Gate { int aligned=0; double openness=0,cooldown=0;
    void tick(double dt){if(!std::isfinite(dt)||dt<=0)return;cooldown=std::fmax(0.,cooldown-dt);openness=approach(openness,aligned==3?1.:0.,2.5,dt);}
    bool align(){if(cooldown>0||aligned>=3)return false;aligned++;cooldown=.5;return true;}
    bool ready()const{return aligned==3&&openness>.96&&cooldown<=0;}
};
}
