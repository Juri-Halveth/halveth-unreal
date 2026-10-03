#pragma once
#include "CoreMinimal.h"
#include "VoidSoul.h"

// All distances are centimetres in UE's Z-up domain. This is our own field;
// it has no dependency on a legacy level, Morrowind record or save.
namespace VoidField {
constexpr int32 Grid = 257;
constexpr double HalfSize = 30000.0;
constexpr double WaterZ = 0.0;
inline uint32 Hash(int32 X, int32 Y, const FVoidSoul& Soul, uint32 Channel=0) {
    uint32 H = uint32(X) * 374761393u + uint32(Y) * 668265263u + Channel * 1442695041u;
    for(uint32 Word:Soul.Words) { H=(H^Word)*1274126177u; H^=H>>13; }
    return H ^ (H >> 16);
}
inline double Fade(double X) { return X * X * X * (X * (X * 6.0 - 15.0) + 10.0); }
inline double Noise(double X, double Y, const FVoidSoul& Soul, uint32 Channel=0) {
    const int32 IX = FMath::FloorToInt(X), IY = FMath::FloorToInt(Y);
    const double U = Fade(X - IX), V = Fade(Y - IY);
    auto N = [&Soul,Channel](int32 A, int32 B) { return double(Hash(A, B, Soul,Channel) & 0xffffffu) / 8388607.5 - 1.0; };
    return FMath::Lerp(FMath::Lerp(N(IX, IY), N(IX + 1, IY), U), FMath::Lerp(N(IX, IY + 1), N(IX + 1, IY + 1), U), V);
}
inline double FBM(double X, double Y, const FVoidSoul& Soul) {
    double Value = 0.0, Amplitude = 0.5;
    for (uint32 I = 0; I < 5; ++I) { Value += Amplitude * Noise(X, Y, Soul, I * 701u); X *= 2.05; Y *= 2.05; Amplitude *= 0.5; }
    return Value;
}
inline double Height(double X, double Y, const FVoidSoul& Soul) {
    const double Radius = FMath::Sqrt(FMath::Square(X / 8500.0) + FMath::Square(Y / 6500.0));
    const double Basin = -530.0 * FMath::Exp(-1.7 * Radius * Radius);
    const double Rise = 620.0 * (1.0 - FMath::Exp(-0.7 * Radius * Radius));
    const double ReliefMask = FMath::Clamp((Radius - 0.65) / 2.5, 0.0, 1.0);
    const double Hills = 4200.0 * ReliefMask * FBM(X / 13000.0, Y / 13000.0, Soul);
    const double Ridge = 3100.0 * FMath::Exp(-FMath::Square((Y - 16000.0) / 6000.0)) * (0.7 + 0.3 * Noise(X / 4200.0, 8.0, Soul, 41u));
    const double Dunes = ReliefMask * 110.0 * FMath::Sin(X / 1200.0 + 1.5 * Noise(X / 9000.0, Y / 9000.0, Soul, 77u));
    return Basin + Rise + Hills + Ridge + Dunes;
}
inline FVector Normal(double X, double Y, const FVoidSoul& Soul) {
    const double E = 40.0;
    return FVector(-(Height(X + E, Y, Soul) - Height(X - E, Y, Soul)) / (2 * E), -(Height(X, Y + E, Soul) - Height(X, Y - E, Soul)) / (2 * E), 1).GetSafeNormal();
}
inline FVector4f Color(double X, double Y, const FVoidSoul& Soul) {
    const double H = Height(X, Y, Soul), Slope = 1.0 - Normal(X, Y, Soul).Z;
    const float Rock = FMath::Clamp(float(Slope * 4.5 + (H - 1650.0) / 3400.0), 0.0f, 0.92f);
    const float Grass = FMath::Clamp(float(1.0 - FMath::Abs(H - 170.0) / 700.0), 0.0f, 0.92f) * (1.0f - Rock);
    return FVector4f(Rock, Grass, 0, 1);
}
inline FVector Start(const FVoidSoul& Soul) { const double X = -5500.0, Y = -6000.0; return FVector(X, Y, Height(X, Y, Soul) + 220.0); }
}
