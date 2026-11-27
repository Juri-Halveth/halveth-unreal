#pragma once
#include "CoreMinimal.h"

// Authored landscape coordinates are centimetres, Z-up. The arrival clearing
// preserves the gameplay positions; the rest is continuous walkable terrain.
namespace GardenField {
constexpr int32 Grid = 257;
constexpr double HalfSize = 30000.0;
inline double Fade(double X) { return X*X*X*(X*(X*6-15)+10); }
inline double Noise(double X, double Y, int32 Realm) {
    int32 IX=FMath::FloorToInt(X), IY=FMath::FloorToInt(Y);
    auto N=[Realm](int32 A,int32 B) {
        uint32 H=uint32(A)*374761393u+uint32(B)*668265263u+uint32(Realm+1)*1442695041u;
        H=(H^(H>>13))*1274126177u; H^=H>>16;
        return double(H&0xffffffu)/8388607.5-1.0;
    };
    double U=Fade(X-IX),V=Fade(Y-IY);
    return FMath::Lerp(FMath::Lerp(N(IX,IY),N(IX+1,IY),U),FMath::Lerp(N(IX,IY+1),N(IX+1,IY+1),U),V);
}
inline double Height(double X,double Y,int32 Realm) {
    double Radius=FMath::Sqrt(X*X+Y*Y);
    double Blend=Fade(FMath::Clamp((Radius-2600.0)/4400.0,0.0,1.0));
    double Hills=1700.0*(.5+.5*Noise(X/11500.0,Y/11500.0,Realm));
    Hills+=450.0*(.5+.5*Noise(X/3600.0,Y/3600.0,Realm+4));
    double Ridge=3500.0*FMath::Exp(-FMath::Square((Y-21000.0)/6500.0))*(.65+.35*Noise(X/4600.0,8,Realm));
    double Lake=Realm==2 ? 2900.0*FMath::Exp(-FMath::Square((X-2500.0)/8000.0)-FMath::Square((Y-13000.0)/7000.0)) : 0.0;
    return Blend*(Hills+Ridge-Lake);
}
inline FVector Normal(double X,double Y,int32 Realm) {
    constexpr double E=40;
    return FVector(-(Height(X+E,Y,Realm)-Height(X-E,Y,Realm))/(2*E),-(Height(X,Y+E,Realm)-Height(X,Y-E,Realm))/(2*E),1).GetSafeNormal();
}
inline FVector4f Color(double X,double Y,int32 Realm) {
    float Rock=FMath::Clamp(float((1-Normal(X,Y,Realm).Z)*4.5+(Height(X,Y,Realm)-2300)/5000),0.0f,.85f);
    return FVector4f(Rock,.92f*(1-Rock),0,1);
}
}
