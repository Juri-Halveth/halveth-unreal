// SPDX-License-Identifier: MIT
#pragma once
#include "TissueModel.h"
// Eight RGBA32F texels per region: world centre/radius, three unit basis
// vectors, four world displacements. An authored carrier, not cell biology.
namespace HalvethTissueField {
inline constexpr int Regions=13,Stride=8,Texels=128;
struct Region {HalvethTissue::V centre;double radius=5;std::array<HalvethTissue::V,3> basis;std::array<HalvethTissue::V,4> displacement;};
inline HalvethTissue::V Sample(const std::array<Region,Regions>& field,HalvethTissue::V point){
    using namespace HalvethTissue;V result;double total=0;
    const V corners[4]={{1,1,1},{1,-1,-1},{-1,1,-1},{-1,-1,1}};
    for(const auto& r:field){const auto q=point-r.centre;const double d=Detail::Dot(q,q),weight=1/((d+100)*(d+100));total+=weight;const V p{Detail::Dot(q,r.basis[0])/r.radius,Detail::Dot(q,r.basis[1])/r.radius,Detail::Dot(q,r.basis[2])/r.radius};double w[4],sum=0;
        for(int k=0;k<4;k++){w[k]=std::clamp((1+Detail::Dot(p,corners[k]))*.25,0.,1.);sum+=w[k];}
        for(int k=0;k<4;k++)result=result+r.displacement[k]*(w[k]/sum*weight);
    }return result/total;
}
inline constexpr const char* Shader=R"HLSL(
if (Enabled < 0.5) return float3(0,0,0);
float3 result=float3(0,0,0);float total=0;
for(int k=0;k<13;k++) {
    float4 centre=Texture2DSampleLevel(Field,FieldSampler,float2((k*8+0.5)/128.0,0.5),0);
    float3 bx=Texture2DSampleLevel(Field,FieldSampler,float2((k*8+1.5)/128.0,0.5),0).xyz;
    float3 by=Texture2DSampleLevel(Field,FieldSampler,float2((k*8+2.5)/128.0,0.5),0).xyz;
    float3 bz=Texture2DSampleLevel(Field,FieldSampler,float2((k*8+3.5)/128.0,0.5),0).xyz;
    float3 q=Position-centre.xyz;
    float d=dot(q,q);float w=1.0/((d+100.0)*(d+100.0));total+=w;
    float3 p=float3(dot(q,bx),dot(q,by),dot(q,bz))/max(centre.w,0.1);
    float4 weights=saturate(float4(1+p.x+p.y+p.z,1+p.x-p.y-p.z,1-p.x+p.y-p.z,1-p.x-p.y+p.z)*0.25);
    weights/=max(dot(weights,float4(1,1,1,1)),0.0001);
    for(int n=0;n<4;n++)result+=Texture2DSampleLevel(Field,FieldSampler,float2((k*8+4+n+0.5)/128.0,0.5),0).xyz*(weights[n]*w);
}
return result*(Softness/total);
)HLSL";
}
