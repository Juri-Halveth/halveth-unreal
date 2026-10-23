// MIT. Same quaternion and star generator used by the native camera/sky.
#include "../Source/HALVETHRealms/Public/ViewSkyMath.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <set>
#include <utility>
int main(){
    using namespace HalvethView;
    Frame frame;int zenith=0,nadir=0;double lastZ=0,arc=0,clamped=0;
    bool wasUp=false,wasDown=false;
    for(int i=0;i<36000;i++){
        const auto before=frame.orientation;assert(frame.apply(0,1));
        arc+=angularDistance(before,frame.orientation);const auto f=rotate(frame.orientation,{1,0,0});
        const bool up=f.z>.99,down=f.z<-.99;
        if(up&&!wasUp)zenith++;
        if(down&&!wasDown)nadir++;
        wasUp=up;wasDown=down;lastZ=f.z;clamped=std::fmin(89.9,clamped+1);
    }
    assert(zenith==100&&nadir==100&&std::abs(lastZ)<1e-9);
    assert(std::abs(arc-200*Pi)<1e-6&&clamped<90); // Old clamp cannot pass the first pole.
    const auto before=frame.orientation;assert(!frame.apply(NAN,0));assert(angularDistance(before,frame.orientation)<1e-7);
    Frame partition;for(int i=0;i<1440;i++)partition.apply(0,.25);
    assert(angularDistance(partition.orientation,{})<1e-7);
    const auto field=stars(0x68414c56,1800),again=stars(0x68414c56,1800),other=stars(0x68414c57,1800);
    int north=0,south=0;std::set<std::pair<int,int>> angularCells;
    for(size_t i=0;i<field.size();i++){
        const auto p=field[i].direction;
        assert(std::abs(p.x*p.x+p.y*p.y+p.z*p.z-1)<1e-12);
        assert(p.x==again[i].direction.x&&p.z==again[i].direction.z);
        north+=p.z>0;south+=p.z<0;
        angularCells.insert({int(std::atan2(p.y,p.x)*1000),int(p.z*1000)});
    }
    assert(north>800&&south>800&&angularCells.size()>1790);
    assert(field[0].direction.x!=other[0].direction.x);
    std::cout<<"VIEW_SKY_PASS pitch_loops=100 zenith_crossings="<<zenith<<" nadir_crossings="<<nadir<<" clamp_counterexample=DETECTED stars=1800 north="<<north<<" south="<<south<<"\n";
}
