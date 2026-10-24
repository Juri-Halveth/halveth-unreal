// Copyright (c) 2026 Juri Janovski.
// SPDX-License-Identifier: LicenseRef-HALVETH-PIRL-2.0
// Exact terms and earlier-grant boundaries: LICENSES.md.
// Portable C++17 geometry, habitat and wind regression tests; no engine/assets.
#include "../Source/HALVETHRealms/Public/MeadowModel.h"

#include <array>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
using namespace HalvethMeadow;
using PointFunction = Vec (*)(const Blade&, double, double);
using WindFunction = Vec (*)(Vec, double, double, double, double);
using HabitatFunction = HabitatStyle (*)(double, double, double, double);
int checks = 0, failures = 0;

void Check(bool passed, const char* message)
{
    ++checks;
    if (!passed && ++failures <= 16)
        std::cerr << "FAIL: " << message << '\n';
}
Vec Sub(Vec a, Vec b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
double Length(Vec a) { return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z); }
bool Finite(Vec a) { return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z); }
bool Same(Vec a, Vec b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
bool Near(Vec a, Vec b) { return Length(Sub(a,b)) < 1e-9; }
bool Same(const Blade& a, const Blade& b)
{
    return Same(a.root,b.root) && a.height==b.height && a.width==b.width
        && a.bend==b.bend && a.angle==b.angle && a.phase==b.phase
        && a.curl==b.curl && a.twist==b.twist && a.lateral==b.lateral
        && a.age==b.age && a.form==b.form;
}
bool ParametersValid(const Blade& b)
{
    return Finite(b.root) && b.root.z==0.0
        && std::hypot(b.root.x,b.root.y)<=ClumpRadiusCm+1e-9
        && b.height>=4.0 && b.height<=24.0
        && (b.form==Form::GroundLeaf ? b.width>=0.60 && b.width<=1.20 : b.width>=0.18 && b.width<=0.55)
        && b.bend>=0.48*b.height && b.bend<=1.14*b.height
        && b.angle>=0.0 && b.angle<2.0*Pi && b.phase>=0.0 && b.phase<2.0*Pi
        && b.curl>=0.12 && b.curl<=0.78 && std::abs(b.twist)<=0.8
        && std::abs(b.lateral)<=0.12 && b.age>=0.0 && b.age<=1.0;
}
double TriangleAreaTwice(Vec a, Vec b, Vec c)
{
    const Vec u=Sub(b,a),v=Sub(c,a);
    return Length({u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x});
}
bool SurfaceValid(const Blade& b, PointFunction point)
{
    const Vec tip=point(b,1.0,0.0);
    if (!Near(point(b,0.0,0.0),b.root)
        || !Same(tip,point(b,1.0,-1.0)) || !Same(tip,point(b,1.0,1.0))
        || tip.z<=b.root.z || tip.z>b.root.z+b.height+1e-9
        || std::hypot(tip.x-b.root.x,tip.y-b.root.y)<0.47*b.height)
        return false;
    const int segments=Segments(b),vertexCount=VertexCount(b);
    std::array<Vec,2*BladeSegments+1> vertices{};
    double crown=0;
    for (int row=0;row<=segments;++row)
    {
        const double t=static_cast<double>(row)/segments;
        const Vec left=point(b,t,-1.0),centre=point(b,t,0.0),right=point(b,t,1.0);
        if (!Finite(left) || !Finite(centre) || !Finite(right)
            || centre.z<b.root.z-1e-9 || centre.z>b.root.z+b.height+1e-9
            || Length(Sub(left,right))>b.width+1e-9)
            return false;
        const Vec edgeMid={(left.x+right.x)*0.5,(left.y+right.y)*0.5,(left.z+right.z)*0.5};
        if (!Near(centre,edgeMid)) return false; // Reject the old centre crease.
        crown=std::max(crown,centre.z-b.root.z);
        if (row<segments)
        {
            vertices[2*row]=left;vertices[2*row+1]=right;
            const double next=static_cast<double>(row+1)/segments;
            const Vec nextCentre=point(b,next,0.0);
            const Vec lineMid={(centre.x+nextCentre.x)*0.5,(centre.y+nextCentre.y)*0.5,(centre.z+nextCentre.z)*0.5};
            if (Length(Sub(point(b,0.5*(t+next),0.0),lineMid))>0.06*b.height) return false;
        }
        else vertices[2*segments]=tip;
    }
    bool meshValid=true;
    int triangles=0;
    std::array<int,2*BladeSegments+1> uses{};
    ForEachTriangle(b,[&](int a,int c,int d){
        ++triangles;
        if (a<0 || c<0 || d<0 || a>=vertexCount || c>=vertexCount || d>=vertexCount)
        {
            meshValid=false;return;
        }
        ++uses[a];++uses[c];++uses[d];
        if (TriangleAreaTwice(vertices[a],vertices[c],vertices[d])<1e-7) meshValid=false;
    });
    if (!meshValid || triangles!=TriangleCount(b)) return false;
    for (int i=0;i<vertexCount;++i) if (uses[i]==0) return false;
    const Vec mid=point(b,0.5,0.0);
    const Vec chord={(b.root.x+tip.x)*0.5,(b.root.y+tip.y)*0.5,(b.root.z+tip.z)*0.5};
    return crown>=0.95*b.height && Length(Sub(mid,chord))>0.06*b.height;
}
bool RootsAnchored(const Blade& b, WindFunction wind)
{
    for (int sample=0;sample<128;++sample)
        for (double side : {-1.0,0.0,1.0})
            if (!Same(wind(BladePoint(b,0.0,side),0.0,sample*1.137,b.phase,b.height),Vec{0.0,0.0,0.0}))
                return false;
    // Quadratic envelope pins the root slope as well as the exact endpoint.
    const Vec close=wind(BladePoint(b,0.0001,0.0),0.0001,4.2,b.phase,b.height);
    return Length(close)<1e-7;
}
bool WindBounded(const Blade& b, WindFunction wind)
{
    const Vec tip=BladePoint(b,1.0,0.0);
    for (int sample=0;sample<64;++sample)
        for (double t : {0.0,0.001,0.25,0.5,0.9,1.0})
        {
            const Vec offset=wind(tip,t,(sample-32.0)*7.123,b.phase,b.height);
            if (!Finite(offset) || Length(offset)>MaxWindDisplacementCm
                || Length(offset)>0.0915*b.height*t*t+1e-9)
                return false;
        }
    return true;
}
bool ContinuousAt(WindFunction wind, Vec p, double phase, double height, double time, double dt)
{
    const Vec a=wind(p,1.0,time,phase,height),b=wind(p,1.0,time+dt,phase,height);
    return Finite(a) && Finite(b) && Length(Sub(a,b))<=0.12*height*dt+1e-9;
}
template<class Population> bool PopulationValid(const Population& blades)
{
    const std::size_t n=blades.size();
    std::array<int,4> forms{};
    std::size_t shortCount=0,downturned=0;
    for (const auto& b : blades)
    {
        if (!ParametersValid(b)) return false;
        if (b.height<=16.0) ++shortCount;
        const double dz=BladePoint(b,1.0,0.0).z-BladePoint(b,0.99,0.0).z;
        if (dz<0.0) ++downturned;
        ++forms[static_cast<std::size_t>(b.form)];
    }
    return shortCount*100>=n*85 && shortCount*100<=n*96
        && forms[0]*100>=static_cast<int>(n)*12 && forms[0]*100<=static_cast<int>(n)*15
        && downturned*100>=n*45
        && std::all_of(forms.begin(),forms.end(),[](int count){return count>0;});
}
template<std::size_t N> bool MeshBudgetValid(const std::array<Blade,N>& blades)
{
    std::size_t vertices=0,triangles=0;
    for (const auto& b : blades) { vertices+=VertexCount(b);triangles+=TriangleCount(b); }
    // Compare one candidate clump with the first native pass, including its
    // unused duplicate terminal vertices. Triangle ceiling is the old 2816.
    return N==256 && vertices<=2880 && triangles<=2816;
}
template<std::size_t N> bool RootCoverage(const std::array<Blade,N>& blades)
{
    std::array<std::size_t,8> rings{};
    std::array<std::size_t,4> quadrants{};
    for (const auto& b : blades)
    {
        const double r2=(b.root.x*b.root.x+b.root.y*b.root.y)/(ClumpRadiusCm*ClumpRadiusCm);
        ++rings[std::min<std::size_t>(7,static_cast<std::size_t>(r2*8))];
        ++quadrants[(b.root.x>=0.0?1U:0U)+(b.root.y>=0.0?2U:0U)];
    }
    return std::all_of(rings.begin(),rings.end(),[](std::size_t n){return n>=N/10 && n<=N/6;})
        && std::all_of(quadrants.begin(),quadrants.end(),[](std::size_t n){return n>=N/5;});
}
bool HabitatValid(HabitatFunction habitat)
{
    const auto lush=habitat(0.9,0.7,0.1,0.5);
    if (lush.density<0.85 || habitat(0.1,0.0,0.0,0.5).density!=0.0
        || habitat(0.9,0.7,0.9,0.5).density!=0.0)
        return false;
    double previous=0;
    for (int i=0;i<=1000;++i)
    {
        const auto h=habitat(i/1000.0,0.6,0.1,0.5);
        if (!std::isfinite(h.density) || h.density<previous-1e-9 || h.density>1.0
            || h.density-previous>0.005 || h.heightScale<0.90 || h.heightScale>1.02
            || h.widthScale<1.02 || h.widthScale>1.12 || h.vigour<0.0 || h.vigour>1.0)
            return false;
        previous=h.density;
    }
    return habitat(0.8,0.05,0.1,0.5).density<0.35*lush.density;
}
// Old tall straws and safe synthetic defects must be rejected by the same
// oracles that accept the candidate. No defective path is in production.
Vec StraightBlade(const Blade& b,double t,double side)
{
    const Vec tip=BladePoint(b,1.0,0.0);
    return {b.root.x+(tip.x-b.root.x)*t-std::sin(b.angle)*b.width*(1-t)*side*0.5,
            b.root.y+(tip.y-b.root.y)*t+std::cos(b.angle)*b.width*(1-t)*side*0.5,
            b.root.z+(tip.z-b.root.z)*t};
}
Vec UntaperedBlade(const Blade& b,double t,double side)
{
    Vec p=BladePoint(b,t,0.0);
    p.x-=std::sin(b.angle)*b.width*side*0.5;
    p.y+=std::cos(b.angle)*b.width*side*0.5;
    return p;
}
Vec CollapsedBlade(const Blade& b,double t,double) { return BladePoint(b,t,0.0); }
Vec CreasedBlade(const Blade& b,double t,double side)
{
    Vec p=BladePoint(b,t,side);p.z-=0.12*b.width*t*(1-t)*side*side;return p;
}
Vec DriftingRoot(Vec p,double t,double time,double phase,double height)
{
    Vec offset=Wind(p,t,time,phase,height);offset.x+=0.25;return offset;
}
Vec ExcessiveWind(Vec,double t,double,double,double) { return {8.0*t*t,0.0,0.0}; }
Vec SteppedWind(Vec,double t,double time,double,double) { return {(time<0.5?-1.0:1.0)*t*t,0.0,0.0}; }
HabitatStyle HardCutoff(double cover,double moisture,double rock,double patch)
{
    auto h=Habitat(cover,moisture,rock,patch);h.density=cover<0.43?0.0:0.95;return h;
}
HabitatStyle Everywhere(double,double,double,double) { return {1.0,1.0,1.0,1.0}; }
}

