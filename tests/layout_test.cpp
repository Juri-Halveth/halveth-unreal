#include "../Source/HALVETHRealms/Public/RealmLayout.h"
#include <cassert>
#include <iostream>
#include <set>

int main()
{
    using namespace HalvethLayout;
    const std::uint32_t Seeds[] = { 0, 1, 127, 987654321u, 0xffffffffu };
    std::set<std::uint64_t> Fingerprints;
    for (std::uint32_t Seed : Seeds)
    {
        for (int Realm = 0; Realm < RealmCount; ++Realm)
        {
            const auto A = Build(Seed, Realm);
            assert(A == Build(Seed, Realm));
            assert(Fingerprints.insert(Fingerprint(A)).second);
            for (const Prop& P : A)
            {
                assert(P.X * P.X + P.Y * P.Y < Radius * Radius);
                assert(P.X <= -585 || P.X >= 585);
                assert(P.Height >= 130 && P.Height <= 420);
                assert(P.Rotation >= 0 && P.Rotation < 360);
                assert(P.Variant >= 0 && P.Variant <= 2);
            }
        }
    }
    // Exactly six directed routes, all reversible through the hub.
    int Routes = 0;
    for (int From = -1; From <= RealmCount; ++From)
        for (int To = -1; To <= RealmCount; ++To)
        {
            if (!ValidRealm(From) || !ValidRealm(To)) assert(!CanTravel(From, To));
            if (From == To) assert(!CanTravel(From, To));
            if (CanTravel(From, To)) { ++Routes; assert(CanTravel(To, From)); }
        }
    assert(Routes == 6);
    assert((Build(127, -1) == std::array<Prop, PropCount>{}));
    assert((Build(127, 99) == std::array<Prop, PropCount>{}));
    constexpr std::uint64_t Expected127[] = {
        15480670318311666167ull, 15103696736554213914ull,
        8776491094132948516ull, 11426871535637314886ull
    };
    for (int Realm = 0; Realm < RealmCount; ++Realm)
        assert(Fingerprint(Build(127, Realm)) == Expected127[Realm]);
    std::cout << "HALVETH_LAYOUT_PASS: 20 seed/realm samples, 800 props, 6 reversible routes\n";
    for (int Realm = 0; Realm < RealmCount; ++Realm)
        std::cout << "seed=127 realm=" << Realm << " fingerprint=" << Fingerprint(Build(127, Realm)) << '\n';
}
