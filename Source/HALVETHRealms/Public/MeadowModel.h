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

enum class Form : std::uint8_t { GroundLeaf, Ribbon, Hooked, Tuft };

struct Blade
{
    Vec root;
    // height bounds the whole undeformed centreline, not just the lowered tip.
    double height, width, bend, angle, phase;
    double curl, twist, lateral, age;
    Form form;
};

inline constexpr double Pi = 3.141592653589793238462643383279502884;
inline constexpr std::uint32_t BladesPerClump = 256;
// Five longitudinal segments for fine grass, six for the broader floor leaves.
// Two edges per row and ONE tip replace the old folded three-column strip.
inline constexpr int BladeSegments = 6;
inline constexpr double ClumpRadiusCm = 48.0;
inline constexpr double MaxBladeHeightCm = 24.0;
inline constexpr double MaxWindDisplacementCm = 2.2;

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
// Radial stratification and jittered golden-angle directions avoid the large
// random holes of the old 128-blade disk. There are still four instanced mesh
// variants; 224 fine blades x 9 + 32 broad leaves x 11 = 2368 triangles.
// That is 2880 authored positions, exactly the former 192 x 15 budget.
// width is the FULL MAXIMUM width, bend is tip lean. These are authored growth
// forms and ages, not a botanical growth simulation.
// Replay is exact with the same floating-point environment; libm rounding can
// differ between platforms. Hash determinism is not a universal uniqueness proof.
inline Blade MakeBlade(std::uint32_t variant, std::uint32_t index) noexcept
{
    std::uint64_t state = (static_cast<std::uint64_t>(variant) << 32) | index;
    const double radius = ClumpRadiusCm * std::sqrt(
        (static_cast<double>(index % BladesPerClump) + Detail::NextUnit(state))
        / static_cast<double>(BladesPerClump));
    const double azimuth = static_cast<double>(index % BladesPerClump) * 2.399963229728653
        + 0.8 * Detail::NextUnit(state) + static_cast<double>(variant) * 0.71;
    const double choice = Detail::NextUnit(state);
    // One broad leaf per eight roots avoids the foreground becoming a bed of
    // wide paper shapes. Shape, height, direction and age still vary per leaf.
    const Form form = index % 8 == 0 ? Form::GroundLeaf
                    : choice < 0.45 ? Form::Ribbon : choice < 0.88 ? Form::Hooked : Form::Tuft;
    const double u = Detail::NextUnit(state), w = Detail::NextUnit(state);
    const double height = form == Form::Tuft ? 16.0 + 8.0 * u
                        : form == Form::GroundLeaf ? 4.0 + 6.0 * u : 7.0 + 9.0 * u;
    const double width = form == Form::GroundLeaf ? 0.60 + 0.60 * w
                       : form == Form::Ribbon ? 0.18 + 0.30 * w
                       : form == Form::Tuft ? 0.18 + 0.20 * w : 0.20 + 0.35 * w;
    const double bend = height * (0.48 + 0.66 * Detail::NextUnit(state));
    const double angle = 2.0 * Pi * Detail::NextUnit(state);
    const double phase = 2.0 * Pi * Detail::NextUnit(state);
    const double curl = form == Form::Hooked ? 0.54 + 0.24 * Detail::NextUnit(state)
                      : form == Form::GroundLeaf ? 0.36 + 0.17 * Detail::NextUnit(state)
                      : 0.12 + 0.18 * Detail::NextUnit(state);
    return {{radius * std::cos(azimuth), radius * std::sin(azimuth), 0.0},
            height, width, bend, angle, phase, curl,
            -0.8 + 1.6 * Detail::NextUnit(state),
            -0.12 + 0.24 * Detail::NextUnit(state), Detail::NextUnit(state), form};
}

// t and side are saturated to [0,1] and [-1,1] respectively (finite inputs).
// The centreline leans immediately, twists sideways and (for hooked leaves)
// turns DOWN at its tip. Each ribbon rolls smoothly through 3D instead of
// carrying the old hard centre fold. Width peaks near the base, then tapers to
// a true point. Triangulate only side=-1,+1 and a single terminal point;
// side=0 remains the centreline for contact/wind diagnostics, not a third mesh
// column. These are opaque curved surfaces, with no camera-facing geometry.
inline Vec BladePoint(const Blade& blade, double t, double side) noexcept
{
    t = std::clamp(t, 0.0, 1.0);
    side = std::clamp(side, -1.0, 1.0);
    const double forwardX = std::cos(blade.angle);
    const double forwardY = std::sin(blade.angle);
    // Both profiles have maximum one: broad leaves at t=1/3, fine grass at
    // t=1/6. This keeps the declared width cap on the actual surface as well.
    const double profile = blade.form == Form::GroundLeaf
        ? 1.5 * (1.0 - t) * (0.5 + 1.5 * t)
        : 1.2 * (1.0 - t) * (0.8 + 1.2 * t);
    const double width = blade.width * profile;
    const double across = 0.5 * width * side;
    const double along = blade.bend * t * (0.35 + 0.65 * t);
    const double lateral = blade.lateral * blade.height * std::sin(Pi * t) * t;
    const double transverseAngle = blade.angle + 0.45 * blade.twist * t;
    const double roll = blade.twist * t;
    const double horizontalAcross = across * std::cos(roll);
    // Normalize the arch so a nominal 8cm hooked leaf really reaches 8cm,
    // instead of shrinking to 4cm merely because its tip turns down.
    const double crown = blade.curl <= 1.0 / 3.0 ? 1.0 - blade.curl
                       : (2.0 / 3.0) * std::sqrt(1.0 / (3.0 * blade.curl));
    return {blade.root.x + forwardX * along - forwardY * lateral
                       - std::sin(transverseAngle) * horizontalAcross,
            blade.root.y + forwardY * along + forwardX * lateral
                       + std::cos(transverseAngle) * horizontalAcross,
            blade.root.z + blade.height * (t - blade.curl * t * t * t) / crown
                       + across * std::sin(roll)};
}

