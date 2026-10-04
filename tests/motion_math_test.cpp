#include "../Source/HALVETHRealms/Public/GuideMotionMath.h"
#include "../Source/HALVETHRealms/Public/EarthDynamicsMath.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace HalvethMotion;
 double worst=0;
 for(int i=0;i<10000;i++){
   double t=i*.003;V start{0,0,80},goal{20*std::sin(t),5*std::cos(t),8+8*std::abs(std::sin(t*.7))};
   auto r=Solve(start,goal,{1,0,0},43,42);assert(r.reachable);
   worst=std::max(worst,(r.end-goal).length());
   assert(std::abs((r.knee-start).length()-43)<1e-9);
   assert(std::abs((r.end-r.knee).length()-42)<1e-9);
 }
 assert(worst<1e-9);
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
 std::cout<<"MOTION_EARTH_MATH_PASS chain_samples=10000 endpoint_error_cm="<<worst<<" frame_partition_invariant=1 contact_phase_continuous=1 mass_gravity_acceleration_support_and_soil=1\n";
}
