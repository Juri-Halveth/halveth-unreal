#include "../Source/HALVETHRealms/Public/LivingWorldModel.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

namespace {
void check(bool value,const char* message){
    if(!value){std::cerr<<"RESIDENT_CONTINUITY_FAIL "<<message<<'\n';std::exit(1);}
}
bool close(double a,double b){return std::abs(a-b)<1e-9;}
}

int main(){
    using namespace HalvethContinuity;
    Policy policy;
    check(policy.bodyBudget==18,"physical budget stays 18");
    check(policy.enterMetres<policy.exitMetres&&policy.exitMetres<policy.unseenCapMetres,
          "entry, exit and unseen-cap radii are separate");

    double outside=-1;
    check(!retire(1000,10,Visibility::InView,false,outside),"even a far visible body stays physical");
    check(!retire(1000,11,Visibility::Unknown,false,outside),"missing camera coverage cannot retire a body");
    check(!retire(65,12,Visibility::OutsideView,false,outside),"near bodies survive outside the view");
    check(!retire(66,13,Visibility::OutsideView,false,outside),"crossing exit radius starts a grace period");
    check(!retire(66,14.99,Visibility::OutsideView,false,outside),"unseen grace is not shortened");
    check(retire(66,15,Visibility::OutsideView,false,outside),"unseen distant retirement after grace");
    check(!retire(66,16,Visibility::InView,false,outside),"turning the camera back protects a body");
    check(outside<0,"visible observation resets the unseen timer");
    check(!retire(66,17,Visibility::OutsideView,false,outside),"new unseen interval cannot inherit old grace");
    check(retire(100,17.01,Visibility::OutsideView,false,outside),"unseen 100m cap bounds retention");
    check(!retire(100,18,Visibility::InView,false,outside),"100m cap cannot evict a visible body");
    check(!retire(100,19,Visibility::OutsideView,true,outside),"active interaction pins its guard");

    check(materialize(0,20,0,Visibility::InView,true,-1),"initial population may be visible");
    check(!materialize(0,20,1,Visibility::InView,false,-1),"later visible spawn is rejected");
    check(!materialize(0,20,1,Visibility::Unknown,false,-1),"later spawn needs known camera coverage");
    check(materialize(17,50,3,Visibility::OutsideView,false,-1),"last available slot accepts an unseen candidate");
    check(!materialize(18,1,3,Visibility::OutsideView,false,-1),"full budget defers even the nearest candidate");
    check(!materialize(18,1,3,Visibility::InView,true,-1),"boot cannot exceed the budget");
    check(!materialize(17,50.001,3,Visibility::OutsideView,false,-1),"entry radius is smaller than retirement radius");
    check(!materialize(17,40,12.99,Visibility::OutsideView,false,10),"retirement has a rematerialization cooldown");
    check(materialize(17,40,13,Visibility::OutsideView,false,10),"cooldown permits later unseen rematerialization");
    check(!materialize(17,40,9,Visibility::OutsideView,false,10),"backward clock does not reopen admission");

    // Old top-N logic discards id 17 when an id 18 candidate becomes nearer.
    // The production policy retains all visible incumbents and defers admission.
    struct Ranked {unsigned id;double rank;};
    std::vector<Ranked> legacy;
    for(unsigned id=0;id<18;id++)legacy.push_back({id,double(id)+10});
    legacy.push_back({18,0});
    std::sort(legacy.begin(),legacy.end(),[](auto a,auto b){return a.rank<b.rank;});
    legacy.resize(18);
    const bool legacyKeepsVisible=std::any_of(legacy.begin(),legacy.end(),[](auto r){return r.id==17;});
    check(!legacyKeepsVisible,"negative control reproduces immediate top-N despawn");
    std::size_t physical=18;
    for(unsigned id=0;id<18;id++){
        double unseen=-1;
        check(!retire(10+id,30,Visibility::InView,false,unseen),"rank pressure cannot retire a visible incumbent");
    }
    for(unsigned candidate=0;candidate<4096;candidate++)
        if(materialize(physical,1,30,Visibility::OutsideView,false,-1))physical++;
    check(physical==18,"4096 closer candidates cannot evict or exceed 18 physical bodies");
    physical=0;
    for(unsigned candidate=0;candidate<4096;candidate++)
        if(materialize(physical,10,0,Visibility::InView,true,-1))physical++;
    check(physical==18,"initial population also saturates exactly at the cap");

    auto people=HalvethLife::population(127,4096);
    check(people.size()==4096,"virtual population is preserved");
    auto guard=people[0];
    const auto firstGuardPosition=HalvethLife::materializationPosition(guard);
    check(close(firstGuardPosition.x,22)&&close(firstGuardPosition.y,-4.5),"first guard placement preserves the gate fixture");
    auto resident=people[8];resident.lastUpdate=10;
    const HalvethLife::Point physicalPosition{100,100};
    Pose snapshot;snapshot.heightMetres=12.5;snapshot.yawDegrees=73;snapshot.idleSeconds=42.5;snapshot.gaitPhase=17.625;
    auto stale=resident;stale.position=physicalPosition;
    check(HalvethLife::advance(stale,20),"legacy stale-clock control advances");
    check(std::hypot(stale.position.x-physicalPosition.x,stale.position.y-physicalPosition.y)>1,
          "negative control reproduces catch-up from time already physically simulated");
    check(HalvethLife::retirePhysical(resident,physicalPosition,20,snapshot),"physical state handoff succeeds");
    check(close(resident.lastUpdate,20)&&close(resident.retiredAt,20),"retirement synchronizes both clocks");
    check(HalvethLife::advance(resident,20),"immediate background update is allowed");
    check(close(resident.position.x,100)&&close(resident.position.y,100),"same-time background update cannot jump");
    check(close(resident.physicalPose.heightMetres,12.5)&&close(resident.physicalPose.yawDegrees,73),
          "saved height and yaw survive retirement");
    check(resident.wasPhysical,"guards and other return visits retain materialization history");
    const auto resumed=resume(resident.physicalPose,resident.retiredAt,23);
    check(close(resumed.idleSeconds,45.5)&&close(resumed.gaitPhase,17.625),"idle clock continues and saved gait phase is retained");
    check(close(resumed.yawDegrees,73),"resume does not reset to the default gate yaw");
    check(HalvethLife::retirePhysical(guard,{37,19},20,snapshot),"guard retires with its actual coordinates");
    check(HalvethLife::advance(guard,20.25),"retired guard continues its normal patrol model");
    const auto returnedGuardPosition=HalvethLife::materializationPosition(guard);
    check(close(returnedGuardPosition.x,guard.position.x)&&close(returnedGuardPosition.y,guard.position.y),
          "guard rematerialization uses its advanced saved position");
    check(std::hypot(returnedGuardPosition.x-22,returnedGuardPosition.y+4.5)>10,
          "guard rematerialization rejects the old fixed-gate reset");
    const auto before=resident.position;
    check(HalvethLife::advance(resident,20.25),"retired resident continues normal virtual motion");
    const double moved=std::hypot(resident.position.x-before.x,resident.position.y-before.y);
    check(moved>0&&moved<=.25*1.25+1e-9,"next cohort advances only newly elapsed time");
    const auto saved=resident;
    check(!HalvethLife::retirePhysical(resident,{200,200},19,snapshot),"backward retirement clock rejected");
    check(close(resident.position.x,saved.position.x)&&close(resident.lastUpdate,saved.lastUpdate),"invalid handoff leaves position and clock together");
    auto invalid=snapshot;invalid.gaitPhase=std::numeric_limits<double>::quiet_NaN();
    check(!HalvethLife::retirePhysical(resident,{200,200},30,invalid),"nonfinite pose cannot partially replace saved state");
    check(close(resident.position.x,saved.position.x)&&close(resident.lastUpdate,saved.lastUpdate),"rejected pose leaves state intact");

    // Every member remains eligible for virtual updates; this change does not
    // stop the remote population to preserve the physical representation.
    unsigned advanced=0;
    for(auto& person:people)advanced+=HalvethLife::advance(person,20);
    check(advanced==4096,"background population remains live");
    const double nan=std::numeric_limits<double>::quiet_NaN();
    check(!materialize(0,nan,0,Visibility::OutsideView,false,-1),"invalid distance cannot spawn");
    check(!retire(nan,20,Visibility::OutsideView,false,outside),"invalid distance cannot retire");
    std::cout<<"RESIDENT_CONTINUITY_PASS virtual=4096 physical_cap=18 visible_retirement=REJECTED "
             <<"visible_respawn=REJECTED legacy_top_n=REJECTED stale_clock=REJECTED "
             <<"entry_m=50 exit_m=65 unseen_cap_m=100 pose_checkpoint=PASS native_visibility=NOT_RUN\n";
}
