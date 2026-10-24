// SPDX-License-Identifier: MIT
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TissueModel.h"
#include "TissueField.h"
#include "HALVETHTissueComponent.generated.h"
class USkeletalMeshComponent;
class USkeletalMesh;
class UTexture2D;
class UMaterialInstanceDynamic;
UCLASS(ClassGroup=(HALVETH))
class HALVETHREALMS_API UHALVETHTissueComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UHALVETHTissueComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Dt,ELevelTick Tick,FActorComponentTickFunction* Function) override;
    void LogAudit() const;
    bool VerifyGpuField() const;
    bool HasDeformedSurface() const {return Active&&!Failed&&Samples>20&&PeakDisplacement>.001&&BoundMaterials>0;}
    bool NeedsFullPose() const {return Active&&!Failed;}
private:
    struct FPatch {
        int32 Bone=0;FVector Centre=FVector::ZeroVector;double Radius=5;
        FVector LastCentre=FVector::ZeroVector,Velocity=FVector::ZeroVector,Acceleration=FVector::ZeroVector;
        std::array<HalvethTissue::V,4> Rest;
        HalvethTissue::Patch State;bool Ready=false;
    };
    bool Prepare();bool InBudget() const;void SetSurfaceActive(bool Value);
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Rig;
    UPROPERTY() TObjectPtr<USkeletalMesh> BoundAsset;
    UPROPERTY() TObjectPtr<UTexture2D> Field;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    TArray<FPatch> Patches;
    std::array<HalvethTissueField::Region,HalvethTissueField::Regions> ExpectedField;
    bool Active=false,Wanted=false,Failed=false;
    int32 BoundMaterials=0;
    double BudgetAt=0,ReadyAfter=0,Blend=0,PeakDisplacement=0,MaxVolumeError=0,MaxMilliseconds=0,TotalMilliseconds=0;
    uint64 Samples=0,Rebases=0,LimitedForces=0;
    FVector PreviousOrigin=FVector::ZeroVector;
};
