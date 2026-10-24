// SPDX-License-Identifier: MIT
#include "../Source/HALVETHRealms/Public/TissueField.h"
#include <cstdlib>
#include <iostream>
using HalvethTissue::V;
using HalvethTissueField::Region;
using Field=std::array<Region,HalvethTissueField::Regions>;
void Check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
V OldNearestTwo(Field f,V point){
    double d[2]={1e30,1e30};int id[2]={0,0};
    for(int i=0;i<HalvethTissueField::Regions;i++){const double v=(point-f[i].centre).length();if(v<d[0]){d[1]=d[0];id[1]=id[0];d[0]=v;id[0]=i;}else if(v<d[1]){d[1]=v;id[1]=i;}}
    const double a=1/std::pow(d[0]*d[0]+100.,2),b=1/std::pow(d[1]*d[1]+100.,2);
    return (f[id[0]].displacement[0]*a+f[id[1]].displacement[0]*b)/(a+b);
}
int main(){
    Field f;for(int i=0;i<HalvethTissueField::Regions;i++){f[i].centre={500.+i*30,0,0};f[i].basis={V{1,0,0},V{0,1,0},V{0,0,1}};for(auto& d:f[i].displacement)d={1,-2,.4};}
    const auto constant=HalvethTissueField::Sample(f,{0,0,0});Check((constant-V{1,-2,.4}).length()<1e-12,"partition must reproduce constant displacement");
    f[0].centre={0,1,0};f[1].centre={-2,0,0};f[2].centre={2,0,0};
    for(auto& d:f[0].displacement){d={0,0,0};}
    for(auto& d:f[1].displacement){d={0,2,0};}
    for(auto& d:f[2].displacement){d={0,-2,0};}
    const V left{-1e-8,0,0},right{1e-8,0,0};
    const double oldJump=(OldNearestTwo(f,left)-OldNearestTwo(f,right)).length();Check(oldJump>1,"legacy nearest-two discontinuity must be rejected");
    const double newJump=(HalvethTissueField::Sample(f,left)-HalvethTissueField::Sample(f,right)).length();Check(newJump<1e-7,"field must be continuous across a nearest-region rank exchange");
    const auto before=HalvethTissueField::Sample(f,{.2,.3,.4});for(auto& r:f)r.centre=r.centre+V{1200,-400,600};
    Check((before-HalvethTissueField::Sample(f,{1200.2,-399.7,600.4})).length()<1e-12,"field must preserve source/frame translation");
    for(int i=0;i<10000;i++){const auto value=HalvethTissueField::Sample(f,{1195+i*.001,-400,600});Check(value.finite()&&value.length()<3,"bounded field sweep");}
    std::cout<<"TISSUE_FIELD_PASS rank_exchange_jump_cm="<<newJump<<" legacy_jump_cm="<<oldJump<<" samples=10000 gpu_execution=NOT_RUN\n";
}
