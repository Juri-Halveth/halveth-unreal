#pragma once
#include <algorithm>
#include <cmath>

namespace HalvethMotion {
struct V {
    double x=0,y=0,z=0;
    V operator+(V b)const{return{x+b.x,y+b.y,z+b.z};}
    V operator-(V b)const{return{x-b.x,y-b.y,z-b.z};}
    V operator*(double s)const{return{x*s,y*s,z*s};}
    double dot(V b)const{return x*b.x+y*b.y+z*b.z;}
    double length()const{return std::sqrt(dot(*this));}
    V unit()const{double l=length();return l>1e-12?*this*(1/l):V{0,0,1};}
};
struct Chain {V knee,end;bool reachable;};
// Maximum hip height for the requested horizontal foot position and knee bend.
// Both leg lengths remain anatomical; a shared pelvis takes the lower limit.
inline double SupportedHipHeight(V hip,V foot,double upper,double lower,double bendRadians){
    double dx=hip.x-foot.x,dy=hip.y-foot.y;
    double reach2=upper*upper+lower*lower+2*upper*lower*std::cos(bendRadians);
    return foot.z+std::sqrt(std::max(0.0,reach2-dx*dx-dy*dy));
}
// Centimetres. Preserve both segment lengths; unreachable goals are explicit.
inline Chain Solve(V start,V target,V pole,double upper,double lower){
    V delta=target-start;double requested=delta.length();
    double lo=std::abs(upper-lower)+1e-7,hi=upper+lower-1e-7;
    double d=std::clamp(requested,lo,hi);V direction=delta.unit();
    V bend=pole-direction*pole.dot(direction);
    if(bend.length()<1e-8){V alternative=std::abs(direction.z)<.9?V{0,0,1}:V{1,0,0};bend=alternative-direction*alternative.dot(direction);}
    double along=(upper*upper-lower*lower+d*d)/(2*d);
    double height=std::sqrt(std::max(0.0,upper*upper-along*along));
    return{start+direction*along+bend.unit()*height,start+direction*d,requested>=lo&&requested<=hi};
}
struct Spring {
    double position=0,velocity=0;
    void update(double target,double dt,double omega=9){
        if(!std::isfinite(target)||!std::isfinite(dt)||dt<0||!std::isfinite(omega)||omega<=0)return;
        double a=position-target,b=velocity+omega*a,e=std::exp(-omega*dt);
        position=target+(a+b*dt)*e;velocity=(velocity-omega*b*dt)*e;
    }
};
struct Step {double forward,lift;bool stance;};
inline Step Footstep(double phase,double halfStride=24,double height=7){
    phase-=std::floor(phase);
    if(phase<.6)return{halfStride*(1-2*phase/.6),0,true};
    double u=(phase-.6)/.4,s=u*u*(3-2*u);
    return{halfStride*(-1+2*s),height*std::sin(3.141592653589793*u),false};
}
}
