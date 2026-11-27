#include "GardenSoil.h"
#include "HALVETHRealmWorld.h"
#include "HALVETHGuideCharacter.h"
#include "HALVETHGuideAnimInstance.h"
#include "EarthDynamicsMath.h"
#include "ProceduralMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

void AHALVETHRealmWorld::BuildSoil(FVector Guide){
    FVector Center=GardenSoil::Center(Guide);const int N=GardenSoil::Grid;
    auto* C=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(C);C->SetupAttachment(RootComponent);
    C->SetMobility(EComponentMobility::Movable);C->bUseAsyncCooking=false;
    C->bUseComplexAsSimpleCollision=true;C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);C->SetCollisionResponseToAllChannels(ECR_Block);
    TArray<FVector> V,Normals;TArray<int32> Triangles;TArray<FVector2D> UV;TArray<FLinearColor> Colors;TArray<FProcMeshTangent> Tangents;
    for(int Y=0;Y<N;Y++)for(int X=0;X<N;X++){
        double WX=Center.X-GardenSoil::Half+X*2*GardenSoil::Half/(N-1),WY=Center.Y-GardenSoil::Half+Y*2*GardenSoil::Half/(N-1);
        V.Emplace(WX,WY,GardenField::Height(WX,WY,CurrentRealm));Normals.Add(FVector::UpVector);UV.Emplace(WX/300,WY/300);
        FVector4f Col=GardenField::Color(WX,WY,CurrentRealm);Colors.Emplace(Col.X,Col.Y,Col.Z,Col.W);Tangents.Emplace(FVector::ForwardVector,false);
    }
    for(int Y=0;Y<N-1;Y++)for(int X=0;X<N-1;X++){int I=Y*N+X;Triangles.Append({I,I+N,I+1,I+1,I+N,I+N+1});}
    C->CreateMeshSection_LinearColor(0,V,Triangles,Normals,UV,Colors,Tangents,true,false);
    if(TerrainComponent&&TerrainComponent->GetStaticMesh())C->SetMaterial(0,TerrainComponent->GetStaticMesh()->GetMaterial(0));
    C->RegisterComponent();Generated.Add(C);SoilComponents.Add(C);SoilCenters.Add(Center);SoilVertices.Add(V);SoilColors.Add(Colors);SoilLoads.Add(0);SoilDepths.Add(0);
    UE_LOG(LogTemp,Display,TEXT("GARDEN_SOIL_READY patch=%d vertices=%d spacing_cm=%.5f replaces_terrain_cells=1 dynamic_collision=1"),SoilComponents.Num(),V.Num(),2*GardenSoil::Half/(N-1));
}

void AHALVETHRealmWorld::TickSoil(float Dt){
    SoilUpdateTime+=Dt;if(SoilUpdateTime<.10f)return;const float StepDt=SoilUpdateTime;SoilUpdateTime=0;
    for(int I=0;I<SoilComponents.Num();I++){
        if(!GuideActors.IsValidIndex(I)||!GuideActors[I]||!GuideBodies[I])continue;
        auto* Pawn=GuideActors[I].Get();auto* Body=GuideBodies[I].Get();auto* Move=Pawn->GetCharacterMovement();
        auto* Motion=Cast<UHALVETHGuideAnimInstance>(Body->GetAnimInstance());
        const double Force=EarthDynamics::SupportNewtons(Move->Mass,0,Move->IsMovingOnGround());
        SoilLoads[I]=Force;
        FVector Foot[2]={Body->GetBoneLocation(TEXT("foot_L"))-GetActorLocation(),Body->GetBoneLocation(TEXT("foot_R"))-GetActorLocation()};
        bool Stance[2]={true,true};if(Motion&&Motion->WalkWeight>.08f){Stance[0]=Motion->Planted[0];Stance[1]=Motion->Planted[1];}
        int Supports=int(Stance[0])+int(Stance[1]);const double Load=Supports?Force/Supports:0;
        auto& V=SoilVertices[I];const int N=GardenSoil::Grid;double MaxDepth=0;
        for(auto& Point:V){
            double Target=GardenField::Height(Point.X,Point.Y,CurrentRealm);
            for(int S=0;S<2;S++)if(Stance[S])Target-=EarthDynamics::SoilDepthCm(Load,FVector::DistSquared2D(Point,Foot[S]));
            const double Edge=FMath::Min(GardenSoil::Half-FMath::Abs(Point.X-SoilCenters[I].X),GardenSoil::Half-FMath::Abs(Point.Y-SoilCenters[I].Y));
            Target=FMath::Lerp(GardenField::Height(Point.X,Point.Y,CurrentRealm),Target,FMath::Clamp(Edge/50,0.,1.));
            Point.Z=FMath::FInterpTo(Point.Z,Target,StepDt,8);MaxDepth=FMath::Max(MaxDepth,GardenField::Height(Point.X,Point.Y,CurrentRealm)-Point.Z);
        }
        SoilDepths[I]=FMath::Max(SoilDepths[I],MaxDepth);
        TArray<FVector> Normals;Normals.SetNum(V.Num());TArray<FVector2D> UV;TArray<FProcMeshTangent> Tangents;
        const double Spacing=2*GardenSoil::Half/(N-1);
        for(int Y=0;Y<N;Y++)for(int X=0;X<N;X++){
            int J=Y*N+X;double DX=(V[Y*N+FMath::Min(X+1,N-1)].Z-V[Y*N+FMath::Max(0,X-1)].Z)/(2*Spacing);
            double DY=(V[FMath::Min(Y+1,N-1)*N+X].Z-V[FMath::Max(0,Y-1)*N+X].Z)/(2*Spacing);
            Normals[J]=FVector(-DX,-DY,1).GetSafeNormal();
        }
        SoilComponents[I]->UpdateMeshSection_LinearColor(0,V,Normals,UV,SoilColors[I],Tangents,false);
    }
}

bool AHALVETHRealmWorld::VerifyGroundDynamics() const{
    if(SoilComponents.Num()!=3)return false;
    bool Good=true;
    for(int I=0;I<SoilComponents.Num();I++){
        const auto* Pawn=GuideActors[I].Get();const auto* Move=Pawn->GetCharacterMovement();
        const bool Supported=Move->IsMovingOnGround()&&Move->CurrentFloor.IsWalkableFloor();
        auto* Motion=Cast<UHALVETHGuideAnimInstance>(GuideBodies[I]->GetAnimInstance());
        UE_LOG(LogTemp,Display,TEXT("GARDEN_GROUND_AUDIT identity=%d supported=%d gravity_cm_s2=%.5f mass_kg=%.2f support_model_newtons=%.5f max_soil_depth_cm=%.5f dynamic_collision=1 plant_target_drift_cm=%.9f samples=%d"),GuideIdentities[I],Supported,Move->GetGravityZ(),Move->Mass,SoilLoads[I],SoilDepths[I],Motion?Motion->PlantDriftMax:0,Motion?Motion->PlantSamples:0);
        Good&=Supported&&SoilLoads[I]>100&&SoilDepths[I]>.03&&SoilDepths[I]<2;
    }
    return Good;
}
