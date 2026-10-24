// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Juri Janovski
// g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic tests/tissue_model_test.cpp -o <temporary-executable>
#include "../Source/HALVETHRealms/Public/TissueModel.h"
#include <iostream>
#include <limits>

namespace {
using namespace HalvethTissue;
using Targets=std::array<V,4>;
const Targets Rest={V{-3,-2,-2},V{3,-2,-2},V{0,4,-2},V{0,0,4}};
int checks=0,failures=0;
void Check(bool value,const char* message){++checks;if(!value&&++failures<=20)std::cerr<<"FAIL: "<<message<<'\n';}
bool Near(double a,double b,double tolerance=1e-9){return std::abs(a-b)<=tolerance;}
bool SameV(V a,V b){return (a.x==b.x||(std::isnan(a.x)&&std::isnan(b.x)))
    &&(a.y==b.y||(std::isnan(a.y)&&std::isnan(b.y)))&&(a.z==b.z||(std::isnan(a.z)&&std::isnan(b.z)));}
bool Same(const Patch& a,const Patch& b){
    if(a.initialized!=b.initialized||a.pendingSeconds()!=b.pendingSeconds())return false;
    for(int i=0;i<4;i++)if(!SameV(a.position[i],b.position[i])||!SameV(a.velocity[i],b.velocity[i]))return false;
    const auto& x=a.diagnostics;const auto& y=b.diagnostics;
    return x.initializations==y.initializations&&x.acceptedCalls==y.acceptedCalls&&x.totalSubsteps==y.totalSubsteps
        &&x.lastSubsteps==y.lastSubsteps&&x.maxOffsetCm==y.maxOffsetCm&&x.restVolumeError==y.restVolumeError;
}
Targets Shift(const Targets& source,V delta){auto result=source;for(auto& p:result)p=p+delta;return result;}
Targets Motion(double t){return Shift(Rest,{3*std::sin(5*t),1.5*std::sin(2*t),.5*std::sin(4*t)});}
V Yaw(V p,double angle){return{std::cos(angle)*p.x-std::sin(angle)*p.y,std::sin(angle)*p.x+std::cos(angle)*p.y,p.z};}
Targets Rotate(const Targets& source,double angle,V shift={}){auto result=source;for(auto& p:result)p=Yaw(p,angle)+shift;return result;}
double Offset(const Patch& p,const Targets& target){double maximum=0;for(int i=0;i<4;i++)maximum=std::max(maximum,p.displacement(i,target).length());return maximum;}
double Difference(const Patch& a,const Patch& b){double maximum=0;for(int i=0;i<4;i++)maximum=std::max(maximum,(a.position[i]-b.position[i]).length());return maximum;}
struct Metrics {double maxOffset=0,maxVolume=0,horizontalLag=0;};
Patch RunSchedule(int fps,Metrics& metrics,bool jitter=false){
    Patch patch;patch.initialize(Rest);double time=0;int frame=0;
    constexpr double intervals[]={1./240,1./80,1./60,1./120,1./48};
    while(time<2.-1e-12){
        const double dt=std::min(2.-time,jitter?intervals[frame%5]:1./fps);time+=dt;frame++;
        const auto target=Motion(time);
        const bool accepted=patch.step(target,dt);Check(accepted,"moving targets accepted without reset");
        if(!accepted)break;
        metrics.maxOffset=std::max(metrics.maxOffset,Offset(patch,target));
        metrics.maxVolume=std::max(metrics.maxVolume,patch.relativeVolumeError(target));
        metrics.horizontalLag=std::max(metrics.horizontalLag,std::abs(patch.displacement(0,target).x));
        Check(patch.diagnostics.lastSubsteps<Patch::MaxSubsteps+1,"fixed-step budget");
        Check(patch.diagnostics.initializations==1,"no spontaneous initialization");
        for(int i=0;i<4;i++)Check(patch.position[i].finite()&&patch.velocity[i].finite(),"finite moving state");
    }
    Check(patch.diagnostics.totalSubsteps==240,"120 Hz ticks over two seconds");
    return patch;
}
Targets StartStop(double t,bool turn){
    // Continuous positions, deliberately abrupt velocity changes at start,
    // stop and reversal. Turning uses 190 cm/s + <8 cm/s rotational node speed.
    const double speed=turn?190.:199.9;
    double distance=0;
    if(t>=.25&&t<.75)distance=(t-.25)*speed;
    else if(t>=.75&&t<1.)distance=.5*speed;
    else if(t>=1.&&t<1.5)distance=(1.5-t)*speed;
    const double angle=turn?.6*std::sin(3*t):0.;
    return Rotate(Rest,angle,{distance,0,0});
}
double StartStopChecks(){
    double peak=0;
    for(int fps:{30,60,120,144,240})for(bool turn:{false,true}){
        Patch patch;patch.initialize(StartStop(0,turn));double time=0;
        while(time<2.-1e-12){
            const double dt=std::min(2.-time,1./fps);time+=dt;
            const auto targets=StartStop(time,turn);
            const Patch before=patch;
            const bool accepted=patch.step(targets,dt);
            Check(accepted,"<200 cm/s start/stop/reversal and rotation stay within 4 cm");
            if(!accepted){Check(Same(before,patch),"motion-bound failure remains atomic");break;}
            peak=std::max(peak,Offset(patch,targets));
            Check(patch.diagnostics.initializations==1,"start/stop never silently rebases");
            Check(patch.relativeVolumeError(targets)<.002,"start/stop volume retained");
        }
    }
    return peak;
}
void Reject(Patch& patch,const Targets& targets,double dt){
    const Patch before=patch;
    Check(!patch.step(targets,dt),"invalid/gap input rejected");
    Check(Same(patch,before),"rejection preserves public state and diagnostics");
    // A subsequent replay also checks retained private target/timing history.
    Patch replay=before;
    const bool a=patch.step(Rest,Patch::FixedDt),b=replay.step(Rest,Patch::FixedDt);
    Check(a==b&&Same(patch,replay),"rejection preserves private continuation");
}
void AccelerationChecks(){
    Patch zero;zero.initialize(Rest);
    for(int i=0;i<4;i++){zero.position[i]=zero.position[i]+V{.3,-.2,.4};zero.velocity[i]={2,-1,3};}
    for(int i=0;i<600;i++)Check(zero.stepWithAcceleration(Rest,Patch::FixedDt,{0,0,0}),"zero external acceleration settles without gravity");
    Check(Offset(zero,Rest)<1e-8&&zero.velocity[0].length()<1e-8,"zero force converges to zero displacement");
    Patch legacy,explicitGravity;legacy.initialize(Rest);explicitGravity.initialize(Rest);
    for(int i=0;i<120;i++){
        const auto targets=Motion(i*Patch::FixedDt);
        Check(legacy.step(targets,Patch::FixedDt)&&explicitGravity.stepWithAcceleration(targets,Patch::FixedDt,{0,0,-980}),"old and extended calls accepted");
        Check(Same(legacy,explicitGravity),"legacy gravity wrapper exactly unchanged");
    }
    Patch positive,negative;positive.initialize(Rest);negative.initialize(Rest);
    for(int i=0;i<600;i++){
        Check(positive.stepWithAcceleration(Rest,Patch::FixedDt,{500,0,0})
              &&negative.stepWithAcceleration(Rest,Patch::FixedDt,{-500,0,0}),"opposite acceleration directions accepted");
        Check((positive.displacement(0,Rest)+negative.displacement(0,Rest)).length()<1e-8,"direction reversal mirror control");
    }
    // Equal-mass attachment equilibrium is displacement = compliance * a.
    // 500 cm/s^2 -> .2 cm, catching accidental metres/seconds or extra gravity.
    Check(Near(positive.displacement(0,Rest).x,.2,1e-7)
          &&std::abs(positive.displacement(0,Rest).z)<1e-8,"centimetre acceleration unit/equilibrium check");
    for(int i=0;i<600;i++)Check(positive.stepWithAcceleration(Rest,Patch::FixedDt,{-500,0,0}),"live force reversal accepted");
    Check(Near(positive.displacement(0,Rest).x,-.2,1e-7),"force reversal recovers opposite equilibrium");
    Patch a,b;Targets rotated=Rest;
    auto rotate3=[](V p){return V{p.z,p.x,p.y};}; // Proper 3D rotation, not gravity-preserving yaw.
    for(auto& p:rotated)p=rotate3(p);
    a.initialize(Rest);b.initialize(rotated);
    for(int i=0;i<240;i++){
        const V force={300*std::sin(i*.03),-200,400};
        Check(a.stepWithAcceleration(Rest,Patch::FixedDt,force)
              &&b.stepWithAcceleration(rotated,Patch::FixedDt,rotate3(force)),"rotated external acceleration accepted");
        for(int n=0;n<4;n++)Check((rotate3(a.position[n])-b.position[n]).length()<1e-8,"full rotation covariance with force frame bound");
    }
    for(V force:{V{std::numeric_limits<double>::quiet_NaN(),0,0},V{0,std::numeric_limits<double>::infinity(),0},
                 V{10000.01,0,0},V{8000,8000,0},V{1e308,1e308,1e308}}){
        const Patch before=a;
        Check(!a.stepWithAcceleration(Rest,Patch::FixedDt,force)&&Same(a,before),"invalid force rejected unchanged");
        Patch replay=before;
        Check(a.stepWithAcceleration(Rest,Patch::FixedDt,{0,0,0})&&replay.stepWithAcceleration(Rest,Patch::FixedDt,{0,0,0})
              &&Same(a,replay),"force rejection retains private history");
    }
    Patch bound;bound.initialize(Rest);
    Check(bound.stepWithAcceleration(Rest,Patch::FixedDt,{10000,0,0}),"inclusive acceleration magnitude boundary");
    Patch unbound;const Patch before=unbound;
    Check(!unbound.stepWithAcceleration(Rest,Patch::FixedDt,{0,0,0})&&Same(unbound,before),"custom forcing cannot create readiness");
}
} // namespace

