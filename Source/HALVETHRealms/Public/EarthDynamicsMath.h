#pragma once
#include <cmath>
#include <algorithm>
namespace EarthDynamics {
constexpr double GravityMps2=9.80665;
inline double SupportNewtons(double massKg,double upwardAccelerationMps2,bool supported){
    return supported?std::max(0.,massKg*(GravityMps2+upwardAccelerationMps2)):0.;
}
// Authored quasi-static soil compliance, N/cm; not a measured soil law.
inline double SoilDepthCm(double forceNewtons,double distanceSquaredCm,double stiffnessNewtonsPerCm=450){
    return std::min(.95,std::max(0.,forceNewtons)/stiffnessNewtonsPerCm)*std::exp(-distanceSquaredCm/(2*15.*15.));
}
}
