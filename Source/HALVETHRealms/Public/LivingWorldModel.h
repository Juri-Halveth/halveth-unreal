// Copyright (c) 2026 Juri Janovski. MIT. Authored game behaviour, not human cognition.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "ResidentContinuity.h"

namespace HalvethLife {
struct Point { double x=0,y=0; };
inline uint32_t mix(uint32_t n){n^=n>>16;n*=0x7feb352dU;n^=n>>15;n*=0x846ca68bU;n^=n>>16;return n;}
enum class Task { Rest, Work, Market, Study, Wander, Patrol };
enum class Temper { Warm, Blunt, Quiet, Curious };
struct Resident {
    uint32_t id=0,seed=0;int realm=0;bool guard=false;Temper temper=Temper::Warm;
    Point position;Task task=Task::Wander;uint64_t decisions=0;double lastUpdate=0;
    bool wasPhysical=false;
    double retiredAt=-1;
    HalvethContinuity::Pose physicalPose;
};
inline Point materializationPosition(const Resident& r){
    if(r.guard&&!r.wasPhysical)return {25+(r.id<4?-3.:3.),-4.5};
    return r.position;
}
inline bool synchronizePhysical(Resident& r,Point position,double now,const HalvethContinuity::Pose& pose){
    if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(now)
       ||now<r.lastUpdate||!HalvethContinuity::finite(pose))return false;
    // Position and its clock are one handoff. Updating only position lets the
    // next background cohort consume time already simulated by CharacterMovement.
    r.position=position;r.lastUpdate=now;r.physicalPose=pose;r.wasPhysical=true;
    return true;
}
inline bool retirePhysical(Resident& r,Point position,double now,const HalvethContinuity::Pose& pose){
    if(!synchronizePhysical(r,position,now,pose))return false;
    r.retiredAt=now;
    return true;
}
inline Point destination(const Resident& r,double seconds){
    const auto cycle=static_cast<uint32_t>(std::floor(std::max(0.,seconds)/24.));
    const auto a=mix(r.seed+cycle*131U),b=mix(a);constexpr double tau=6.283185307179586;
    const double angle=(a/4294967296.)*tau;
    const double radius=4+(b%1800)/100.;
    // Four declared local places; these coordinates do not claim complete housing.
    const Point places[]={{25,-8},{25,18},{0,-6},{-7,5}};
    const auto at=places[(r.id/4+cycle)%4];
    return {at.x+std::cos(angle)*radius,at.y+std::sin(angle)*radius};
}
inline Task routine(const Resident& r,double seconds){
    if(r.guard)return Task::Patrol;
    const auto hour=static_cast<int>(std::floor(std::max(0.,seconds)/3600.))%24;
    const int shift=(hour+static_cast<int>(r.id%3)*8)%24;
    if(shift<5)return Task::Rest;
    if(shift<13)return Task::Work;
    if(shift<17)return Task::Market;
    if(shift<21)return Task::Study;
    return Task::Wander;
}
inline std::vector<Resident> population(uint32_t seed,uint32_t count){
    std::vector<Resident> out;out.reserve(count);
    for(uint32_t i=0;i<count;i++){
        Resident r;r.id=i;r.seed=mix(seed+i*7919U);r.realm=static_cast<int>(i%4);r.guard=i<8;
        r.temper=static_cast<Temper>((r.seed>>8)%4);r.position=destination(r,0);out.push_back(r);
    }return out;
}
inline bool advance(Resident& r,double now){
    if(!std::isfinite(now)||now<r.lastUpdate)return false;
    const double dt=now-r.lastUpdate;r.lastUpdate=now;
    const auto task=routine(r,now);if(task!=r.task){r.task=task;r.decisions++;}
    if(task==Task::Rest)return true;
    const auto goal=destination(r,now);const double dx=goal.x-r.position.x,dy=goal.y-r.position.y;
    const double d=std::hypot(dx,dy),step=std::min(d,dt*(r.guard?1.5:.7+(r.seed%55)/100.));
    if(d>1e-8){r.position.x+=dx*step/d;r.position.y+=dy*step/d;}return true;
}
enum class Stage { Quiet, Warning, Pursuit, Held, Cooling };
enum class Event { None, Warn, Chase, Hold, Release, Resolved, Lost };
struct Signal { double now=0;bool visible=false,hazard=false,inReach=false; };
struct Notice {
    Stage stage=Stage::Quiet;double entered=0,lastSeen=0,lastNow=0,safeSince=-1;int warnings=0,holds=0;
    Event observe(Signal s){
        if(!std::isfinite(s.now)||s.now<lastNow)return Event::None;
        lastNow=s.now;if(s.visible)lastSeen=s.now;
        if(stage==Stage::Held){if(s.now-entered>=2.2){stage=Stage::Cooling;entered=s.now;return Event::Release;}return Event::None;}
        if(stage==Stage::Cooling){if(s.now-entered>=25)stage=Stage::Quiet;else return Event::None;}
        if(stage==Stage::Quiet&&s.visible&&s.hazard){stage=Stage::Warning;entered=s.now;safeSince=-1;warnings++;return Event::Warn;}
        if(stage==Stage::Warning){
            // A single collision/stalled frame must not cancel a warning and
            // grant another cooldown. Observe a stable safe interval instead.
            if(s.visible&&!s.hazard){
                if(safeSince<0)safeSince=s.now;
                if(s.now-safeSince>=1){stage=Stage::Cooling;entered=s.now;return Event::Resolved;}
            }else safeSince=-1;
            if(s.now-lastSeen>8){stage=Stage::Cooling;entered=s.now;return Event::Lost;}
            if(s.visible&&s.hazard&&s.now-entered>=6){stage=Stage::Pursuit;entered=s.now;return Event::Chase;}
        }
        if(stage==Stage::Pursuit){
            if(s.now-lastSeen>8){stage=Stage::Cooling;entered=s.now;return Event::Lost;}
            if(s.visible&&s.inReach){stage=Stage::Held;entered=s.now;holds++;return Event::Hold;}
        }
        return Event::None;
    }
};
}
