// Copyright (c) 2026 Juri Janovski.
// SPDX-License-Identifier: LicenseRef-HALVETH-PIRL-2.0
// Exact terms and earlier-grant boundaries: LICENSES.md.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

// Portable mesh/wind math only: centimetres, seconds and radians; Z is up.
// Reuse a fixed set of (variant, index) pairs to build instanced mesh variants.
// Triangulate samples of BladePoint with an opaque material; it is not an
// alpha-card implementation. Instance creation, flowers and materials belong
// to the caller. These functions allocate nothing and have no mutable globals.
namespace HalvethMeadow
{
struct Vec
{
    double x, y, z;
};

struct Blade
{
    Vec root;
    double height, width, bend, angle, phase;
};

inline constexpr double Pi = 3.141592653589793238462643383279502884;
inline constexpr double MaxWindDisplacementCm = 10.0;

namespace Detail
{
// SplitMix64: unsigned wraparound and the 53-bit conversion are fully defined.
// Do not use std::hash or implementation-specific random distributions here.
inline double NextUnit(std::uint64_t& state) noexcept
{
    state += UINT64_C(0x9e3779b97f4a7c15);
    std::uint64_t value = state;
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    value ^= value >> 31;
    return static_cast<double>(value >> 11) * (1.0 / 9007199254740992.0);
}
} // namespace Detail

// The entire uint32 pair seeds the blade, independent of call order and threads.
// Roots sample area uniformly inside a 45 cm disk, with root.z exactly zero.
// width is the FULL base width. height is vertical height, bend is tip lean.
// Replay is exact with the same floating-point environment; libm rounding can
// differ between platforms. Hash determinism is not a universal uniqueness proof.
inline Blade MakeBlade(std::uint32_t variant, std::uint32_t index) noexcept
{
    std::uint64_t state = (static_cast<std::uint64_t>(variant) << 32) | index;
    const double radius = 45.0 * std::sqrt(Detail::NextUnit(state));
    const double azimuth = 2.0 * Pi * Detail::NextUnit(state);
    return {{radius * std::cos(azimuth), radius * std::sin(azimuth), 0.0},
            18.0 + 47.0 * Detail::NextUnit(state),
            0.45 + 0.90 * Detail::NextUnit(state),
            4.0 + 18.0 * Detail::NextUnit(state),
            2.0 * Pi * Detail::NextUnit(state),
            2.0 * Pi * Detail::NextUnit(state)};
}

// t and side are saturated to [0,1] and [-1,1] respectively (finite inputs).
// The centreline has quadratic lean, plus a shallow transverse fold between
// the centre and edges. Sample several t rows and side=-1,0,+1 for a curved
// surface. All three terminal vertices coincide: use ONE tip rather than a
// final zero-width quad. Rendering both faces/thickness is the caller's choice.
inline Vec BladePoint(const Blade& blade, double t, double side) noexcept
{
    t = std::clamp(t, 0.0, 1.0);
    side = std::clamp(side, -1.0, 1.0);
    const double forwardX = std::cos(blade.angle);
    const double forwardY = std::sin(blade.angle);
    const double width = blade.width * (1.0 - t) * (1.0 - 0.35 * t);
    const double across = 0.5 * width * side;
    const double along = blade.bend * t * t + 0.15 * width * t * side * side;
    return {blade.root.x + forwardX * along - forwardY * across,
            blade.root.y + forwardY * along + forwardX * across,
            blade.root.z + blade.height * t};
}

// Returns an OFFSET, not a displaced position. position is the UNDEFORMED
// world position in cm, including the instance transform (not clump-local XY).
// Supply normalized blade height t and the stored per-blade phase. Both base
// edges stay anchored because t=0 returns exact zero, with zero root slope.
//
// Two sine harmonics: primary sway and a slower gust, which also modulates
// sway amplitude. Since |primary|,|gust|<=1 and weight<=1, the displacement
// norm is <=sqrt(8^2+3.5^2+0.6^2) < 8.76 cm, hence below the 10 cm contract.
// At fixed position/phase its time derivative has norm <16 cm/s; there are no
// frame-dependent updates. Inputs must be finite with finite phase arithmetic.
inline Vec Wind(Vec position, double t, double time, double phase) noexcept
{
    t = std::clamp(t, 0.0, 1.0);
    if (t == 0.0)
        return {0.0, 0.0, 0.0};
    const double primary = std::sin(time * 1.7 + position.x * 0.021
                                    + position.y * 0.017 + phase);
    const double gust = std::sin(time * 0.63 - position.x * 0.013
                                 + position.y * 0.009 + phase * 1.37);
    const double weight = t * t;
    return {weight * (8.0 * primary * (0.7 + 0.3 * gust)),
            weight * (3.5 * gust),
            weight * (0.6 * primary * gust)};
}

// Same algebra and constants as Wind, for the material author to copy. HLSL
// float versus C++ double permits rounding differences, not bitwise equality.
// Preserve t/phase as mesh attributes; obtain position before wind deformation.
inline constexpr const char* WindHLSL = R"hlsl(
float3 HalvethMeadowWind(float3 position, float t, float time, float phase)
{
    t = saturate(t);
    if (t == 0.0)
        return float3(0.0, 0.0, 0.0);
    float primary = sin(time * 1.7 + position.x * 0.021
                        + position.y * 0.017 + phase);
    float gust = sin(time * 0.63 - position.x * 0.013
                     + position.y * 0.009 + phase * 1.37);
    float weight = t * t;
    return float3(weight * (8.0 * primary * (0.7 + 0.3 * gust)),
                  weight * (3.5 * gust),
                  weight * (0.6 * primary * gust));
}
)hlsl";
} // namespace HalvethMeadow
