#pragma once
// SPDX-License-Identifier: MIT
#include <array>
#include <cstdint>
#include <string>
namespace HalvethGenome {
// Reversible 2-bit symbols for a virtual game genome. Gameplay semantics
// have their own mapping; this alphabet alone does not supply biology.
using Digest=std::array<std::uint8_t,32>;
inline std::string Encode(const Digest& bytes){
    const char bases[]="ACGT";std::string result;result.reserve(128);
    for(auto b:bytes)for(int shift:{6,4,2,0})result.push_back(bases[(b>>shift)&3]);
    return result;
}
inline bool Decode(const std::string& bases,Digest& bytes){
    if(bases.size()!=128)return false;Digest decoded{};
    for(std::size_t i=0;i<128;i++){
        unsigned value;
        switch(bases[i]){case 'A':value=0;break;case 'C':value=1;break;case 'G':value=2;break;case 'T':value=3;break;default:return false;}
        decoded[i/4]|=std::uint8_t(value<<(6-2*(i%4)));
    }
    bytes=decoded;return true;
}
}
