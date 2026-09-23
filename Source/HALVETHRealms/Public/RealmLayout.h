#pragma once

#include <array>
#include <cstdint>

// This is the production layout algorithm. It has no Unreal or platform dependencies.
namespace HalvethLayout
{
    constexpr int RealmCount = 4;
    constexpr int PropCount = 40;
    constexpr int Radius = 2200;

    struct Prop
    {
        int X;
        int Y;
        int Height;
        int Rotation;
        int Variant;
        constexpr bool operator==(const Prop&) const = default;
    };

    class Stream
    {
        std::uint32_t State;
    public:
        explicit constexpr Stream(std::uint32_t Seed) : State(Seed ? Seed : 0x6d2b79f5u) {}
        constexpr std::uint32_t Next()
        {
            State ^= State << 13;
            State ^= State >> 17;
            State ^= State << 5;
            return State;
        }
        constexpr int Range(int Minimum, int Maximum)
        {
            return Minimum + static_cast<int>(Next() % static_cast<std::uint32_t>(Maximum - Minimum + 1));
        }
    };

    constexpr bool ValidRealm(int Realm) { return Realm >= 0 && Realm < RealmCount; }

    inline std::array<Prop, PropCount> Build(std::uint32_t Seed, int Realm)
    {
        std::array<Prop, PropCount> Result{};
        if (!ValidRealm(Realm)) return Result;
        Stream Random(Seed ^ (0x9e3779b9u * static_cast<std::uint32_t>(Realm + 1)));
        for (int Index = 0; Index < PropCount; ++Index)
        {
            // Five rows on either side of the clear north/south portal path.
            const int Side = Index % 2 == 0 ? -1 : 1;
            const int Row = (Index / 2) % 5;
            const int Column = Index / 10;
            Result[Index] = {
                Side * (650 + Column * 310 + Random.Range(-65, 65)),
                -1250 + Row * 600 + Random.Range(-75, 75),
                Random.Range(130, 420), Random.Range(0, 359), Random.Range(0, 2)
            };
        }
        return Result;
    }

    constexpr bool CanTravel(int From, int To)
    {
        return ValidRealm(From) && ValidRealm(To) && From != To && (From == 0 || To == 0);
    }

    // A small stable fingerprint for regression tests, not a cryptographic identity.
    inline std::uint64_t Fingerprint(const std::array<Prop, PropCount>& Props)
    {
        std::uint64_t Hash = 14695981039346656037ull;
        for (const Prop& P : Props)
        {
            const int Fields[] = { P.X, P.Y, P.Height, P.Rotation, P.Variant };
            for (int Value : Fields)
            {
                const auto Bits = static_cast<std::uint32_t>(Value);
                for (unsigned Shift = 0; Shift < 32; Shift += 8)
                {
                    Hash ^= (Bits >> Shift) & 0xffu;
                    Hash *= 1099511628211ull;
                }
            }
        }
        return Hash;
    }
}
