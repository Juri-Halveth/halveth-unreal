#include "HALVETHRealmWorld.h"
#include "GardenField.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

void AHALVETHRealmWorld::BuildLandscape(uint32 Seed) {
    TreeCount=FernCount=RockCount=0;
    FString TerrainPath=FString::Printf(TEXT("/Game/Garden/Terrain_R%d.Terrain_R%d"),CurrentRealm,CurrentRealm);
    UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*TerrainPath);
    if(!Mesh) { UE_LOG(LogTemp,Error,TEXT("GARDEN_TERRAIN_MISSING run GardenPrepare")); return; }
    TerrainComponent=Shape(Mesh,FVector::ZeroVector,FVector(1),FLinearColor::White);
    TerrainComponent->SetMaterial(0,Mesh->GetMaterial(0));
    TerrainComponent->SetMobility(EComponentMobility::Static);
    if(CurrentRealm==2) {
        if(auto* Water=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/VOID/Water.Water"))) {
            auto* Lake=Shape(Water,FVector(2500,13000,-180),FVector(1),FLinearColor::White,0,false);
            Lake->SetMaterial(0,Water->GetMaterial(0));
        }
    }
    FString Raw; TSharedPtr<FJsonObject> Doc;
    if(!FFileHelper::LoadFileToString(Raw,*(FPaths::ProjectSavedDir()/TEXT("Garden-imports.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw),Doc) || !Doc.IsValid()) {
        UE_LOG(LogTemp,Error,TEXT("GARDEN_NATURE_MANIFEST_MISSING")); return;
    }
    FRandomStream RNG(int32(Seed)^int32((CurrentRealm+1)*7919));
    auto Scatter=[this,&RNG,&Doc](const FString& Key,int32 Attempts,bool Solid) {
        const TArray<TSharedPtr<FJsonValue>>* Items=nullptr;
        if(!Doc->TryGetArrayField(Key,Items)) return;
        TArray<UHierarchicalInstancedStaticMeshComponent*> Groups;
        for(const auto& Item:*Items) {
            UStaticMesh* Asset=LoadObject<UStaticMesh>(nullptr,*Item->AsString()); if(!Asset) continue;
            auto* C=NewObject<UHierarchicalInstancedStaticMeshComponent>(this); AddInstanceComponent(C);
            C->SetupAttachment(RootComponent); C->SetStaticMesh(Asset); C->SetMobility(EComponentMobility::Static);
            C->SetCollisionEnabled(Solid?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
            C->SetCollisionResponseToAllChannels(ECR_Block); C->SetCullDistances(0,45000);
            C->RegisterComponent(); Generated.Add(C); Groups.Add(C);
        }
        if(Groups.IsEmpty()) return;
        for(int32 I=0;I<Attempts;I++) {
            double X=RNG.FRandRange(-28500,28500),Y=RNG.FRandRange(-28500,28500);
            double Z=GardenField::Height(X,Y,CurrentRealm);
            if(Z<0 || GardenField::Normal(X,Y,CurrentRealm).Z<.72) continue;
            auto* C=Groups[RNG.RandRange(0,Groups.Num()-1)];
            float Scale=Key==TEXT("tree_small_02")?RNG.FRandRange(2.4f,4.8f):Key==TEXT("fern_02")?RNG.FRandRange(1.0f,2.5f):RNG.FRandRange(1.0f,4.8f);
            FBox Bounds=C->GetStaticMesh()->GetBoundingBox();
            double Footprint=Bounds.GetExtent().Size2D()*Scale;
            if(FVector2D(X,Y).Size()<2500+Footprint) continue;
            // A broad corridor leads out of the original interaction clearing.
            if(FMath::Abs(X)<650+Footprint && Y<10000 && Y>0) continue;
            FRotator Rotation(0,RNG.FRandRange(0,360),0);
            FVector Center=Bounds.GetCenter();
            FVector Anchor=Rotation.RotateVector(FVector(Center.X,Center.Y,Bounds.Min.Z)*Scale);
            if(Key==TEXT("rock_moss_set_01")) {
                FVector E=Bounds.GetExtent()*Scale;
                for(FVector2D Offset:{FVector2D(E.X,0),FVector2D(-E.X,0),FVector2D(0,E.Y),FVector2D(0,-E.Y)}) {
                    FVector R=Rotation.RotateVector(FVector(Offset.X,Offset.Y,0));
                    Z=FMath::Min(Z,GardenField::Height(X+R.X,Y+R.Y,CurrentRealm));
                }
                Z-=Bounds.GetSize().Z*Scale*.18;
            }
            C->AddInstance(FTransform(Rotation,FVector(X,Y,Z)-Anchor-FVector(0,0,4),FVector(Scale)));
            if(Key==TEXT("tree_small_02")) TreeCount++; else if(Key==TEXT("fern_02")) FernCount++; else RockCount++;
        }
    };
    Scatter(TEXT("tree_small_02"),CurrentRealm==1?1700:1050,false);
    Scatter(TEXT("fern_02"),15000,false);
    Scatter(TEXT("rock_moss_set_01"),550,true);
    UE_LOG(LogTemp,Display,TEXT("GARDEN_LANDSCAPE_READY realm=%d extent_m=600 trees=%d ferns=%d rocks=%d"),CurrentRealm,TreeCount,FernCount,RockCount);
}

bool AHALVETHRealmWorld::VerifyLandscape() {
    if(!TerrainComponent || !TreeCount || !FernCount || !RockCount) return false;
    int32 Passed=0;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GardenTerrain),true);
    for(int32 Y=0;Y<21;Y++) for(int32 X=0;X<21;X++) {
        double WX=-26000+2600*X,WY=-26000+2600*Y;
        FHitResult Hit;
        if(!TerrainComponent->LineTraceComponent(Hit,FVector(WX,WY,12000),FVector(WX,WY,-12000),Params) ||
            FMath::Abs(Hit.ImpactPoint.Z-GardenField::Height(WX,WY,CurrentRealm))>55) {
            UE_LOG(LogTemp,Error,TEXT("GARDEN_TERRAIN_RAY_FAILED realm=%d x=%.0f y=%.0f hit=%.2f"),CurrentRealm,WX,WY,Hit.ImpactPoint.Z); return false;
        }
        Passed++;
    }
    TerrainChecks+=Passed;
    UE_LOG(LogTemp,Display,TEXT("GARDEN_TERRAIN_CHECK_PASS realm=%d checks=%d tolerance_cm=55"),CurrentRealm,Passed);
    return true;
}
