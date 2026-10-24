// Copyright (c) 2026 Juri Janovski. MIT. Materialization policy, not movement.
#pragma once
#include <cmath>
#include <cstddef>

namespace HalvethContinuity {
enum class Visibility { Unknown, InView, OutsideView };
struct Policy {
    std::size_t bodyBudget=18;
    double enterMetres=50,exitMetres=65,unseenCapMetres=100;
    double unseenGraceSeconds=2,rematerializeDelaySeconds=3;
};
struct Pose {
    double heightMetres=0,yawDegrees=-90,idleSeconds=0,gaitPhase=0;
};
inline bool finite(const Pose& pose){
    return std::isfinite(pose.heightMetres)&&std::isfinite(pose.yawDegrees)
        &&std::isfinite(pose.idleSeconds)&&std::isfinite(pose.gaitPhase);
}
// Existing bodies never compete with a nearest-N candidate list. Unknown view
// coverage protects an actor just as an intersecting conservative camera bound does.
inline bool retire(double distanceMetres,double now,Visibility view,bool pinned,
                   double& outsideSince,const Policy& policy={}){
    if(!std::isfinite(distanceMetres)||distanceMetres<0||!std::isfinite(now)
       ||pinned||view!=Visibility::OutsideView||distanceMetres<=policy.exitMetres){
        outsideSince=-1;
        return false;
    }
    if(outsideSince<0||now<outsideSince)outsideSince=now;
    return distanceMetres>=policy.unseenCapMetres||now-outsideSince>=policy.unseenGraceSeconds;
}
inline bool materialize(std::size_t physical,double distanceMetres,double now,
                        Visibility view,bool initialPopulation,double retiredAt,
                        const Policy& policy={}){
    if(physical>=policy.bodyBudget||!std::isfinite(distanceMetres)||distanceMetres<0
       ||distanceMetres>policy.enterMetres||!std::isfinite(now)||!std::isfinite(retiredAt))return false;
    if(!initialPopulation&&view!=Visibility::OutsideView)return false;
    return retiredAt<0||(now>=retiredAt&&now-retiredAt>=policy.rematerializeDelaySeconds);
}
inline Pose resume(Pose pose,double retiredAt,double now){
    // Idle time continues off screen; gait resumes from its saved phase. No
    // off-screen skeletal motion or interpolated physical path is invented.
    if(retiredAt>=0&&std::isfinite(now)&&now>=retiredAt)pose.idleSeconds+=now-retiredAt;
    return pose;
}
}
