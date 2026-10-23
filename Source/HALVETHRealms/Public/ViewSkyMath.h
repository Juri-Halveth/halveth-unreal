// Copyright (c) 2026 Juri Janovski. MIT. Degrees in, unit quaternions out.
#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

namespace HalvethView {
constexpr double Pi=3.14159265358979323846;
struct V {double x=0,y=0,z=0;};
struct Q {double w=1,x=0,y=0,z=0;};
inline Q multiply(Q a,Q b){return {a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,
    a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w};}
inline Q unit(Q q){const double n=std::sqrt(q.w*q.w+q.x*q.x+q.y*q.y+q.z*q.z);return {q.w/n,q.x/n,q.y/n,q.z/n};}
inline V rotate(Q q,V p){const auto r=multiply(multiply(q,{0,p.x,p.y,p.z}),{q.w,-q.x,-q.y,-q.z});return {r.x,r.y,r.z};}
inline double angularDistance(Q a,Q b){return 2*std::acos(std::fmin(1.,std::abs(a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z)));}
struct Frame {
    Q orientation;
    bool apply(double yaw,double pitch,double roll=0){
        if(!std::isfinite(yaw)||!std::isfinite(pitch)||!std::isfinite(roll))return false;
        const double y=std::remainder(yaw,360.)*Pi/360,p=std::remainder(pitch,360.)*Pi/360,r=std::remainder(roll,360.)*Pi/360;
        // World vertical yaw; camera-local pitch and roll. No Euler pole clamp.
        orientation=unit(multiply(multiply(multiply({std::cos(y),0,0,std::sin(y)},orientation),{std::cos(p),0,-std::sin(p),0}),{std::cos(r),std::sin(r),0,0}));
        return true;
    }
};
struct Star {V direction; double radius,color,brightness;};
inline std::vector<Star> stars(uint32_t seed,int count){
    auto next=[&seed](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return double(seed)/4294967296.;};
    std::vector<Star> result;
    for(int i=0;i<count;i++){
        const double z=2*next()-1,phi=2*Pi*next(),r=std::sqrt(1-z*z),brightness=std::pow(next(),3.);
        result.push_back({{r*std::cos(phi),r*std::sin(phi),z},.00065+.0009*brightness,next(),.08+.92*brightness});
    }
    return result;
}
}
