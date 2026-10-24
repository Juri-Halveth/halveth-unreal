#include "../Source/HALVETHRealms/Public/LivingWorldModel.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <limits>
#include <set>
int main(){
    using namespace HalvethLife;
    Notice obey;assert(obey.observe({0,true,true,false})==Event::Warn);
    assert(obey.observe({2,true,false,false})==Event::None);
    assert(obey.observe({3.01,true,false,false})==Event::Resolved);assert(obey.holds==0);
    Notice interrupted;interrupted.observe({0,true,true,false});
    assert(interrupted.observe({2,true,false,false})==Event::None);
    assert(interrupted.observe({2.4,true,true,false})==Event::None);
    assert(interrupted.observe({6,true,true,false})==Event::Chase);
    Notice n;assert(n.observe({0,false,true,true})==Event::None); // A wall or lost sight prevents warning.
    assert(n.observe({1,true,true,true})==Event::Warn);
    assert(n.observe({6.9,true,true,true})==Event::None); // Real grace interval.
    assert(n.observe({7,true,true,false})==Event::Chase);
    assert(n.observe({8,false,true,true})==Event::None); // Nearby behind wall cannot be caught.
    assert(n.observe({9,true,true,true})==Event::Hold);
    assert(n.observe({10,true,true,true})==Event::None);
    assert(n.observe({11.21,true,true,true})==Event::Release);
    assert(n.observe({15,true,true,true})==Event::None);assert(n.holds==1);
    assert(n.observe({37,true,true,true})==Event::Warn);
    Notice lost;lost.observe({0,true,true,false});assert(lost.observe({9,false,true,true})==Event::Lost);
    Notice nan;assert(nan.observe({std::numeric_limits<double>::quiet_NaN(),true,true,true})==Event::None);
    auto people=population(127,25000),same=population(127,25000);std::set<uint32_t> ids;int roles[4]={0,0,0,0};
    const auto start=std::chrono::steady_clock::now();
    for(auto& r:people){ids.insert(r.id);roles[static_cast<int>(r.temper)]++;auto old=r.position;assert(advance(r,1));
        assert(std::hypot(r.position.x-old.x,r.position.y-old.y)<=1.51);assert(!advance(r,.5));}
    const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    assert(ids.size()==25000);for(auto count:roles)assert(count>5000);
    for(size_t i=0;i<people.size();i++){assert(people[i].seed==same[i].seed);assert(advance(same[i],1));assert(people[i].position.x==same[i].position.x);}
    int changes=0;for(auto& r:people){auto t=r.task;advance(r,36000);changes+=r.task!=t;}
    assert(changes>10000);
    std::cout<<"LIVING_MODEL_PASS residents=25000 routines_changed="<<changes<<" update_ms="<<ms<<" wall_control=REJECTED grace=6 hold=2.2 cooldown=25\n";
}
