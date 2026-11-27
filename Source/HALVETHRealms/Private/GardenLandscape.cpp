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
    // Leaves remain traversable; a separate simple trunk blocks actual movement.
    // Collision uses instancing as the visible forest does, rather than one actor per tree.
    TreeTrunks=nullptr;
    if(auto* Trunk=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"))) {
        TreeTrunks=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        AddInstanceComponent(TreeTrunks); TreeTrunks->SetupAttachment(RootComponent);
        TreeTrunks->SetStaticMesh(Trunk); TreeTrunks->SetMobility(EComponentMobility::Static);
        TreeTrunks->SetVisibility(false); TreeTrunks->SetHiddenInGame(true);
        TreeTrunks->SetCastShadow(false); TreeTrunks->SetCanEverAffectNavigation(false);
        TreeTrunks->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        TreeTrunks->SetCollisionResponseToAllChannels(ECR_Block);
        TreeTrunks->RegisterComponent(); Generated.Add(TreeTrunks);
    }
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
            const FTransform Placement(Rotation,FVector(X,Y,Z)-Anchor-FVector(0,0,4),FVector(Scale));
            C->AddInstance(Placement);
            if(Key==TEXT("tree_small_02")&&TreeTrunks) {
                const FVector Base=Placement.TransformPosition(FVector(0,0,Bounds.Min.Z));
                const double Radius=FMath::Clamp(Bounds.GetExtent().Size2D()*.045,8.,24.)*Scale;
                const double Height=FMath::Clamp(Bounds.GetSize().Z*Scale*.45,180.,1400.);
                TreeTrunks->AddInstance(FTransform(FRotator::ZeroRotator,Base+FVector(0,0,Height*.5),FVector(Radius/50.,Radius/50.,Height/100.)));
            }
            if(Key==TEXT("tree_small_02")) TreeCount++; else if(Key==TEXT("fern_02")) FernCount++; else RockCount++;
        }
    };
    Scatter(TEXT("tree_small_02"),CurrentRealm==1?1700:1050,false);
    Scatter(TEXT("fern_02"),15000,false);
    Scatter(TEXT("rock_moss_set_01"),550,true);
    UE_LOG(LogTemp,Display,TEXT("GARDEN_LANDSCAPE_READY realm=%d extent_m=600 trees=%d ferns=%d rocks=%d"),CurrentRealm,TreeCount,FernCount,RockCount);
    UE_LOG(LogTemp,Display,TEXT("GARDEN_FOREST_PHYSICS realm=%d visual_trees=%d colliding_trunks=%d"),CurrentRealm,TreeCount,TreeTrunks?TreeTrunks->GetInstanceCount():0);
}

bool AHALVETHRealmWorld::VerifyLandscape() {
    if(!TerrainComponent || !TreeCount || !FernCount || !RockCount || !TreeTrunks || TreeTrunks->GetInstanceCount()!=TreeCount) return false;
    FCollisionQueryParams TrunkQuery(SCENE_QUERY_STAT(GardenTrunkCollision),false);
    int32 TrunkChecks=0;
    for(int32 Instance:{0,TreeCount/2,TreeCount-1}) {
        FTransform Pose;
        if(!TreeTrunks->GetInstanceTransform(Instance,Pose,true))return false;
        const FVector Centre=Pose.GetLocation();
        const double Radius=Pose.GetScale3D().X*50.;
        FHitResult Hit;
        if(!TreeTrunks->LineTraceComponent(Hit,Centre-FVector(Radius*2+25,0,0),Centre+FVector(Radius*2+25,0,0),TrunkQuery)) {
            UE_LOG(LogTemp,Error,TEXT("GARDEN_TRUNK_RAY_FAILED realm=%d instance=%d"),CurrentRealm,Instance);return false;
        }
        TrunkChecks++;
    }
    UE_LOG(LogTemp,Display,TEXT("GARDEN_TRUNK_COLLISION_PASS realm=%d sampled_trunks=%d"),CurrentRealm,TrunkChecks);
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
