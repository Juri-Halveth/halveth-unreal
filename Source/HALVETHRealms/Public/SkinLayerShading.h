#pragma once
// SPDX-License-Identifier: MIT
namespace HalvethSkinLayers {
// Masks are a single LINEAR RGBA lookup in UV0, never world position.
inline constexpr const char* Composite=R"HLSL(
float strength=saturate(Strength);
float4 zones=saturate(Masks)*strength;
float3 color=Base;
color=lerp(color,color*LipTint,zones.r*.48);
color=lerp(color,color*CheekTint,zones.g*.22);
color=lerp(color,color*LidTint,zones.b*.23);
float roughness=lerp(Rough,.32,zones.r*.85);
roughness=lerp(roughness,.55,zones.g*.42);
roughness=lerp(roughness,.44,zones.b*.55);
roughness=lerp(roughness,.37,zones.a*.58);
return float4(saturate(color),clamp(roughness,.22,.82));
)HLSL";
}
