#pragma once
#include "GardenField.h"
namespace GardenSoil {
constexpr int Grid=65;
constexpr double TerrainStep=2*GardenField::HalfSize/(GardenField::Grid-1);
constexpr double Half=TerrainStep;
inline FVector Center(FVector Guide){return FVector(FMath::RoundToDouble(Guide.X/TerrainStep)*TerrainStep,FMath::RoundToDouble(Guide.Y/TerrainStep)*TerrainStep,0);}
inline bool Contains(FVector C,double X,double Y){return FMath::Abs(X-C.X)<Half-.001&&FMath::Abs(Y-C.Y)<Half-.001;}
inline bool Cutout(int Realm,double X,double Y){
    if(Contains(Center(FVector(-380,Realm==0?-530:-470,0)),X,Y))return true;
    return Realm==0&&(Contains(Center(FVector(380,0,0)),X,Y)||Contains(Center(FVector(-380,600,0)),X,Y));
}
}
