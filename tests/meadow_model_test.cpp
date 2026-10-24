// Copyright (c) 2026 Juri Janovski.
// SPDX-License-Identifier: LicenseRef-HALVETH-PIRL-2.0
// Exact terms and earlier-grant boundaries: LICENSES.md.

// Standalone C++17; no engine, assets, test framework or network required.
// Example: clang++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic
//          tests/meadow_model_test.cpp -o <temporary-executable>
#include "../Source/HALVETHRealms/Public/MeadowModel.h"

#include <array>
#include <iostream>
#include <limits>

namespace
{
using namespace HalvethMeadow;
using PointFunction = Vec (*)(const Blade&, double, double);
using WindFunction = Vec (*)(Vec, double, double, double);

int checks = 0;
int failures = 0;

void Check(bool passed, const char* description)
{
    ++checks;
    if (!passed && ++failures <= 12)
        std::cerr << "FAIL: " << description << '\n';
}

Vec Difference(Vec a, Vec b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
double Length(Vec a) { return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z); }
bool Finite(Vec a) { return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z); }
bool Same(Vec a, Vec b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
bool Near(double a, double b) { return std::abs(a - b) <= 1e-10; }
bool Near(Vec a, Vec b) { return Length(Difference(a, b)) <= 1e-10; }

bool Same(const Blade& a, const Blade& b)
{
    return Same(a.root, b.root) && a.height == b.height && a.width == b.width
        && a.bend == b.bend && a.angle == b.angle && a.phase == b.phase;
}

bool ParametersValid(const Blade& blade)
{
    return Finite(blade.root) && blade.root.z == 0.0
        && std::hypot(blade.root.x, blade.root.y) <= 45.0 + 1e-10
        && blade.height >= 18.0 && blade.height <= 65.0
        && blade.width >= 0.45 && blade.width <= 1.35
        && blade.bend >= 4.0 && blade.bend <= 22.0
        && blade.angle >= 0.0 && blade.angle < 2.0 * Pi
        && blade.phase >= 0.0 && blade.phase < 2.0 * Pi;
}

double TriangleAreaTwice(Vec a, Vec b, Vec c)
{
    const Vec u = Difference(b, a);
    const Vec v = Difference(c, a);
    return Length({u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z,
                   u.x * v.y - u.y * v.x});
}

bool SurfaceValid(const Blade& blade, PointFunction point)
{
    const Vec tip = point(blade, 1.0, 0.0);
    if (!Near(point(blade, 0.0, 0.0), blade.root)
        || !Near(tip.z - blade.root.z, blade.height)
        || !Near(std::hypot(tip.x - blade.root.x, tip.y - blade.root.y), blade.bend)
        || !Same(tip, point(blade, 1.0, -1.0))
        || !Same(tip, point(blade, 1.0, 1.0)))
        return false;

    double previousWidth = blade.width + 1.0;
    for (int row = 0; row <= 12; ++row)
    {
        const double t = static_cast<double>(row) / 12.0;
        const Vec left = point(blade, t, -1.0);
        const Vec centre = point(blade, t, 0.0);
        const Vec right = point(blade, t, 1.0);
        const double width = Length(Difference(left, right));
        if (!Finite(left) || !Finite(centre) || !Finite(right)
            || !Near(centre.z - blade.root.z, blade.height * t)
            || !Near(left.z, centre.z) || !Near(right.z, centre.z)
            || !(width < previousWidth)
            || (row == 0 && !Near(width, blade.width)))
            return false;
        previousWidth = width;
        if (row < 12)
        {
            const double nextT = static_cast<double>(row + 1) / 12.0;
            const Vec nextLeft = point(blade, nextT, -1.0);
            const Vec nextRight = point(blade, nextT, 1.0);
            // A strip has two triangles per row and ONE final tip triangle.
            if (TriangleAreaTwice(left, right, nextLeft) < 1e-7
                || (row < 11 && TriangleAreaTwice(right, nextRight, nextLeft) < 1e-7)
                || TriangleAreaTwice(left, centre, nextLeft) < 1e-7
                || TriangleAreaTwice(centre, right, nextRight) < 1e-7)
                return false;
        }
    }
    const Vec linearMid = {(blade.root.x + tip.x) * 0.5,
                           (blade.root.y + tip.y) * 0.5,
                           (blade.root.z + tip.z) * 0.5};
    const Vec centreMid = point(blade, 0.5, 0.0);
    const Vec leftMid = point(blade, 0.5, -1.0);
    const Vec rightMid = point(blade, 0.5, 1.0);
    const Vec edgeMid = {(leftMid.x + rightMid.x) * 0.5,
                         (leftMid.y + rightMid.y) * 0.5,
                         (leftMid.z + rightMid.z) * 0.5};
    return Length(Difference(centreMid, linearMid)) > 0.5
        && Length(Difference(centreMid, edgeMid)) > 0.001;
}

bool RootsAnchored(const Blade& blade, WindFunction wind)
{
    for (int sample = 0; sample < 1000; ++sample)
    {
        const double time = (static_cast<double>(sample) - 500.0) * 0.137;
        for (double side : {-1.0, 0.0, 1.0})
            if (!Same(wind(BladePoint(blade, 0.0, side), 0.0, time, blade.phase),
                      Vec{0.0, 0.0, 0.0}))
                return false;
    }
    return true;
}

bool WindBounded(const Blade& blade, WindFunction wind)
{
    const Vec tip = BladePoint(blade, 1.0, 0.0);
    for (int sample = 0; sample < 64; ++sample)
    {
        const double time = (static_cast<double>(sample) - 32.0) * 7.123;
        for (double t : {0.0, 0.001, 0.25, 0.5, 0.9, 1.0})
        {
            const Vec offset = wind(tip, t, time, blade.phase);
            if (!Finite(offset) || Length(offset) > MaxWindDisplacementCm
                || Length(offset) > 8.76 * t * t + 1e-10)
                return false;
        }
    }
    return true;
}

bool ContinuousAt(WindFunction wind, Vec position, double phase, double time, double dt)
{
    const Vec a = wind(position, 1.0, time, phase);
    const Vec b = wind(position, 1.0, time + dt, phase);
    return Finite(a) && Finite(b) && Length(Difference(a, b)) <= 16.0 * dt + 1e-10;
}

template <std::size_t N>
bool DistinctPhases(const std::array<Blade, N>& blades)
{
    std::array<double, N> phases{};
    for (std::size_t i = 0; i < N; ++i)
        phases[i] = blades[i].phase;
    std::sort(phases.begin(), phases.end());
    return std::adjacent_find(phases.begin(), phases.end()) == phases.end();
}

// Safe synthetic defects ensure the important oracles can actually reject bad
// geometry/wind. They are test-only and never enter the production helper.
Vec StraightBlade(const Blade& blade, double t, double side)
{
    Blade straight = blade;
    straight.bend = 0.0;
    Vec p = BladePoint(straight, t, side);
    p.x += std::cos(blade.angle) * blade.bend * t;
    p.y += std::sin(blade.angle) * blade.bend * t;
    return p;
}

Vec UntaperedBlade(const Blade& blade, double t, double side)
{
    Vec p = BladePoint(blade, t, 0.0);
    p.x -= std::sin(blade.angle) * 0.5 * blade.width * side;
    p.y += std::cos(blade.angle) * 0.5 * blade.width * side;
    return p;
}

Vec CollapsedBlade(const Blade& blade, double t, double) { return BladePoint(blade, t, 0.0); }
Vec DriftingRoot(Vec p, double t, double time, double phase)
{
    Vec offset = Wind(p, t, time, phase);
    offset.x += 0.25;
    return offset;
}
Vec ExcessiveWind(Vec, double t, double, double) { return {11.0 * t, 0.0, 0.0}; }
Vec SteppedWind(Vec, double t, double time, double)
{
    return {(time < 0.5 ? -8.0 : 8.0) * t * t, 0.0, 0.0};
}
} // namespace

