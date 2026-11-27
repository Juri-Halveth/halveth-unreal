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
    GroundedTreeCount=GroundedFernCount=GroundedRockCount=0;
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
    if(!FFileHelper::LoadFileToString(Raw,*(FPaths::ProjectSavedDir()/TEXT("Garden-imports.json")))) {
        // Packaged games have no editor-side import receipt under Saved.
        // These are the fixed public CC0 imports used by GardenPrepare.
        Raw=TEXT(R"garden({
            "tree_small_02":["/Game/VOID/Models/tree_small_02/tree_small_02_2k/StaticMeshes/tree_small_02_2k.tree_small_02_2k"],
            "fern_02":[
                "/Game/VOID/Models/fern_02/fern_02_2k/StaticMeshes/fern_02_a.fern_02_a",
                "/Game/VOID/Models/fern_02/fern_02_2k/StaticMeshes/fern_02_b.fern_02_b",
                "/Game/VOID/Models/fern_02/fern_02_2k/StaticMeshes/fern_02_c.fern_02_c",
                "/Game/VOID/Models/fern_02/fern_02_2k/StaticMeshes/fern_02_d.fern_02_d"],
            "rock_moss_set_01":[
                "/Game/VOID/Models/rock_moss_set_01/rock_moss_set_01_2k/StaticMeshes/rock_moss_set_01_rock01.rock_moss_set_01_rock01",
                "/Game/VOID/Models/rock_moss_set_01/rock_moss_set_01_2k/StaticMeshes/rock_moss_set_01_rock02.rock_moss_set_01_rock02",
                "/Game/VOID/Models/rock_moss_set_01/rock_moss_set_01_2k/StaticMeshes/rock_moss_set_01_rock03.rock_moss_set_01_rock03",
                "/Game/VOID/Models/rock_moss_set_01/rock_moss_set_01_2k/StaticMeshes/rock_moss_set_01_rock04.rock_moss_set_01_rock04",
                "/Game/VOID/Models/rock_moss_set_01/rock_moss_set_01_2k/StaticMeshes/rock_moss_set_01_rock05.rock_moss_set_01_rock05",
                "/Game/VOID/Models/rock_moss_set_01/rock_moss_set_01_2k/StaticMeshes/rock_moss_set_01_rock06.rock_moss_set_01_rock06"]
        })garden");
    }
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw),Doc) || !Doc.IsValid()) {
        UE_LOG(LogTemp,Error,TEXT("GARDEN_NATURE_MANIFEST_MISSING")); return;
    }
    FRandomStream RNG(int32(Seed)^int32((CurrentRealm+1)*7919));
    // Query the rendered/colliding triangle surface, not the analytic height
    // used to author its vertices. The two differ between grid vertices.
    auto TraceSurface=[this](double X,double Y,FHitResult& Hit) {
        FCollisionQueryParams Params(SCENE_QUERY_STAT(GardenSurfacePlacement),true);
        return TerrainComponent->LineTraceComponent(Hit,FVector(X,Y,12000),FVector(X,Y,-12000),Params);
    };
    auto Scatter=[this,&RNG,&Doc,&TraceSurface](const FString& Key,int32 Attempts,bool Solid) {
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
            FHitResult Ground;
            if(!TraceSurface(X,Y,Ground)) continue;
            double Z=Ground.ImpactPoint.Z;
            if(Z<0 || Ground.ImpactNormal.Z<.72) continue;
            // Vegetation follows the same moisture/grass/rock field that colors
            // the terrain. This produces coherent groves and open stony ground.
            const FVector4f Soil=GardenField::Color(X,Y,CurrentRealm);
            if(Key==TEXT("tree_small_02") && RNG.FRand()>
                FMath::Clamp((double(Soil.Y)-.28)*1.6*(1.-.7*Soil.X),.0,1.)) continue;
            if(Key==TEXT("fern_02") && RNG.FRand()>
                FMath::Clamp((double(Soil.Y)-.23)*1.55*(1.-.8*Soil.X),.0,1.)) continue;
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
                double Lowest=Z,Highest=Z;
                bool bSupported=true;
                for(FVector2D Offset:{FVector2D(E.X,0),FVector2D(-E.X,0),FVector2D(0,E.Y),FVector2D(0,-E.Y)}) {
                    FVector R=Rotation.RotateVector(FVector(Offset.X,Offset.Y,0));
                    FHitResult Support;
                    if(!TraceSurface(X+R.X,Y+R.Y,Support)) { bSupported=false; break; }
                    Lowest=FMath::Min(Lowest,Support.ImpactPoint.Z);
                    Highest=FMath::Max(Highest,Support.ImpactPoint.Z);
                }
                // A rigid boulder cannot rest on a steep four-corner footprint
                // without hovering or disappearing into the hill.
                if(!bSupported || Highest-Lowest>80) continue;
                Z=Lowest-FMath::Min(16.,Bounds.GetSize().Z*Scale*.07);
            }
            const FTransform Placement(Rotation,FVector(X,Y,Z)-Anchor-FVector(0,0,4),FVector(Scale));
            C->AddInstance(Placement);
            const FVector Root=Placement.TransformPosition(FVector(Center.X,Center.Y,Bounds.Min.Z));
            if(Key==TEXT("tree_small_02")&&TreeTrunks) {
                const double Radius=FMath::Clamp(Bounds.GetExtent().Size2D()*.045,8.,24.)*Scale;
                const double Height=FMath::Clamp(Bounds.GetSize().Z*Scale*.45,180.,1400.);
                TreeTrunks->AddInstance(FTransform(FRotator::ZeroRotator,Root+FVector(0,0,Height*.5),FVector(Radius/50.,Radius/50.,Height/100.)));
            }
            if(Key==TEXT("tree_small_02")) { TreeCount++; if(FMath::Abs(Root.Z-Ground.ImpactPoint.Z)<6) GroundedTreeCount++; }
            else if(Key==TEXT("fern_02")) { FernCount++; if(FMath::Abs(Root.Z-Ground.ImpactPoint.Z)<6) GroundedFernCount++; }
            else { RockCount++; if(Root.Z<=Ground.ImpactPoint.Z && Ground.ImpactPoint.Z-Root.Z<110) GroundedRockCount++; }
        }
    };
    Scatter(TEXT("tree_small_02"),CurrentRealm==1?1700:1050,false);
    Scatter(TEXT("fern_02"),15000,false);
    Scatter(TEXT("rock_moss_set_01"),550,true);
    UE_LOG(LogTemp,Display,TEXT("GARDEN_LANDSCAPE_READY realm=%d extent_m=600 trees=%d ferns=%d rocks=%d"),CurrentRealm,TreeCount,FernCount,RockCount);
    UE_LOG(LogTemp,Display,TEXT("GARDEN_FOREST_PHYSICS realm=%d visual_trees=%d colliding_trunks=%d"),CurrentRealm,TreeCount,TreeTrunks?TreeTrunks->GetInstanceCount():0);
    UE_LOG(LogTemp,Display,TEXT("GARDEN_SUBSTRATE_CONTACT realm=%d trees=%d/%d ferns=%d/%d rocks=%d/%d terrain=triangle_trace biome=soil_color_moisture"),CurrentRealm,GroundedTreeCount,TreeCount,GroundedFernCount,FernCount,GroundedRockCount,RockCount);
}

bool AHALVETHRealmWorld::VerifyLandscape() {
    if(!TerrainComponent || !TreeCount || !FernCount || !RockCount || !TreeTrunks || TreeTrunks->GetInstanceCount()!=TreeCount) return false;
    if(GroundedTreeCount!=TreeCount || GroundedFernCount!=FernCount || GroundedRockCount!=RockCount) return false;
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
        FHitResult Ground;
        if(!TerrainComponent->LineTraceComponent(Ground,FVector(Centre.X,Centre.Y,12000),FVector(Centre.X,Centre.Y,-12000),TrunkQuery) ||
            FMath::Abs(Centre.Z-Pose.GetScale3D().Z*50.-Ground.ImpactPoint.Z)>12) {
            UE_LOG(LogTemp,Error,TEXT("GARDEN_TRUNK_GROUND_GAP realm=%d instance=%d"),CurrentRealm,Instance);return false;
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
