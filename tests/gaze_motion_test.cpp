// SPDX-License-Identifier: MIT
#include "../Source/HALVETHRealms/Public/GazeMotion.h"
#include <cstdio>
#include <limits>
int main(){
    int failures=0,checks=0;
    auto check=[&](bool ok,const char* why){checks++;if(!ok){failures++;std::printf("FAIL %s\n",why);}};
    HalvethGaze::State s;
    for(int i=0;i<5;i++)s.step(.32,.12,true,.016);
    check(s.eyeTotalYaw>.20&&s.headYaw==0,"eyes lead delayed head");
    for(int i=0;i<250;i++)s.step(.32,.12,true,.016);
    check(std::abs(s.headYaw-.2496)<1e-5,"head follows absolute target once");
    check(std::abs(s.headYaw+s.eyeYaw-.32)<1e-7,"head plus residual does not double target");
    check(std::abs(s.headPitch+s.eyePitch-.12)<1e-7,"pitch shares target basis");
    for(int i=0;i<1000;i++){
        const double yaw=i<500?.8:-.8;s.step(yaw,.35,true,.013);
        check(std::abs(s.headYaw)<=.64&&std::abs(s.headPitch)<=.26,"bounded head");
        check(std::abs(s.eyeYaw)<=.34&&std::abs(s.eyePitch)<=.23,"bounded residual");
        check(std::isfinite(s.eyeYaw)&&s.shiftBlink()>=0&&s.shiftBlink()<=.35,"finite bounded blink");
    }
    for(int i=0;i<500;i++)s.step(1.,1.,false,.016);
    check(std::abs(s.headYaw)<1e-8&&std::abs(s.eyeTotalYaw)<1e-8,"occluded or distant target relaxes");
    check(s.attention<1e-7,"attention releases without observation");
    const auto before=s;
    check(!s.step(std::numeric_limits<double>::quiet_NaN(),0,true,.1),"NaN rejected");
    check(!s.step(0,0,true,-1),"negative time rejected");
    check(s.eyeYaw==before.eyeYaw&&s.headYaw==before.headYaw,"invalid input preserves state");
    s.step(.5,.2,true,1000);
    check(std::isfinite(s.headYaw)&&std::abs(s.headYaw)<=.64,"long stall stays bounded");
    // A deliberately coupled control fails the eye-lead discriminator.
    const double coupledEye=.08*.32,coupledHead=.92*.32;
    check(!(coupledEye>.20&&coupledHead==0),"old fixed eye/head ratio rejected");
    HalvethGaze::State expression;expression.updateContact(true,1./60.);
    check(expression.contactExpression>0&&expression.contactExpression<.06,"contact expression rises smoothly");
    for(int i=0;i<200;i++)expression.updateContact(true,1./60.);
    const double high=expression.contactExpression;expression.updateContact(false,1./60.);
    check(high-expression.contactExpression<.06,"contact expression releases smoothly");
    std::printf("GazeMotion: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