int main()
{
    constexpr std::size_t variantCount = 16;
    constexpr std::size_t bladesPerVariant = 64;
    std::array<Blade, variantCount * bladesPerVariant> blades{};
    double minHeight = 65.0, maxHeight = 18.0;
    double minWidth = 1.35, maxWidth = 0.45;
    double minBend = 22.0, maxBend = 4.0;
    double minRadius = 45.0, maxRadius = 0.0;
    std::array<bool, 4> quadrants{};
    for (std::size_t v = 0; v < variantCount; ++v)
    {
        for (std::size_t i = 0; i < bladesPerVariant; ++i)
        {
            const Blade b = MakeBlade(static_cast<std::uint32_t>(v), static_cast<std::uint32_t>(i));
            blades[v * bladesPerVariant + i] = b;
            Check(ParametersValid(b), "disk, dimensions, angles and root Z bounds");
            Check(SurfaceValid(b, BladePoint), "curved tapered surface, full height and nonzero triangles");
            Check(RootsAnchored(b, Wind), "both base edges and centre pinned across 1000 times");
            Check(WindBounded(b, Wind), "finite wind, quadratic root envelope and bounded tip");
            const Vec tip = BladePoint(b, 1.0, 0.0);
            for (int frame = 0; frame < 16; ++frame)
                for (double dt : {1.0 / 60.0, 1.0 / 240.0})
                    Check(ContinuousAt(Wind, tip, b.phase, -1000.0 + frame * 137.23, dt),
                          "frame continuity at 60/240 Hz, including negative times");
            minHeight = std::min(minHeight, b.height); maxHeight = std::max(maxHeight, b.height);
            minWidth = std::min(minWidth, b.width); maxWidth = std::max(maxWidth, b.width);
            minBend = std::min(minBend, b.bend); maxBend = std::max(maxBend, b.bend);
            const double radius = std::hypot(b.root.x, b.root.y);
            minRadius = std::min(minRadius, radius); maxRadius = std::max(maxRadius, radius);
            quadrants[(b.root.x >= 0.0 ? 1U : 0U) + (b.root.y >= 0.0 ? 2U : 0U)] = true;
        }
    }

    // Replay in reverse order: unrelated calls must not advance a global RNG.
    for (std::size_t n = blades.size(); n-- > 0;)
        Check(Same(blades[n], MakeBlade(static_cast<std::uint32_t>(n / bladesPerVariant),
                                        static_cast<std::uint32_t>(n % bladesPerVariant))),
              "exact deterministic replay independent of call order");
    for (std::uint32_t value : {0U, 1U, 0x80000000U, std::numeric_limits<std::uint32_t>::max()})
    {
        const Blade b = MakeBlade(value, value);
        Check(ParametersValid(b) && Same(b, MakeBlade(value, value)), "full uint32 seed domain");
        Check(!Same(b, MakeBlade(value ^ 0x80000000U, value)), "variant high bits affect output");
        Check(!Same(b, MakeBlade(value, value ^ 0x80000000U)), "index high bits affect output");
    }
    Check(DistinctPhases(blades), "no shared phase in the declared 16 x 64 population");
    Check(maxHeight - minHeight > 44.0 && maxWidth - minWidth > 0.85 && maxBend - minBend > 17.0,
          "fixed variants span substantial dimension ranges");
    Check(minRadius < 5.0 && maxRadius > 44.0
          && std::all_of(quadrants.begin(), quadrants.end(), [](bool seen) { return seen; }),
          "disk distribution reaches centre, rim and all quadrants");

    const Blade b = blades[0];
    const Vec tip = BladePoint(b, 1.0, 0.0);
    const Vec distant = {tip.x + 137.0, tip.y - 71.0, tip.z};
    Check(Length(Difference(Wind(tip, 1.0, 2.25, b.phase), Wind(distant, 1.0, 2.25, b.phase))) > 0.1,
          "wind phase responds to world position");
    Check(Length(Difference(Wind(tip, 1.0, 2.25, b.phase), Wind(tip, 1.0, 2.25, b.phase + 1.0))) > 0.1,
          "wind phase responds to per-blade phase");
    Check(Same(Wind(tip, -1.0, 3.0, b.phase), Vec{0.0, 0.0, 0.0})
          && Same(Wind(tip, 2.0, 3.0, b.phase), Wind(tip, 1.0, 3.0, b.phase)), "wind t saturation");
    Check(Same(BladePoint(b, -1.0, -2.0), BladePoint(b, 0.0, -1.0))
          && Same(BladePoint(b, 2.0, 2.0), BladePoint(b, 1.0, 1.0)), "geometry parameter saturation");
    Check(!SurfaceValid(b, StraightBlade), "negative challenge rejects straight blade");
    Check(!SurfaceValid(b, UntaperedBlade), "negative challenge rejects missing taper/tip");
    Check(!SurfaceValid(b, CollapsedBlade), "negative challenge rejects degenerate triangles");
    Blade shortened = b;
    shortened.height = 17.0;
    Check(!ParametersValid(shortened), "negative challenge rejects out-of-contract height");
    Check(!RootsAnchored(b, DriftingRoot), "negative challenge rejects moving roots");
    Check(!WindBounded(b, ExcessiveWind), "negative challenge rejects excessive wind");
    Check(!ContinuousAt(SteppedWind, tip, b.phase, 0.5 - 1.0 / 240.0, 1.0 / 120.0),
          "negative challenge rejects frame discontinuity");
    auto sharedPhase = blades;
    sharedPhase[1].phase = sharedPhase[0].phase;
    Check(!DistinctPhases(sharedPhase), "negative challenge rejects phase reuse");

    std::cout << "HalvethMeadow: " << checks << " checks, " << failures
              << " failures; 1024 blades, 1000 root times each, 12 geometry segments, 60/240 Hz.\n";
    return failures == 0 ? 0 : 1;
}
