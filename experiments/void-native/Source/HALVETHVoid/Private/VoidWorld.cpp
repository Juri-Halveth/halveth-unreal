#include "VoidWorld.h"
#include "VoidField.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

AVoidWorld::AVoidWorld() {
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WorldOrigin"));
    RootComponent->SetMobility(EComponentMobility::Static);
    Terrain = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SeedTerrain"));
    Terrain->SetupAttachment(RootComponent);
    Terrain->SetMobility(EComponentMobility::Static);
    Terrain->SetCollisionProfileName(TEXT("BlockAll"));
}
void AVoidWorld::BeginPlay() {
    Super::BeginPlay();
    const FString Path = TEXT("/Game/VOID/Soul_")+Soul.ID+TEXT("/Terrain.Terrain");
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
    if (!Mesh) { UE_LOG(LogTemp, Error, TEXT("VOID_TERRAIN_MISSING %s: run VOID.sh new with this soulseed"), *Path); return; }
    Terrain->SetStaticMesh(Mesh); bTerrainReady = true;
    UStaticMeshComponent* Water = NewObject<UStaticMeshComponent>(this, TEXT("Lake"));
    Water->SetupAttachment(RootComponent);
    Water->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/VOID/Water.Water")));
    Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Water->SetRelativeLocation(FVector(0,0,VoidField::WaterZ));
    Water->RegisterComponent();
    AddAtmosphere(); AddNature();
    UE_LOG(LogTemp, Display, TEXT("VOID_WORLD_READY soul=%s trees=%d rocks=%d ferns=%d"), *Soul.ID, TreeCount, RockCount, FernCount);
}
void AVoidWorld::AddAtmosphere() {
    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>();
    auto* Light = Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
    Light->SetMobility(EComponentMobility::Movable);
    Sun->SetActorRotation(FRotator(-29, -42, 0));
    Light->SetIntensity(48000); Light->SetLightColor(FLinearColor(1.0f,.96f,.90f));
    Light->bAtmosphereSunLight = true;
    Light->SetLightSourceAngle(.7f);
    AActor* Atmos = GetWorld()->SpawnActor<AActor>();
    USkyAtmosphereComponent* SkyAtmos = NewObject<USkyAtmosphereComponent>(Atmos);
    Atmos->SetRootComponent(SkyAtmos); SkyAtmos->RegisterComponent();
    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>();
    Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->bRealTimeCapture = true;
    Sky->GetLightComponent()->SetIntensity(1.4f);
    AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>();
    Fog->SetActorLocation(FVector(0,0,-220));
    Fog->GetComponent()->SetFogDensity(.007f);
    Fog->GetComponent()->SetFogHeightFalloff(.15f);
    Fog->GetComponent()->SetVolumetricFog(true);
    APostProcessVolume* PP = GetWorld()->SpawnActor<APostProcessVolume>();
    PP->bUnbound = true;
    PP->Settings.bOverride_AutoExposureMethod = true;
    PP->Settings.AutoExposureMethod = AEM_Histogram;
    PP->Settings.bOverride_AutoExposureMinBrightness = true;
    PP->Settings.bOverride_AutoExposureMaxBrightness = true;
    PP->Settings.AutoExposureMinBrightness = 12.0f;
    PP->Settings.AutoExposureMaxBrightness = 12.0f;
    PP->Settings.bOverride_BloomIntensity = true; PP->Settings.BloomIntensity = .22f;
    PP->Settings.bOverride_VignetteIntensity = true; PP->Settings.VignetteIntensity = .12f;
}
void AVoidWorld::AddNature() {
    FString Raw;
    if (!FFileHelper::LoadFileToString(Raw, *(FPaths::ProjectSavedDir()/TEXT("VOID-imports.json")))) return;
    TSharedPtr<FJsonObject> Doc;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw), Doc) || !Doc.IsValid()) return;
    uint32 ScatterSeed=0; for(uint32 Word:Soul.Words) ScatterSeed=(ScatterSeed^Word)*1274126177u;
    FRandomStream RNG{int32(ScatterSeed)};
    auto Scatter = [this, &RNG, &Doc](const FString& Key, int32 Count, bool Solid) {
        const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
        if (!Doc->TryGetArrayField(Key, Items) || Items->IsEmpty()) return;
        TArray<UHierarchicalInstancedStaticMeshComponent*> Groups;
        for (const auto& Item : *Items) {
            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Item->AsString());
            if (!Mesh) continue;
            auto* Comp = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
            Comp->SetupAttachment(RootComponent); Comp->SetStaticMesh(Mesh);
            Comp->SetMobility(EComponentMobility::Static);
            Comp->SetCollisionEnabled(Solid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
            Comp->SetCollisionResponseToAllChannels(ECR_Block);
            Comp->SetCullDistances(0, 45000);
            Comp->RegisterComponent(); Groups.Add(Comp);
        }
        if(Groups.IsEmpty()) return;
        for(int32 I=0;I<Count;I++) {
            const double X=RNG.FRandRange(-27000,27000), Y=RNG.FRandRange(-27000,27000);
            const double Z=VoidField::Height(X,Y,Soul);
            const float Moist=VoidField::Color(X,Y,Soul).Y;
            if(Z<25 || VoidField::Normal(X,Y,Soul).Z<.68) continue;
            if(Key != TEXT("rock_moss_set_01") && (Moist<.3f || RNG.FRand()>Moist)) continue;
            if(FVector2D::Distance(FVector2D(X,Y),FVector2D(-5500,-6000))<600) continue;
            auto* C=Groups[RNG.RandRange(0,Groups.Num()-1)];
            float Scale=Key==TEXT("tree_small_02") ? RNG.FRandRange(1.8f,3.6f) : (Key==TEXT("fern_02") ? RNG.FRandRange(1.2f,3.4f) : RNG.FRandRange(.9f,4.0f));
            const FBox Bounds=C->GetStaticMesh()->GetBoundingBox();
            if(Solid) {
                // Keep the authored arrival/walk corridor clear of each rock's
                // actual footprint, rather than checking its displaced pivot.
                const double Radius=Bounds.GetExtent().Size2D()*Scale;
                const FVector Position(X,Y,0);
                if(FMath::PointDistToSegmentSquared(Position,FVector(-10500,-6000,0),FVector(-2500,-4500,0))<FMath::Square(Radius+800.0)) continue;
            }
            const FRotator Rotation(0,RNG.FRandRange(0,360),0);
            const FVector Center=Bounds.GetCenter();
            const FVector Anchor=Rotation.RotateVector(FVector(Center.X,Center.Y,Bounds.Min.Z)*Scale);
            double Ground=Z;
            if(Key==TEXT("rock_moss_set_01")) {
                // glTF sets can bake a displaced pivot into each variant.
                // Anchor the actual footprint, then embed the stone on slopes.
                const FVector E=Bounds.GetExtent()*Scale;
                for(const FVector2D Offset : {FVector2D(E.X,0),FVector2D(-E.X,0),FVector2D(0,E.Y),FVector2D(0,-E.Y)}) {
                    const FVector R=Rotation.RotateVector(FVector(Offset.X,Offset.Y,0));
                    Ground=FMath::Min(Ground,VoidField::Height(X+R.X,Y+R.Y,Soul));
                }
                Ground-=Bounds.GetSize().Z*Scale*.18;
            }
            C->AddInstance(FTransform(Rotation,FVector(X,Y,Ground)-Anchor-FVector(0,0,6),FVector(Scale)));
            if(Key==TEXT("tree_small_02")) TreeCount++;
            else if(Key==TEXT("fern_02")) FernCount++; else RockCount++;
        }
    };
    Scatter(TEXT("tree_small_02"),1900,false);
    Scatter(TEXT("fern_02"),18000,false);
    Scatter(TEXT("rock_moss_set_01"),600,true);
}
