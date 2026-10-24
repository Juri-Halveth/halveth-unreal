// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Juri Janovski
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

// Authored visual soft-tissue proxy, not a biological/material calibration.
// Original implementation of XPBD's accumulated-multiplier update, Eq. (18):
// https://matthias-research.github.io/pages/publications/XPBD.pdf
// Positions: cm; time: seconds; four equal 1 kg proxy masses. No UE dependency.
namespace HalvethTissue {
struct V {
    double x=0,y=0,z=0;
    V operator+(V b)const{return{x+b.x,y+b.y,z+b.z};}
    V operator-(V b)const{return{x-b.x,y-b.y,z-b.z};}
    V operator*(double s)const{return{x*s,y*s,z*s};}
    V operator/(double s)const{return{x/s,y/s,z/s};}
    double length()const{return std::hypot(x,y,z);}
    bool finite()const{return std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z);}
};
namespace Detail {
inline double Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V Cross(V a,V b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline double Volume(const std::array<V,4>& p){return Dot(p[1]-p[0],Cross(p[2]-p[0],p[3]-p[0]))/6.;}
inline constexpr int Edges[6][2]={{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}};
// Bound the numerical domain before any products: centimetre-scale patches,
// finite coordinates within 100 km, edge lengths .1..100 cm, non-sliver tets.
inline bool ValidTargets(const std::array<V,4>& p){
    for(const auto v:p)if(!v.finite()||std::max({std::abs(v.x),std::abs(v.y),std::abs(v.z)})>1e7)return false;
    double longest=0;
    for(const auto& edge:Edges){const double d=(p[edge[0]]-p[edge[1]]).length();if(d<.1||d>100)return false;longest=std::max(longest,d);}
    return std::abs(Volume(p))*6.>1e-4*longest*longest*longest;
}
} // namespace Detail

struct Patch {
    std::array<V,4> position{},velocity{};
    bool initialized=false;
    static constexpr double FixedDt=1./120.,MaxGap=.25,MaxOffsetCm=4.;
    static constexpr double MaxAccelerationCmS2=10000.;
    static constexpr int MaxSubsteps=31,Iterations=16;
    // Unit inverse masses. Attachment/edge compliance use s^2/kg. The volume
    // constraint is normalized by REST signed volume, so its compliance uses
    // s^2/(kg*cm^2); it must not be reused for an unnormalized cm^3 constraint.
    static constexpr double AttachmentCompliance=.0004,EdgeCompliance=.00002;
    static constexpr double VolumeCompliance=1e-10,DampingPerSecond=18.;
    struct Diagnostics {
        unsigned long long initializations=0,acceptedCalls=0,totalSubsteps=0;
        int lastSubsteps=0;
        double maxOffsetCm=0,restVolumeError=0;
    } diagnostics;

    // Explicit initialization/rebase ONLY. Invalid targets leave every field
    // unchanged, including a previously valid patch and its diagnostics.
    void initialize(const std::array<V,4>& targets){
        if(!Detail::ValidTargets(targets))return;
        Patch next;
        next.position=targets;next.previousTargets_=targets;next.solvedTargets_=targets;
        next.restVolume_=Detail::Volume(targets);
        for(int e=0;e<6;e++)next.restEdges_[e]=(targets[Detail::Edges[e][0]]-targets[Detail::Edges[e][1]]).length();
        next.initialized=true;next.diagnostics.initializations=diagnostics.initializations+1;
        *this=next;
    }

