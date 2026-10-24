// SPDX-License-Identifier: MIT
#include "HALVETHTissueComponent.h"
#include "TissueField.h"
#include "HALVETHGuideAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UnrealClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
namespace {
HalvethTissue::V T(FVector V){return {V.X,V.Y,V.Z};}
FVector U(HalvethTissue::V V){return {V.x,V.y,V.z};}
const FVector Tetra[4]={{1,1,1},{1,-1,-1},{-1,1,-1},{-1,-1,1}};
struct FTexel {float X,Y,Z,W;};
static_assert(sizeof(FTexel)==16);
FTexel Pack(FVector V,float W=0){return {float(V.X),float(V.Y),float(V.Z),W};}
}
UHALVETHTissueComponent::UHALVETHTissueComponent(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PostUpdateWork;}
void UHALVETHTissueComponent::BeginPlay(){Super::BeginPlay();ReadyAfter=GetWorld()->GetTimeSeconds()+.75;if(auto* Person=Cast<ACharacter>(GetOwner()))Rig=Person->GetMesh();if(Rig)AddTickPrerequisiteComponent(Rig);}
void UHALVETHTissueComponent::EndPlay(const EEndPlayReason::Type Reason){LogAudit();SetSurfaceActive(false);Super::EndPlay(Reason);}
bool UHALVETHTissueComponent::InBudget() const{
    const auto* Player=UGameplayStatics::GetPlayerPawn(this,0);if(!Player)return false;
    if(!Active&&(!Rig||!Rig->WasRecentlyRendered(.25f)))return false;
    const double Distance=FVector::DistSquared(GetOwner()->GetActorLocation(),Player->GetActorLocation());
    if(Distance>FMath::Square(Active?1600.:1400.))return false;
    int32 Ahead=0,Others=0;
    for(TActorIterator<ACharacter> It(GetWorld());It;++It){auto* Other=It->FindComponentByClass<UHALVETHTissueComponent>();if(*It==GetOwner()||!Other||Other->Failed||(!Other->Active&&(!Other->Rig||!Other->Rig->WasRecentlyRendered(.25f))))continue;
        Others+=Other->Active;const double D=FVector::DistSquared(It->GetActorLocation(),Player->GetActorLocation());
        if(D<Distance||(D==Distance&&It->GetUniqueID()<GetOwner()->GetUniqueID()))Ahead++;
    }
    return Ahead<6&&(Active||Others<6);
}
void UHALVETHTissueComponent::SetSurfaceActive(bool Value){
    if(!Rig||Active==Value)return;Active=Value;
    for(const auto& M:Materials)M->SetScalarParameterValue(TEXT("TissueEnabled"),Value?1.f:0.f);
    Rig->VisibilityBasedAnimTickOption=Value?EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones:EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    if(auto* Budgeted=Cast<USkeletalMeshComponentBudgeted>(Rig))Budgeted->SetComponentSignificance(Value?10.f:1.f,Value,Value,!Value);
    if(Value){Blend=0;PreviousOrigin=Rig->GetComponentLocation();for(auto& Patch:Patches)Patch.Ready=false;}
}
bool UHALVETHTissueComponent::Prepare(){
    if(!Rig||!Rig->GetSkeletalMeshAsset())return false;auto* Asset=Rig->GetSkeletalMeshAsset();const auto& Ref=Asset->GetRefSkeleton();
    const TCHAR* Names[]={TEXT("spine03"),TEXT("spine02"),TEXT("spine01"),TEXT("head"),TEXT("neck03"),TEXT("upperarm01_L"),TEXT("upperarm01_R"),TEXT("lowerarm01_L"),TEXT("lowerarm01_R"),TEXT("upperleg01_L"),TEXT("upperleg01_R"),TEXT("lowerleg01_L"),TEXT("lowerleg01_R")};
    for(const TCHAR* Name:Names){FPatch Patch;Patch.Bone=Ref.FindBoneIndex(Name);if(Patch.Bone==INDEX_NONE){Failed=true;return false;}
        Patch.Radius=FString(Name)==TEXT("head")?3.:5.;for(int32 K=0;K<4;K++)Patch.Rest[K]=T(Tetra[K]*Patch.Radius);Patches.Add(Patch);
    }
    // NAME_None asks UObject for a distinct transient identity. A shared literal
    // name in the transient package aliases different bodies' dynamic fields.
    Field=UTexture2D::CreateTransient(HalvethTissueField::Texels,1,PF_A32B32G32R32F);if(!Field){Failed=true;return false;}
    Field->SRGB=false;Field->Filter=TF_Nearest;Field->AddressX=TA_Clamp;Field->AddressY=TA_Clamp;
    auto& Mip=Field->GetPlatformData()->Mips[0];void* Bytes=Mip.BulkData.Lock(LOCK_READ_WRITE);FMemory::Memzero(Bytes,HalvethTissueField::Texels*sizeof(FTexel));Mip.BulkData.Unlock();Field->UpdateResource();
    for(int32 I=0;I<Rig->GetNumMaterials();I++){
        auto* Original=Rig->GetMaterial(I);if(!Original)continue;float Softness=0,Enabled=0;
        if(!Original->GetScalarParameterValue(FMaterialParameterInfo(TEXT("TissueSoftness")),Softness)||Softness<=0)continue;
        if(!Original->GetScalarParameterValue(FMaterialParameterInfo(TEXT("TissueEnabled")),Enabled)){Failed=true;return false;}
        auto* M=Rig->CreateDynamicMaterialInstance(I,Original);if(!M){Failed=true;return false;}
        M->SetTextureParameterValue(TEXT("TissueField"),Field);M->SetScalarParameterValue(TEXT("TissueEnabled"),0);Materials.Add(M);BoundMaterials++;
    }
    if(BoundMaterials==0){Failed=true;UE_LOG(LogTemp,Error,TEXT("TISSUE_SURFACE_UNAVAILABLE owner=%s reason=material_bridge_missing"),*GetOwner()->GetName());return false;}
    BoundAsset=Asset;UE_LOG(LogTemp,Display,TEXT("TISSUE_SURFACE_READY owner=%s field=%s regions=%d nodes=%d bound_materials=%d renderer=gpu_volume_field morphs=original_gpu_path biology=false"),*GetOwner()->GetName(),*Field->GetName(),Patches.Num(),Patches.Num()*4,BoundMaterials);return true;
}
void UHALVETHTissueComponent::TickComponent(float Dt,ELevelTick Tick,FActorComponentTickFunction* Function){
    Super::TickComponent(Dt,Tick,Function);if(Failed||!Rig||!FMath::IsFinite(Dt)||Dt<=0)return;
    // Do not replace materials while the first GPU morph buffers are being
    // created asynchronously. Only already rendered, warmed rigs enter here.
    if(GetWorld()->GetTimeSeconds()<ReadyAfter)return;
    if(Rig->GetSkeletalMeshAsset()!=BoundAsset){if(BoundAsset){Failed=true;SetSurfaceActive(false);return;}if(!Rig->GetSkeletalMeshAsset())return;}
    if(auto* Motion=Cast<UHALVETHGuideAnimInstance>(Rig->GetAnimInstance()))if(!Motion->PoseAuditReady)return;
    const double Now=GetWorld()->GetTimeSeconds();if(Now>=BudgetAt){BudgetAt=Now+.25;Wanted=InBudget();if(Wanted&&!BoundAsset&&!Prepare())return;if(Wanted&&!Active)SetSurfaceActive(true);}
    if(!Active)return;const double Started=FPlatformTime::Seconds();
    // Release amplitude continuously and keep occupying the budget slot until
    // its actual displacement reaches zero. Newcomers cannot evict this body.
    if(!Wanted&&Blend<=0){SetSurfaceActive(false);return;}
    if(!Field||!Field->GetResource())return;
    const bool Rebase=Dt>.25f||FVector::DistSquared(Rig->GetComponentLocation(),PreviousOrigin)>FMath::Square(200.);PreviousOrigin=Rig->GetComponentLocation();
    if(Rebase){Rebases++;UE_LOG(LogTemp,Display,TEXT("TISSUE_REBASE owner=%s reason=explicit_transform_or_gap dt=%.6f"),*GetOwner()->GetName(),Dt);}
    auto* Atlas=new FTexel[HalvethTissueField::Texels]{};Blend=FMath::Clamp(Blend+(Wanted?1.:-1.)*Dt*3.,0.,1.);
    const bool Disconnected=FParse::Param(FCommandLine::Get(),TEXT("HalvethRigidTissueControl"));int32 Index=0;
    for(auto& Patch:Patches){
        const FTransform Bone=Rig->GetBoneTransform(Patch.Bone);const FVector Centre=Bone.TransformPosition(Patch.Centre);
        if(!Patch.Ready||Rebase){
            HalvethTissue::Patch Fresh;Fresh.initialize(Patch.Rest);if(!Fresh.initialized){delete[] Atlas;Failed=true;SetSurfaceActive(false);return;}
            Patch.State=Fresh;Patch.LastCentre=Centre;Patch.Velocity=GetOwner()->GetVelocity();Patch.Acceleration=FVector::ZeroVector;Patch.Ready=true;
        }else{
            const FVector Velocity=(Centre-Patch.LastCentre)/Dt;
            const double Smooth=1-FMath::Exp(-double(Dt)*18.);const FVector Raw=(Velocity-Patch.Velocity)/Dt;
            Patch.Acceleration=FMath::Lerp(Patch.Acceleration,Raw,Smooth);Patch.Velocity=Velocity;Patch.LastCentre=Centre;
            // Explicit artistic excitation bound; the rest carrier follows
            // animation, while only compliant residuals are integrated.
            const FVector Inertia=Patch.Acceleration.GetClampedToMaxSize(1500.);LimitedForces+=Patch.Acceleration.SizeSquared()>1500.*1500.;
            const FVector Force=Bone.InverseTransformVectorNoScale(FVector(0,0,-980)-Inertia);
            if(!Patch.State.stepWithAcceleration(Patch.Rest,Dt,T(Force))){delete[] Atlas;Failed=true;SetSurfaceActive(false);UE_LOG(LogTemp,Error,TEXT("TISSUE_STEP_REJECTED owner=%s region=%d"),*GetOwner()->GetName(),Index);return;}
        }
        const int32 Base=Index*HalvethTissueField::Stride;
        Atlas[Base]=Pack(Centre,float(Patch.Radius*Bone.GetScale3D().GetMax()));
        auto& Expected=ExpectedField[Index];Expected.centre=T(Centre);Expected.radius=Patch.Radius*Bone.GetScale3D().GetMax();
        for(int32 Axis=0;Axis<3;Axis++){const FVector Basis=Bone.GetUnitAxis(EAxis::Type(Axis+1));Atlas[Base+1+Axis]=Pack(Basis);Expected.basis[Axis]=T(Basis);}
        for(int32 K=0;K<4;K++){
            const FVector RawDisplacement=Bone.TransformVector(U(Patch.State.displacement(K,Patch.Rest)))*Blend;Expected.displacement[K]=T(RawDisplacement);
            const FVector Displacement=Disconnected?FVector::ZeroVector:RawDisplacement;
            PeakDisplacement=FMath::Max(PeakDisplacement,Displacement.Length());Atlas[Base+4+K]=Pack(Displacement);
        }
        MaxVolumeError=FMath::Max(MaxVolumeError,Patch.State.relativeVolumeError(Patch.Rest));Index++;
    }
    auto* Region=new FUpdateTextureRegion2D(0,0,0,0,HalvethTissueField::Texels,1);
    Field->UpdateTextureRegions(0,1,Region,HalvethTissueField::Texels*sizeof(FTexel),sizeof(FTexel),reinterpret_cast<uint8*>(Atlas),[](uint8* Data,const FUpdateTextureRegion2D* R){delete[] reinterpret_cast<FTexel*>(Data);delete R;});
    Samples++;const double Cost=(FPlatformTime::Seconds()-Started)*1000;MaxMilliseconds=FMath::Max(MaxMilliseconds,Cost);TotalMilliseconds+=Cost;
}
void UHALVETHTissueComponent::LogAudit() const{
    UE_LOG(LogTemp,Display,TEXT("TISSUE_SURFACE_AUDIT owner=%s active=%d samples=%llu regions=%d gpu_materials=%d peak_field_displacement_cm=%.6f max_relative_volume_error=%.9f mean_update_ms=%.5f max_update_ms=%.5f explicit_rebases=%llu excitation_limited=%llu failed=%d gpu_readback=NOT_MEASURED"),*GetOwner()->GetName(),Active,Samples,Patches.Num(),BoundMaterials,PeakDisplacement,MaxVolumeError,Samples?TotalMilliseconds/Samples:0,MaxMilliseconds,Rebases,LimitedForces,Failed);
}
bool UHALVETHTissueComponent::VerifyGpuField() const{
    if(!Active||Failed||!Field||Samples<20)return false;
    auto* Source=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Characters/M_TissueFieldProbe.M_TissueFieldProbe"));if(!Source)return false;
    auto* Probe=UMaterialInstanceDynamic::Create(Source,GetTransientPackage());
    Probe->SetTextureParameterValue(TEXT("TissueField"),Field);Probe->SetScalarParameterValue(TEXT("TissueEnabled"),1);Probe->SetScalarParameterValue(TEXT("TissueSoftness"),1);
    const auto Point=ExpectedField[0].centre,Expected=HalvethTissueField::Sample(ExpectedField,Point);
    Probe->SetVectorParameterValue(TEXT("TissueProbePosition"),FLinearColor(float(Point.x),float(Point.y),float(Point.z),1));
    auto* Target=NewObject<UTextureRenderTarget2D>(GetTransientPackage());Target->InitCustomFormat(1,1,PF_A32B32G32R32F,false);Target->UpdateResourceImmediate(true);
    UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(),Target,Probe);
    TArray<FLinearColor> Pixels;FReadSurfaceDataFlags Flags(RCM_MinMax);Flags.SetLinearToGamma(false);
    const bool Read=Target->GameThread_GetRenderTargetResource()->ReadLinearColorPixels(Pixels,Flags);
    const FVector Actual=Read&&Pixels.Num()==1?FVector((Pixels[0].R-.5)*20,(Pixels[0].G-.5)*20,(Pixels[0].B-.5)*20):FVector(TNumericLimits<double>::Max());
    const double Error=FVector::Distance(Actual,U(Expected));const bool Passed=Read&&Expected.length()>.001&&Error<.002;
    UE_LOG(LogTemp,Display,TEXT("TISSUE_GPU_FIELD_WITNESS owner=%s result=%s expected_cm=%s readback_cm=%s error_cm=%.9f scope=shared_shader_field_not_final_skinned_pixels"),*GetOwner()->GetName(),Passed?TEXT("PASS"):TEXT("FAIL"),*U(Expected).ToString(),*Actual.ToString(),Error);
    return Passed;
}
