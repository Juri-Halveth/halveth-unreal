#pragma once
// SPDX-License-Identifier: MIT
#include <array>
#include <cstdint>

namespace HalvethAppearance {
// Eight authored ADULT bodies. Stable seeds add restrained surface variation;
// a cosmetic phenotype is not a biological simulation or a personal identity.
inline constexpr std::array<const char*,8> Models={
    "Scarlet","Lucinet","Rachel","Tala","Orin","Ivo","Mara","Neri"};
inline constexpr std::array<const char*,4> Morphs={
    "IdentityJaw","IdentityNose","IdentityCheek","IdentityBrow"};
inline std::uint32_t mix(std::uint32_t x){
    x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return x;
}
inline double unit(std::uint32_t seed,std::uint32_t channel){
    return double(mix(seed+0x9e3779b9u*(channel+1))&0xffffffu)/16777215.;
}
struct Traits {
    std::uint32_t model=0;
    std::array<double,4> morph{};
    double heightScale=1,skinMultiplier=1,warmth=0;
};
inline Traits traits(std::uint32_t seed){
    Traits value;value.model=mix(seed)%Models.size();
    for(std::uint32_t i=0;i<value.morph.size();i++)value.morph[i]=unit(seed,i)*.8;
    value.heightScale=.93+.12*unit(seed,4);
    value.skinMultiplier=.90+.16*unit(seed,5);
    value.warmth=-.025+.05*unit(seed,6);
    return value;
}
}