inline int Segments(const Blade& blade) noexcept
{
    return blade.form == Form::GroundLeaf ? BladeSegments : BladeSegments - 1;
}

inline int VertexCount(const Blade& blade) noexcept { return 2 * Segments(blade) + 1; }
inline int TriangleCount(const Blade& blade) noexcept { return 2 * Segments(blade) - 1; }

// The editor and the portable tests use the same topology. Indices are local
// to ONE blade; the callback may add its mesh's base index. No duplicate tip,
// unused centre column, final collapsed quad or degenerate UV-sized card.
template<class Emit> inline void ForEachTriangle(const Blade& blade, Emit emit)
{
    const int segments = Segments(blade);
    for (int row = 0; row < segments - 1; ++row)
    {
        const int a = 2 * row;
        emit(a, a + 2, a + 1);
        emit(a + 1, a + 2, a + 3);
    }
    emit(2 * segments - 2, 2 * segments, 2 * segments - 1);
}

// Linear albedo, with muted age variation instead of a bright identical tip
// gradient. The bottom blends into the dark litter, while older blades become
// olive/tan. Lighting, transmission and habitat tint are applied by the caller.
inline Vec Albedo(const Blade& blade, double t) noexcept
{
    t = std::clamp(t, 0.0, 1.0);
    const double dry = std::clamp((blade.age - 0.72) / 0.28, 0.0, 1.0);
    const double shade = 0.85 + 0.15 * blade.phase / (2.0 * Pi);
    const double lift = 0.68 + 0.32 * std::sin(0.5 * Pi * t);
    return {(0.038 + 0.040 * dry) * lift * shade,
            (0.069 - 0.007 * dry) * lift * shade,
            (0.022 + 0.011 * dry) * lift * shade};
}

struct HabitatStyle { double density, heightScale, widthScale, vigour; };

inline double Smooth(double low, double high, double value) noexcept
{
    const double t = std::clamp((value - low) / (high - low), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

// cover/moisture/rock are the existing terrain's authored weights, not measured
// ecology. Continuous occupancy replaces the hard 0.43 grass cutoff. Patch is
// a coherent [0,1] spatial input so neighbouring cells share quieter clearings.
inline HabitatStyle Habitat(double cover, double moisture, double rock, double patch) noexcept
{
    const double wet = Smooth(0.08, 0.55, moisture);
    const double vigour = Smooth(0.24, 0.78, cover) * wet;
    return {Smooth(0.22, 0.72, cover) * (1.0 - Smooth(0.25, 0.67, rock))
                * (0.28 + 0.72 * wet) * (0.88 + 0.12 * std::clamp(patch, 0.0, 1.0)),
            0.90 + 0.12 * vigour, 1.02 + 0.10 * vigour, vigour};
}

// Returns an OFFSET, not a displaced position. position is the UNDEFORMED
// world position in cm, including the instance transform (not clump-local XY).
// Supply normalized blade height t and the stored per-blade phase. Both base
// edges stay anchored because t=0 returns exact zero, with zero root slope.
//
// Coherent slow field plus small independent flutter. Tip displacement is at
// most 9.15% of the blade height, capped at 2.2cm. Short leaves therefore no
// longer whip about as far as long grass. Root offset and root slope are zero.
// The time derivative norm is <0.12 * height cm/s. Inputs must be finite.
inline Vec Wind(Vec position, double t, double time, double phase,
                double heightCm = MaxBladeHeightCm) noexcept
{
    t = std::clamp(t, 0.0, 1.0);
    if (t == 0.0)
        return {0.0, 0.0, 0.0};
    heightCm = std::clamp(heightCm, 0.0, MaxBladeHeightCm);
    const double primary = std::sin(time * 1.05 + position.x * 0.007
                                    + position.y * 0.005 + 0.16 * std::sin(phase));
    const double gust = std::sin(time * 0.39 - position.x * 0.004
                                 + position.y * 0.003 + phase);
    const double weight = t * t;
    return {heightCm * weight * (0.085 * primary * (0.78 + 0.22 * gust)),
            heightCm * weight * (0.033 * gust),
            heightCm * weight * (0.007 * primary * gust)};
}

// Same algebra and constants as Wind, for the material author to copy. HLSL
// float versus C++ double permits rounding differences, not bitwise equality.
// Preserve t/phase as mesh attributes; obtain position before wind deformation.
inline constexpr const char* WindHLSL = R"hlsl(
float3 HalvethMeadowWind(float3 position, float t, float time, float phase, float heightCm)
{
    t = saturate(t);
    if (t == 0.0)
        return float3(0.0, 0.0, 0.0);
    heightCm = clamp(heightCm, 0.0, 24.0);
    float primary = sin(time * 1.05 + position.x * 0.007
                        + position.y * 0.005 + 0.16 * sin(phase));
    float gust = sin(time * 0.39 - position.x * 0.004
                     + position.y * 0.003 + phase);
    float weight = t * t;
    return float3(heightCm * weight * (0.085 * primary * (0.78 + 0.22 * gust)),
                  heightCm * weight * (0.033 * gust),
                  heightCm * weight * (0.007 * primary * gust));
}
)hlsl";
} // namespace HalvethMeadow
