// SPDX-License-Identifier: MIT
#include "../Source/HALVETHRealms/Public/GuideMotionMath.h"
#include "../Source/HALVETHRealms/Public/EarthDynamicsMath.h"
#include "../Source/HALVETHRealms/Public/GuideBodyModel.h"
#include "../Source/HALVETHRealms/Public/GuideGenome.h"
// Keep all existing assertions active in optimized/NDEBUG CI configurations.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <limits>

namespace {
using HalvethMotion::Step;
using StepFunction=Step (*)(double,double,double);
Step OldFootstep(double phase,double stride,double height){
 phase-=std::floor(phase);
 if(phase<.6)return{stride*(1-2*phase/.6),0,true};
 const double u=(phase-.6)/.4,s=u*u*(3-2*u);
 return{stride*(-1+2*s),height*std::sin(3.141592653589793*u),false};
}
// C1 in LOCAL coordinates, but wrong for contact with a moving body's ground.
Step ZeroLocalVelocity(double phase,double stride,double height){
 phase-=std::floor(phase);
 auto result=HalvethMotion::Footstep(phase,stride,height);
 const double u=phase<.6?phase/.6:(phase-.6)/.4;
 const double s=u*u*(3-2*u);
 result.forward=phase<.6?stride*(1-2*s):stride*(-1+2*s);
 return result;
}
bool C1Boundaries(StepFunction step,double stride=24,double height=7){
 constexpr double e=1e-7;
 for(double boundary:{.6,1.}){
  const auto a=step(boundary-e,stride,height),b=step(boundary,stride,height),c=step(boundary+e,stride,height);
  for(const auto sample:{a,b,c})if(!std::isfinite(sample.forward)||!std::isfinite(sample.lift))return false;
  if(std::abs(a.forward-b.forward)>.001||std::abs(c.forward-b.forward)>.001
     ||std::abs((b.forward-a.forward)/e-(c.forward-b.forward)/e)>.005
     ||std::abs((b.lift-a.lift)/e-(c.lift-b.lift)/e)>.005)return false;
 }
 return true;
}
bool WorldContact(StepFunction step,double speed,double weight){
 constexpr double e=1e-7,stride=24;
 // The animation instance uses this rate: body translation cancels stance
 // travel. Test BOTH sides of each transition, including the 1 -> 0 wrap.
 const double rate=.6*speed/(48*weight);
 for(double boundary:{.6,1.}){
  const auto a=step(boundary-e,stride,7),b=step(boundary,stride,7),c=step(boundary+e,stride,7);
  for(double localVelocity:{(b.forward-a.forward)/e,(c.forward-b.forward)/e})
   if(!std::isfinite(localVelocity)||std::abs(speed+weight*rate*localVelocity)>.02)return false;
  for(double verticalVelocity:{(b.lift-a.lift)/e,(c.lift-b.lift)/e})
   if(!std::isfinite(verticalVelocity)||std::abs(weight*rate*verticalVelocity)>.02)return false;
 }
 // Constant world position throughout stance, not just at its endpoints.
 for(int i=0;i<60;i++){
  const double p=static_cast<double>(i)*.01;
  const double world=weight*step(p,stride,7).forward+speed*p/rate;
  if(std::abs(world-weight*stride)>1e-9)return false;
 }
 // WORLD swing travel must never move backwards despite local overshoot.
 for(int i=1;i<40;i++){
  const double p=.6+static_cast<double>(i)*.01;
  const double velocity=speed+weight*rate*(step(p+e,stride,7).forward-step(p-e,stride,7).forward)/(2*e);
  if(velocity<-.02)return false;
 }
 return true;
}
double ElbowBend(HalvethMotion::V offset,double upper,double lower){
 auto result=HalvethMotion::Solve({0,0,0},offset,{1,0,0},upper,lower);
 return std::acos(std::clamp(result.knee.unit().dot((result.end-result.knee).unit()),-1.,1.));
}
void CandidateChecks(){
 using namespace HalvethMotion;
 for(double stride:{0.,12.,24.,36.})for(double height:{0.,3.,7.,12.})
  assert(C1Boundaries(Footstep,stride,height));
 assert(!C1Boundaries(OldFootstep));
 assert(C1Boundaries(ZeroLocalVelocity)); // A local-only continuity test is insufficient.
 assert(!WorldContact(ZeroLocalVelocity,110,1));
 assert(!WorldContact(OldFootstep,110,1));
 for(double speed:{12.,84.,110.,200.,390.})for(double weight:{.09,.25,.6,1.})
  assert(WorldContact(Footstep,speed,weight));
 for(int i=-2000;i<=2000;i++){
  const auto step=Footstep(static_cast<double>(i)/997.,24,7);
  assert(std::isfinite(step.forward)&&std::isfinite(step.lift));
  assert(step.lift>=0&&step.lift<=7.000001);
  if(step.stance)assert(step.lift==0);
 }
 for(double upper:{20.,30.,37.})for(double lower:{18.,28.,35.}){
  for(double gesture:{0.,.5,1.})for(double forward:{-20.,3.,33.}){
   const auto offset=RelaxedHandOffset(upper,lower,forward,3,gesture);
   const auto solved=Solve({0,0,0},offset,{1,0,0},upper,lower);
   assert(solved.reachable&&(solved.end-offset).length()<1e-9);
   assert(std::abs((solved.knee).length()-upper)<1e-9);
   assert(std::abs((solved.end-solved.knee).length()-lower)<1e-9);
   assert(std::abs(ElbowBend(offset,upper,lower)-(25+20*gesture)*3.141592653589793/180.)<1e-10);
  }
 }
 assert(ElbowBend(RelaxedHandOffset(30,28,3,3),30,28)>.4);
 assert(ElbowBend({3,3,-58*.993},30,28)<.4); // Old almost-straight resting arm.
 // The exact spring handles real frame intervals and lag without overshoot.
 for(int hz:{24,30,60,144,240}){
  Spring hand;
  for(int i=0;i<hz;i++){
   hand.update(1,1./hz,11);
   assert(std::isfinite(hand.position)&&std::isfinite(hand.velocity));
   assert(hand.position>=0&&hand.position<=1);
  }
  Spring once;once.update(1,1,11);
  assert(std::abs(hand.position-once.position)<1e-12&&std::abs(hand.velocity-once.velocity)<1e-12);
 }
 Spring lag;lag.update(1,1./60,11);assert(lag.position>0&&lag.position<1);
 const Spring preserved=lag;
 for(double dt:{0.,-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
  lag.update(1,dt,11);assert(lag.position==preserved.position&&lag.velocity==preserved.velocity);
 }
 lag.update(std::numeric_limits<double>::quiet_NaN(),.1,11);
 assert(lag.position==preserved.position&&lag.velocity==preserved.velocity);
 lag.update(1,1e308,11);assert(lag.position==1&&lag.velocity==0);
 // Weight transfer is lateral only; the original support-height solve still
 // has to keep BOTH measured leg lengths and exact contact targets intact.
 Spring sway;
 for(int i=0;i<1000;i++){
  sway.update(1.15*std::sin(i*.017),1./60,9);
  V foot{24*std::sin(i*.01),7,8};
  V hip{sway.position,0,0};
  hip.z=SupportedHipHeight(hip,foot,43,42,.104719755);
  const auto supported=Solve(hip,foot,{1,0,0},43,42);
  assert(supported.reachable&&(supported.end-foot).length()<1e-9);
  assert(std::abs((supported.knee-hip).length()-43)<1e-9);
  assert(std::abs((supported.end-supported.knee).length()-42)<1e-9);
 }
}
}
int main(){
 using namespace HalvethMotion;
 CandidateChecks();
 double worst=0;
 for(int i=0;i<10000;i++){
   double t=i*.003;V start{0,0,80},goal{20*std::sin(t),5*std::cos(t),8+8*std::abs(std::sin(t*.7))};
   auto r=Solve(start,goal,{1,0,0},43,42);assert(r.reachable);
   worst=std::max(worst,(r.end-goal).length());
   assert(std::abs((r.knee-start).length()-43)<1e-9);
   assert(std::abs((r.end-r.knee).length()-42)<1e-9);
 }
 assert(worst<1e-9);
 // Hip support must keep feet reachable across stride width and elevation.
 for(int i=0;i<1000;i++){
   V goal{24*std::sin(i*.01),7,8+7*std::abs(std::sin(i*.021))};
   V hip{0,0,SupportedHipHeight({0,0,80},goal,43,42,.104719755)};
   auto supported=Solve(hip,goal,{1,0,0},43,42);
   assert(supported.reachable&&(supported.end-goal).length()<1e-9);
   V upper=supported.knee-hip,lower=supported.end-supported.knee;
   assert(std::abs(std::acos(upper.unit().dot(lower.unit()))-.104719755)<1e-10);
 }
 auto unreachable=Solve({0,0,0},{0,0,200},{1,0,0},40,40);assert(!unreachable.reachable);
 auto coincident=Solve({0,0,0},{0,0,0},{0,0,1},40,40);assert(!coincident.reachable&&std::isfinite(coincident.knee.x));
 Spring a,b;for(int i=0;i<30;i++)a.update(1,1./30);for(int i=0;i<144;i++)b.update(1,1./144);
 assert(std::abs(a.position-b.position)<1e-12);assert(std::abs(a.velocity-b.velocity)<1e-12);
 assert(std::abs(Footstep(.6-1e-9).forward-Footstep(.6+1e-9).forward)<1e-5);
 assert(std::abs(Footstep(1-1e-9).forward-Footstep(1+1e-9).forward)<1e-5);
 for(int i=0;i<1000;i++){auto f=Footstep(i*.001);assert(f.lift>=0&&f.lift<=7.000001);if(f.stance)assert(f.lift==0);}
 using namespace EarthDynamics;
 assert(std::abs(SupportNewtons(10,0,true)-98.0665)<1e-9);
 assert(std::abs(SupportNewtons(10,2,true)-118.0665)<1e-9);
 assert(SupportNewtons(10,0,false)==0);
 assert(SupportNewtons(10,-GravityMps2,true)==0);
 assert(std::abs(SoilDepthCm(225,0)-.5)<1e-12);
 assert(SoilDepthCm(225,225)<SoilDepthCm(225,0));
 assert(SoilDepthCm(1e9,0)<=.95);
 for(int identity=1;identity<=3;identity++){
   auto genes=HalvethBody::Profile::ForIdentity(identity);HalvethBody::State body;
   assert(std::abs(HalvethBody::MassBudget(genes.massKg).total()-genes.massKg)<1e-10);
   for(int i=0;i<1800;i++)body.advance(genes,genes.paceCmS,1./30);
   assert(body.muscleReserve<.11&&body.musclePower<.76&&body.breathsPerMinute>27&&body.heartRateBpm>134);
   const double reaction=body.reactionSeconds;
   for(int i=0;i<900;i++)body.advance(genes,0,1./30);
   assert(body.muscleReserve>.99&&body.musclePower>.99&&body.reactionSeconds<reaction);
 }
 for(int offset=0;offset<256;offset++){
   HalvethGenome::Digest before,after;
   for(int i=0;i<32;i++)before[i]=std::uint8_t(offset+i);
   auto symbols=HalvethGenome::Encode(before);assert(symbols.size()==128);
   assert(HalvethGenome::Decode(symbols,after)&&before==after);
   symbols[64]='X';auto preserved=after;
   assert(!HalvethGenome::Decode(symbols,after)&&after==preserved);
 }
 std::cout<<"MOTION_EARTH_MATH_PASS chain_samples=10000 endpoint_error_cm="<<worst<<" frame_partition_invariant=1 contact_phase_continuous=1 contact_velocity_C1=1 world_stance_fixed=1 old_and_zero_local_velocity_rejected=1 relaxed_elbow=1 finite_lag=1 lateral_support_preserved=1 mass_gravity_acceleration_support_and_soil=1\n";
}