int main(){
    using namespace HalvethTissue;
    Check(Near((V{1,2,3}+V{2,1,0}).length(),std::sqrt(27.)),"vector addition/length");
    Check(SameV((V{1,2,3}*2.)/2.,V{1,2,3})&&V{}.finite(),"vector scaling and finite");
    Patch empty;Check(!empty.initialized&&!empty.step(Rest,Patch::FixedDt),"step never auto-initializes");
    Check(!empty.displacement(0,Rest).finite()&&std::isinf(empty.relativeVolumeError(Rest)),"unbound diagnostics explicit");
    Patch settled;settled.initialize(Rest);
    Check(settled.initialized&&settled.relativeVolumeError(Rest)==0,"explicit rest binding");
    for(int i=0;i<600;i++)Check(settled.step(Rest,Patch::FixedDt),"gravity settle accepted");
    Check(settled.diagnostics.initializations==1,"normal positive time never rebases");
    Check(settled.displacement(0,Rest).z<-.1&&settled.displacement(0,Rest).z>-.8,"visible bounded compliant gravity sag");
    Check(settled.velocity[0].length()<.01&&settled.relativeVolumeError(Rest)<1e-4,"damped equilibrium and conserved volume");

    Patch compressed;compressed.initialize(Rest);
    for(auto& p:compressed.position)p=p*.7+V{.4,0,0};
    Check(compressed.relativeVolumeError(Rest)>.5,"compressed challenge is material");
    for(int i=0;i<600;i++)Check(compressed.step(Rest,Patch::FixedDt),"compressed patch recovers without reset");
    Check(Difference(compressed,settled)<.01&&compressed.relativeVolumeError(Rest)<1e-4,"compression and displacement recover");
    Patch impulse=settled;impulse.velocity[3].x+=30.;
    double deformation=0;
    for(int i=0;i<12;i++){
        Check(impulse.step(Rest,Patch::FixedDt),"localized impulse accepted");
        deformation=std::max(deformation,std::abs((impulse.position[3]-impulse.position[0]).length()-(Rest[3]-Rest[0]).length()));
        Check(impulse.relativeVolumeError(Rest)<.002,"volume retained under localized deformation");
    }
    Check(deformation>.001,"actual nonrigid deformation, not one rigid transform");
    for(int i=0;i<600;i++)Check(impulse.step(Rest,Patch::FixedDt),"impulse settles");
    Check(Difference(impulse,settled)<.01,"localized deformation settles back");

    Metrics referenceMetrics;const Patch reference=RunSchedule(120,referenceMetrics);
    Check(referenceMetrics.horizontalLag>.02&&referenceMetrics.maxOffset>.1,"moving/inertial response is visible");
    Check(referenceMetrics.maxOffset<1.1&&referenceMetrics.maxVolume<.002,"normal motion bounded, volume conserved");
    // Safe rigid-copy negative control: it has zero lag and cannot pass the
    // positive inertial/deformation requirement, even though its volume is exact.
    Patch rigid;rigid.initialize(Motion(2.));
    Check(!(std::abs(rigid.displacement(0,Motion(2.)).x)>.02),"rigid target copy fails inertia oracle");
    for(int fps:{30,60,240}){
        Metrics metrics;const Patch other=RunSchedule(fps,metrics);
        Check(Difference(reference,other)<.06,"different frame schedules remain close");
        Check(metrics.maxOffset<1.1&&metrics.maxVolume<.002,"schedule bounds retained");
    }
    Metrics jitterMetrics;const Patch jitter=RunSchedule(120,jitterMetrics,true);
    Check(Difference(reference,jitter)<.06&&jitterMetrics.maxVolume<.002,"jittered schedule, interpolated targets");

    // Gravity-preserving rotation covariance (yaw), including translation.
    Patch a,b;a.initialize(Rest);b.initialize(Rotate(Rest,.73,{100,200,30}));
    double rotationError=0;
    for(int i=1;i<=240;i++){
        const auto target=Motion(i*Patch::FixedDt);
        Check(a.step(target,Patch::FixedDt)&&b.step(Rotate(target,.73,{100,200,30}),Patch::FixedDt),"rotated trajectory accepted");
        for(int n=0;n<4;n++)rotationError=std::max(rotationError,(Yaw(a.position[n],.73)+V{100,200,30}-b.position[n]).length());
    }
    Check(rotationError<1e-7,"rotation/translation covariance with unchanged gravity");
    Patch winding;Targets reversed=Rest;std::swap(reversed[1],reversed[2]);winding.initialize(reversed);
    Check(winding.step(reversed,.25)&&winding.relativeVolumeError(reversed)<1e-4,"both signed rest windings supported");
    Patch tilted;Targets tilt=Rest;
    for(auto& p:tilt){const double x=p.x;p.x=.6*p.x+.8*p.z;p.z=-.8*x+.6*p.z;}
    tilted.initialize(tilt);
    for(int i=0;i<240;i++)Check(tilted.step(tilt,Patch::FixedDt),"tilted rest patch bounded under world gravity");
    Check(Offset(tilted,tilt)<.8&&tilted.relativeVolumeError(tilt)<1e-4,"3D orientation retains volume and bounds");

    Patch timing;timing.initialize(Rest);
    Check(timing.step(Rest,Patch::FixedDt*.5)&&timing.diagnostics.lastSubsteps==0,"half tick retained");
    Check(timing.step(Rest,Patch::FixedDt*.5)&&timing.diagnostics.lastSubsteps==1,"two halves produce one fixed tick");
    Check(timing.step(Rest,.25)&&timing.diagnostics.lastSubsteps==30,"maximum accepted gap within budget");
    Check(timing.step(Rest,1e-12)&&timing.diagnostics.lastSubsteps==0,"tiny positive dt accepted, never reset");
    for(double dt:{0.,-1.,.250001,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})Reject(timing,Rest,dt);
    Targets invalid=Rest;invalid[0].x=std::numeric_limits<double>::quiet_NaN();Reject(timing,invalid,Patch::FixedDt);
    invalid=Rest;invalid[1]=invalid[0];Reject(timing,invalid,Patch::FixedDt);
    invalid=Rest;invalid[3].z=-2;Reject(timing,invalid,Patch::FixedDt);
    invalid=Rest;invalid[3].z=-2+1e-8;Reject(timing,invalid,Patch::FixedDt);
    Reject(timing,reversed,Patch::FixedDt);
    invalid=Shift(Rest,{100,0,0});Reject(timing,invalid,Patch::FixedDt); // Explicit teleport/rebase boundary.
    const Patch beforeBadInit=timing;timing.initialize(Targets{});
    Check(Same(timing,beforeBadInit),"degenerate initialize leaves existing patch unchanged");
    Check(!timing.displacement(-1,Rest).finite()&&!timing.displacement(4,Rest).finite(),"bad node index explicit");
    Check(std::isinf(timing.relativeVolumeError(Targets{})),"degenerate volume diagnostic safe");
    Patch collapsed=settled;for(auto& p:collapsed.position)p={0,0,0};
    const Patch beforeCollapse=collapsed;const bool recovered=collapsed.step(Rest,Patch::FixedDt);
    Check(recovered?collapsed.position[0].finite():Same(collapsed,beforeCollapse),"collapsed state safely recovers or rejects atomically");
    const auto rebaseCount=timing.diagnostics.initializations;timing.initialize(Rest);
    Check(timing.diagnostics.initializations==rebaseCount+1&&timing.pendingSeconds()==0,"explicit caller rebase recorded");

    const double startStopPeak=StartStopChecks();
    AccelerationChecks();
    std::cout<<"HALVETH_TISSUE "<<checks<<" checks, "<<failures<<" failures"
             <<"; max_normal_offset_cm="<<referenceMetrics.maxOffset
             <<" max_relative_volume_error="<<referenceMetrics.maxVolume
             <<" max_horizontal_lag_cm="<<referenceMetrics.horizontalLag
             <<" max_start_stop_offset_cm="<<startStopPeak
             <<" rotation_error_cm="<<rotationError<<'\n';
    return failures?1:0;
}