    // targets are the END of this input interval, linearly interpolated from
    // the previous accepted input. Fixed 120 Hz ticks carry a fractional tail;
    // a positive interval shorter than a tick is accepted without inventing a
    // variable-size step. position/velocity remain at the last completed tick.
    // No implicit initialize/reset, no dropped time, no displacement clipping.
    // False means transactional rejection; caller may explicitly rebase.
    bool step(const std::array<V,4>& targets,double dt){
        return stepWithAcceleration(targets,dt,{0,0,-980});
    }
    // Acceleration is cm/s^2 in the SAME coordinate frame as targets. It is
    // held over this call's completed fixed ticks; sub-tick input is sampled
    // when the next tick completes. Include gravity here: no gravity is added.
    // Caller owns frame conversion/filtering and any omitted rotating-frame
    // terms. Finite magnitude <=10000; out-of-domain input rejects unchanged.
    bool stepWithAcceleration(const std::array<V,4>& targets,double dt,V accelerationCmS2){
        if(!accelerationCmS2.finite()||accelerationCmS2.length()>MaxAccelerationCmS2)return false;
        if(!initialized||!std::isfinite(dt)||dt<=0||dt>MaxGap||!Detail::ValidTargets(targets)
           ||Detail::Volume(targets)*restVolume_<=0)return false;
        for(int i=0;i<4;i++)if(!position[i].finite()||!velocity[i].finite()
            ||(position[i]-solvedTargets_[i]).length()>MaxOffsetCm||velocity[i].length()>2000)return false;
        Patch next=*this;
        const double total=pending_+dt;
        const int count=static_cast<int>(std::floor(total/FixedDt+1e-10));
        if(count<0||count>MaxSubsteps)return false;
        for(int s=0;s<count;s++){
            const double fraction=std::clamp(((s+1)*FixedDt-pending_)/dt,0.,1.);
            std::array<V,4> goal;
            for(int i=0;i<4;i++)goal[i]=previousTargets_[i]+(targets[i]-previousTargets_[i])*fraction;
            if(!Detail::ValidTargets(goal)||Detail::Volume(goal)*restVolume_<=0||!next.solve(goal,accelerationCmS2))return false;
            next.solvedTargets_=goal;
        }
        next.pending_=std::max(0.,total-count*FixedDt);
        next.previousTargets_=targets;
        next.diagnostics.maxOffsetCm=0;
        for(int i=0;i<4;i++)next.diagnostics.maxOffsetCm=std::max(next.diagnostics.maxOffsetCm,(next.position[i]-targets[i]).length());
        if(next.diagnostics.maxOffsetCm>MaxOffsetCm)return false;
        next.diagnostics.lastSubsteps=count;
        next.diagnostics.totalSubsteps+=static_cast<unsigned>(count);
        next.diagnostics.acceptedCalls++;
        next.diagnostics.restVolumeError=std::abs(Detail::Volume(next.position)/restVolume_-1.);
        *this=next;return true;
    }
    V displacement(int i,const std::array<V,4>& targets)const{
        if(!initialized||i<0||i>=4||!targets[i].finite())return invalidVector();
        return position[i]-targets[i];
    }
    // Reports against the supplied geometry; the solver itself preserves the
    // explicitly INITIALIZED rest volume/edges, not animated target stretch.
    double relativeVolumeError(const std::array<V,4>& targets)const{
        if(!initialized||!Detail::ValidTargets(targets))return std::numeric_limits<double>::infinity();
        return std::abs(Detail::Volume(position)/Detail::Volume(targets)-1.);
    }
    double pendingSeconds()const{return pending_;}

private:
    std::array<V,4> previousTargets_{},solvedTargets_{};
    std::array<double,6> restEdges_{};
    double restVolume_=0,pending_=0;
    static V invalidVector(){const double q=std::numeric_limits<double>::quiet_NaN();return{q,q,q};}
    bool solve(const std::array<V,4>& goal,V accelerationCmS2){
        constexpr double h=FixedDt,w=1.; // Four massive nodes, no pinned node.
        const auto before=position;
        const double decay=std::exp(-DampingPerSecond*h);
        for(int i=0;i<4;i++){
            const V targetVelocity=(goal[i]-solvedTargets_[i])/h;
            const V damped=targetVelocity+(velocity[i]-targetVelocity)*decay;
            position[i]=position[i]+damped*h+accelerationCmS2*(h*h);
        }
        std::array<V,4> attachmentLambda{};
        std::array<double,6> edgeLambda{};
        double volumeLambda=0;
        const double a=AttachmentCompliance/(h*h),b=EdgeCompliance/(h*h),c=VolumeCompliance/(h*h);
        for(int iteration=0;iteration<Iterations;iteration++){
            for(int i=0;i<4;i++){
                const V dl=(position[i]-goal[i]+attachmentLambda[i]*a)*(-1./(w+a));
                attachmentLambda[i]=attachmentLambda[i]+dl;
                position[i]=position[i]+dl*w;
            }
            for(int e=0;e<6;e++){
                const int i=Detail::Edges[e][0],j=Detail::Edges[e][1];
                const V delta=position[i]-position[j];const double length=delta.length();
                if(!std::isfinite(length)||length<1e-10)return false;
                const double dl=(-(length-restEdges_[e])-b*edgeLambda[e])/(2*w+b);
                edgeLambda[e]+=dl;
                const V correction=delta*(w*dl/length);
                position[i]=position[i]+correction;position[j]=position[j]-correction;
            }
            const V x=position[1]-position[0],y=position[2]-position[0],z=position[3]-position[0];
            std::array<V,4> g;
            g[1]=Detail::Cross(y,z)/(6*restVolume_);
            g[2]=Detail::Cross(z,x)/(6*restVolume_);
            g[3]=Detail::Cross(x,y)/(6*restVolume_);
            g[0]=(g[1]+g[2]+g[3])*(-1.);
            double denominator=c;
            for(const auto gradient:g)denominator+=w*Detail::Dot(gradient,gradient);
            const double constraint=Detail::Volume(position)/restVolume_-1.;
            const double dl=(-constraint-c*volumeLambda)/denominator;
            volumeLambda+=dl;
            for(int i=0;i<4;i++)position[i]=position[i]+g[i]*(w*dl);
        }
        if(Detail::Volume(position)*restVolume_<=0)return false;
        for(int i=0;i<4;i++){
            velocity[i]=(position[i]-before[i])/h;
            if(!position[i].finite()||!velocity[i].finite()||(position[i]-goal[i]).length()>MaxOffsetCm
               ||velocity[i].length()>2000)return false;
        }
        return true;
    }
};
} // namespace HalvethTissue