int main()
{
    constexpr std::size_t variants=16;
    std::array<Blade,variants*BladesPerClump> blades{};
    std::array<Blade,BladesPerClump> clump{};
    std::array<double,variants*BladesPerClump> phases{};
    for (std::size_t v=0;v<variants;++v)
    {
        for (std::uint32_t i=0;i<BladesPerClump;++i)
        {
            const Blade b=MakeBlade(static_cast<std::uint32_t>(v),i);
            const std::size_t n=v*BladesPerClump+i;
            blades[n]=b;clump[i]=b;phases[n]=b.phase;
            Check(ParametersValid(b),"bounded low dimensions, leaning forms and root radius");
            Check(SurfaceValid(b,BladePoint),"smooth narrow ribbons, no centre crease, shared topology without unused/degenerate vertices");
            Check(RootsAnchored(b,Wind),"root edges and slope remain pinned");
            Check(WindBounded(b,Wind),"tip sway stays below 9.15 percent of blade height and 2.2cm");
            const Vec tip=BladePoint(b,1.0,0.0);
            for (int frame=0;frame<8;++frame)
                for (double dt : {1.0/60.0,1.0/240.0})
                    Check(ContinuousAt(Wind,tip,b.phase,b.height,-1000.0+frame*137.23,dt),"continuous wind across 60/240Hz steps");
            for (double t : {0.0,0.25,0.5,1.0})
            {
                const Vec c=Albedo(b,t);
                Check(Finite(c) && c.x>0 && c.y>0 && c.z>0
                      && c.x<0.09 && c.y<0.08 && c.z<0.04
                      && c.y/c.x<2.0,"muted finite linear albedo, no fluorescent tip gradient");
            }
        }
        Check(RootCoverage(clump),"every clump fills equal-area rings and all directions without root clustering");
        Check(PopulationValid(clump),"each mesh variant contains mostly short leaves and several growth forms");
        Check(MeshBudgetValid(clump),"256 roots remain within 2880 positions and 2816 triangles per clump");
    }
    Check(PopulationValid(blades),"85-96 percent short floor leaves plus minority taller tufts and many hooked tips");
    Check(BladesPerClump>192,"more roots than the first native pass");
    for (std::size_t n=blades.size();n-->0;)
        Check(Same(blades[n],MakeBlade(static_cast<std::uint32_t>(n/BladesPerClump),static_cast<std::uint32_t>(n%BladesPerClump))),
              "reverse-order deterministic replay without global random state");
    for (std::uint32_t value : {0U,1U,0x80000000U,std::numeric_limits<std::uint32_t>::max()})
    {
        const Blade b=MakeBlade(value,value);
        Check(ParametersValid(b) && Same(b,MakeBlade(value,value)),"full uint32 seed range remains finite and repeatable");
        Check(!Same(b,MakeBlade(value^0x80000000U,value)) && !Same(b,MakeBlade(value,value^0x80000000U)),"all seed bits participate");
    }
    std::sort(phases.begin(),phases.end());
    Check(std::adjacent_find(phases.begin(),phases.end())==phases.end(),"no duplicate phase in declared population");
    const Blade b=blades[0];const Vec tip=BladePoint(b,1.0,0.0);
    Check(Near(Wind(tip,1.0,3.2,b.phase,20.0),
               {Wind(tip,1.0,3.2,b.phase,4.0).x*5,Wind(tip,1.0,3.2,b.phase,4.0).y*5,Wind(tip,1.0,3.2,b.phase,4.0).z*5}),
          "short blades have proportionally smaller sway, not old fixed 8cm motion");
    Check(Same(Wind(tip,1.0,3.2,b.phase,0.0),Vec{0,0,0}),"zero-height wind stays still");
    Check(Same(Wind(tip,-1.0,3.0,b.phase,b.height),Vec{0,0,0})
          && Same(Wind(tip,2.0,3.0,b.phase,b.height),Wind(tip,1.0,3.0,b.phase,b.height)),"wind parameter saturation");
    Check(Same(BladePoint(b,-1.0,-2.0),BladePoint(b,0.0,-1.0))
          && Same(BladePoint(b,2.0,2.0),BladePoint(b,1.0,1.0)),"geometry parameter saturation");
    Check(HabitatValid(Habitat),"continuous cover transition, moisture response and bare rock exclusions");
    Check(!HabitatValid(HardCutoff),"negative challenge rejects old abrupt grass boundary");
    Check(!HabitatValid(Everywhere),"negative challenge rejects grass on bare dry or rocky ground");
    Check(!SurfaceValid(b,StraightBlade),"negative challenge rejects straight flat straws");
    Check(!SurfaceValid(b,UntaperedBlade),"negative challenge rejects missing taper/tip");
    Check(!SurfaceValid(b,CollapsedBlade),"negative challenge rejects collapsed sheet");
    Check(!SurfaceValid(b,CreasedBlade),"negative challenge rejects the old hard longitudinal fold");
    Check(!RootsAnchored(b,DriftingRoot),"negative challenge rejects root drift");
    Check(!WindBounded(b,ExcessiveWind),"negative challenge rejects old radiation-sized sway");
    Check(!ContinuousAt(SteppedWind,tip,b.phase,b.height,0.5-1.0/240.0,1.0/120.0),"negative challenge rejects wind jumps");
    // Keep the large negative populations off Windows' default 1MiB stack.
    std::vector<Blade> changed(blades.begin(),blades.end());
    for (std::size_t i=0;i<changed.size();++i) changed[i].height=18.0+47.0*static_cast<double>(i%100)/99.0;
    Check(!PopulationValid(changed),"negative challenge rejects original 18-65cm height population");
    changed.assign(blades.begin(),blades.end());
    for (auto& blade : changed) { blade.bend=0;blade.curl=0; }
    Check(!PopulationValid(changed),"negative challenge rejects all-upright needles");
    changed.assign(blades.begin(),blades.end());
    for (auto& blade : changed) blade.width=1.5;
    Check(!PopulationValid(changed),"negative challenge rejects the first-pass broad grass population");
    auto broad=clump;
    for (auto& blade : broad) { blade.form=Form::GroundLeaf;blade.width=0.9; }
    Check(!PopulationValid(broad),"negative challenge rejects too many broad floor leaves");
    Check(!MeshBudgetValid(broad),"negative challenge rejects six segments on every blade exceeding the position budget");
    for (auto& blade : clump) { blade.root.x*=0.1;blade.root.y*=0.1; }
    Check(!RootCoverage(clump),"negative challenge rejects concentrated roots with empty disk rim");
    std::size_t shortCount=0,hookCount=0;double sum=0,max=0;
    for (const auto& blade : blades)
    {
        if (blade.height<=16) ++shortCount;
        if (blade.curl>1.0/3.0) ++hookCount;
        sum+=blade.height;max=std::max(max,blade.height);
    }
    int positions=0,triangles=0;
    for (const auto& blade : clump) { positions+=VertexCount(blade);triangles+=TriangleCount(blade); }
    std::cout<<"HalvethMeadow: "<<checks<<" checks, "<<failures<<" failures; "<<blades.size()
             <<" blades, short="<<shortCount<<", downturned="<<hookCount
             <<", mean_height_cm="<<sum/blades.size()<<", max_height_cm="<<max
             <<", positions_per_clump="<<positions<<", blade_triangles_per_clump="<<triangles<<".\n";
    return failures==0?0:1;
}
