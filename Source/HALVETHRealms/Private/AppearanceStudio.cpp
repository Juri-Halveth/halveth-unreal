// SPDX-License-Identifier: MIT
#include "HALVETHRealmWorld.h"
#include "HALVETHGuideCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

void AHALVETHRealmWorld::EnableAppearanceStudio(){
    if(AppearanceStudio)return;AppearanceStudio=true;
    // Isolated lighting CONTROL in an explicitly requested capture process.
    // Ordinary play keeps its sky, clock, portals, exposure and player camera.
    Sun->SetLightColor(FLinearColor::White);Sun->SetIntensity(10000);
    Sun->SetWorldRotation(FRotator(-35,180,0));Sun->SetLightSourceAngle(2.f);
    NightFill->SetIntensity(0);Sky->SetLightColor(FLinearColor::White);Sky->SetIntensity(.7f);
    Fog->SetVisibility(false);
    PostProcess->Settings.bOverride_FilmSlope=false;PostProcess->Settings.bOverride_FilmToe=false;PostProcess->Settings.bOverride_FilmShoulder=false;
    PostProcess->Settings.AutoExposureMinBrightness=12;PostProcess->Settings.AutoExposureMaxBrightness=12;
    PostProcess->Settings.BloomIntensity=0;PostProcess->Settings.VignetteIntensity=0;
    TArray<UPointLightComponent*> Points;GetComponents(Points);for(auto* Point:Points)Point->SetVisibility(false);
    TArray<UTextRenderComponent*> Labels;GetComponents(Labels);for(auto* Text:Labels)Text->SetVisibility(false);
    auto* Subject=GetGuideCharacter(1);if(!Subject)return;
    const FVector Base=Subject->GetMesh()->GetComponentLocation();
    auto* Back=Shape(CubeMesh,Base-GetActorLocation()+FVector(-90,0,160),FVector(.1f,12.f,6.f),FLinearColor(.18f,.18f,.18f),0,false);
    Back->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Characters/M_AppearanceBackdrop.M_AppearanceBackdrop")));Back->SetCastShadow(false);
    auto* Key=Light(Base-GetActorLocation()+FVector(110,-75,225),FLinearColor::White,1400,450);Key->SetSourceRadius(10);Key->SetCastShadows(true);
    auto* Fill=Light(Base-GetActorLocation()+FVector(90,90,165),FLinearColor::White,400,400);Fill->SetSourceRadius(22);
    UE_LOG(LogTemp,Display,TEXT("APPEARANCE_STUDIO neutral_control=true directional_lux=10000 key_lumens=1400 fill_lumens=400 exposure_ev=12 coloured_points=hidden clock=held backdrop_albedo=.18 fictional_adult_subject=Scarlet"));
}
