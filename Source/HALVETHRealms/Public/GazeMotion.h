#pragma once
// SPDX-License-Identifier: MIT
#include <algorithm>
#include <cmath>

namespace HalvethGaze {
// All angles are radians relative to the neutral body frame. Eyes and head
// receive the SAME absolute target; eye bones receive only the residual.
struct State {
    double eyeTotalYaw=0,eyeTotalPitch=0,headYaw=0,headPitch=0;
    double eyeYaw=0,eyePitch=0,attention=0,delay=0,shiftAge=1;
    double previousYaw=0,previousPitch=0;
    double contactExpression=0;
    void updateContact(bool contact,double dt){
        if(!std::isfinite(dt)||dt<=0)return;
        const double goal=contact?1.:0.;
        contactExpression=goal+(contactExpression-goal)*std::exp(-dt/.32);
    }
    bool step(double yaw,double pitch,bool observed,double dt){
        if(!std::isfinite(yaw)||!std::isfinite(pitch)||!std::isfinite(dt)||dt<0)return false;
        if(dt==0)return true;
        yaw=observed?std::clamp(yaw,-.85,.85):0;
        pitch=observed?std::clamp(pitch,-.36,.36):0;
        const bool shift=std::hypot(yaw-previousYaw,pitch-previousPitch)>.11;
        if(shift){delay=.11;shiftAge=0;previousYaw=yaw;previousPitch=pitch;}
        // Analytic first-order channels cannot overshoot, even after a stall.
        auto follow=[dt](double old,double goal,double seconds){return goal+(old-goal)*std::exp(-dt/seconds);};
        eyeTotalYaw=follow(eyeTotalYaw,yaw,.065);eyeTotalPitch=follow(eyeTotalPitch,pitch,.075);
        attention=follow(attention,observed?1.:0.,observed?.22:.45);
        const double headDt=std::max(0.,dt-delay);delay=std::max(0.,delay-dt);
        if(headDt>0){
            const double goalYaw=std::clamp(yaw*.78,-.64,.64),goalPitch=std::clamp(pitch*.76,-.26,.26);
            headYaw=goalYaw+(headYaw-goalYaw)*std::exp(-headDt/.32);
            headPitch=goalPitch+(headPitch-goalPitch)*std::exp(-headDt/.38);
        }
        eyeYaw=std::clamp(eyeTotalYaw-headYaw,-.34,.34);
        eyePitch=std::clamp(eyeTotalPitch-headPitch,-.23,.23);
        shiftAge+=dt;return true;
    }
    double shiftBlink() const {
        const double t=std::clamp(1.-std::abs(shiftAge-.075)/.075,0.,1.);
        return .35*t*t*(3.-2.*t);
    }
};
}
