// SPDX-License-Identifier: MIT
#include "HALVETHGameMode.h"
#include "HALVETHRealmWorld.h"
#include "HALVETHCharacter.h"
#include "HALVETHGuideCharacter.h"
#include "HALVETHGuideAnimInstance.h"
#include "HALVETHTissueComponent.h"
#include "HumanAppearance.h"
#include "HumanAppearanceRuntime.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HighResScreenshot.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Materials/MaterialInstanceDynamic.h"

// A declared art inspection fixture. It changes the camera and isolated
// subject on each cut; it is never described as an ordinary continuous walk.
void AHALVETHGameMode::RunAppearanceCapture(float Dt){
    auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* PC=GetWorld()->GetFirstPlayerController();if(!P||!PC||!RealmWorld)return;
    MotionElapsed+=Dt;if(MotionElapsed<6)return;
    const double T=MotionElapsed-6;
    const bool bLayerInspection=FParse::Param(FCommandLine::Get(),TEXT("HalvethSkinLayersCapture"));
    const bool Studio=bLayerInspection||FParse::Param(FCommandLine::Get(),TEXT("HalvethAppearanceStudio"));
    const int32 FinalStage=bLayerInspection?16:Studio?4:10;
    const int32 Stage=FMath::Min(FinalStage,FMath::FloorToInt(T/2.));
    const int32 ModelIndex=bLayerInspection?Stage/2:Studio?0:Stage;
    const FString Folder=FPaths::ProjectDir()/(bLayerInspection?TEXT("Preview/SkinLayers"):Studio?TEXT("Preview/AppearanceStudio"):TEXT("Preview/Appearance"));
    if(MotionStage!=Stage){
        MotionStage=Stage;P->DisableInput(PC);PC->SetIgnoreLookInput(true);PC->SetIgnoreMoveInput(true);
        P->GetCharacterMovement()->SetMovementMode(MOVE_Flying);ApplyQuality(1,false);
        if(auto* HUD=PC->GetHUD())HUD->bShowHUD=false;
        if(Stage==FinalStage){
            P->SetAppearanceInspectionFov(0);
            FFileHelper::SaveStringToFile(MotionTiming,*(FPaths::ProjectDir()/(bLayerInspection?TEXT("QA/skin-layer-frames.tsv"):Studio?TEXT("QA/appearance-studio-frames.tsv"):TEXT("QA/appearance-frames.tsv"))));
            UE_LOG(LogTemp,Display,TEXT("APPEARANCE_CAPTURE_COMPLETE subjects=%d environment_views=%d requested_frames=%d quality=1 fixture=true studio=%d skin_layers=%d aesthetic_acceptance=HUMAN_REVIEW"),bLayerInspection?8:Studio?1:8,Studio?0:2,MotionFrame,Studio,bLayerInspection);
            bMotionCapture=false;FPlatformMisc::RequestExit(false);return;
        }
        auto* Subject=RealmWorld->GetGuideCharacter(1);if(!Subject){bMotionCapture=false;FPlatformMisc::RequestExitWithStatus(false,6);return;}
        // This is a material/pose inspection, with explicit mesh replacement.
        // Normal walking and the shared live field are captured separately.
        if(auto* Tissue=Subject->FindComponentByClass<UHALVETHTissueComponent>())Tissue->SuspendForAppearanceInspection();
        if(bLayerInspection||Stage<8){
            const FString Name=UTF8_TO_TCHAR(HalvethAppearance::Models[ModelIndex]);
            const FString Path=TEXT("/Game/Characters/")+Name+TEXT(".")+Name;
            auto* Mesh=Subject->GetMesh();auto* Source=LoadObject<USkeletalMesh>(nullptr,*Path);
            if(!Source){UE_LOG(LogTemp,Error,TEXT("APPEARANCE_SUBJECT_MISSING model=%s"),*Name);bMotionCapture=false;FPlatformMisc::RequestExitWithStatus(false,6);return;}
            Mesh->SetSkeletalMeshAsset(Source);
            // Clear outgoing overrides: a portrait must use its OWN skin.
            Mesh->EmptyOverrideMaterials();
            Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);Mesh->SetAnimInstanceClass(UHALVETHGuideAnimInstance::StaticClass());
            if(auto* Anim=Cast<UHALVETHGuideAnimInstance>(Mesh->GetAnimInstance())){
                Anim->BaseIdle=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/Characters/")+Name+TEXT("Idle.")+Name+TEXT("Idle")));
                Anim->Identity=ModelIndex+1;Anim->CruiseSpeed=84;
                Anim->FreezeAppearancePose=Studio;
            }
            HalvethAppearanceRuntime::Apply(Mesh,7919u*uint32(ModelIndex+1));
            Subject->GetCharacterMovement()->StopMovementImmediately();Subject->GetCharacterMovement()->DisableMovement();
            Subject->SetActorRotation(FRotator(0,0,0));
            if(Studio){
                RealmWorld->EnableAppearanceStudio();
                const float Amount=bLayerInspection?.08f:Stage==0?0.f:Stage==1?.04f:Stage==2?.08f:.2f;
                for(int32 Slot=0;Slot<Mesh->GetNumMaterials();Slot++){
                    float Existing;if(!Mesh->GetMaterial(Slot)||!Mesh->GetMaterial(Slot)->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SkinScatter")),Existing))continue;
                    auto* Material=Mesh->CreateDynamicMaterialInstance(Slot);
                    Material->SetScalarParameterValue(TEXT("SkinScatter"),Amount);
                    Material->SetScalarParameterValue(TEXT("SkinRoughnessBias"),0.f);
                    Material->SetScalarParameterValue(TEXT("TissueEnabled"),0);
                    if(bLayerInspection)Material->SetScalarParameterValue(TEXT("MakeupStrength"),Stage%2?.85f:0.f);
                }
                UE_LOG(LogTemp,Display,TEXT("APPEARANCE_STUDIO_PARAMETERS stage=%d skin_scatter=%.3f roughness_bias=0 subject_seed=%u pose_time=0 gait=0 gaze_yaw_pitch=0 blink=0 talk=0 breath=.25 tissue_field=disabled"),Stage,Amount,7919u*uint32(ModelIndex+1));
                if(bLayerInspection)UE_LOG(LogTemp,Display,TEXT("SKIN_LAYER_CONTROL model=%s strength=%.3f scatter=.08 micro_strength=1 camera=held pose=held all_other_parameters=held"),*Name,Stage%2?.85f:0.f);
            }
            UE_LOG(LogTemp,Display,TEXT("APPEARANCE_SUBJECT model=%s adult=true wardrobe=removed fixture=true"),*Name);
        }
        FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Folder);
    }
    auto* Subject=RealmWorld->GetGuideCharacter(1);if(!Subject)return;
    const FVector Base=Subject->GetMesh()->GetComponentLocation();
    if(bLayerInspection||Stage<8){
        P->SetAppearanceInspectionFov(30);
        const FVector Head=(Subject->GetMesh()->GetBoneLocation(TEXT("eye_L"))+Subject->GetMesh()->GetBoneLocation(TEXT("eye_R")))*.5;
        // Front + slightly oblique view in the SAME world lighting as normal play.
        FVector Front=Subject->GetMesh()->GetBoneLocation(TEXT("toe3-1_L"))-Subject->GetMesh()->GetBoneLocation(TEXT("foot_L"));
        Front.Z=0;Front.Normalize();const FVector Side=FVector::CrossProduct(Front,FVector::UpVector);
        const FVector Eye=Head+Front*105+Side*14+FVector(0,0,2);
        P->SetActorLocation(Eye-FVector(0,0,66),false,nullptr,ETeleportType::TeleportPhysics);
        PC->SetControlRotation((Head+FVector(0,0,2)-Eye).Rotation());
    }else if(Stage==8){
        P->SetAppearanceInspectionFov(70);
        const FVector Eye=Base+FVector(0,-460,115);
        P->SetActorLocation(Eye-FVector(0,0,66),false,nullptr,ETeleportType::TeleportPhysics);
        PC->SetControlRotation((Base+FVector(0,0,85)-Eye).Rotation());
    }else{
        P->SetAppearanceInspectionFov(70);
        const FVector Eye=Base+FVector(180,-240,34);
        P->SetActorLocation(Eye-FVector(0,0,66),false,nullptr,ETeleportType::TeleportPhysics);
        PC->SetControlRotation((Base+FVector(160,100,5)-Eye).Rotation());
    }
    // One settled screenshot per cut, after >1 second of TAA and skin warm-up.
    if(T-Stage*2.>=1.25&&MotionFrame<=Stage){
        const TCHAR* StudioNames[]={TEXT("studio-no-sss.png"),TEXT("studio-scatter04.png"),TEXT("studio-scatter08.png"),TEXT("studio-scatter20.png")};
        const FString Name=bLayerInspection?FString::Printf(TEXT("layers-%s-%s.png"),UTF8_TO_TCHAR(HalvethAppearance::Models[ModelIndex]),Stage%2?TEXT("on"):TEXT("off")):Studio?FString(StudioNames[Stage]):Stage<8?FString::Printf(TEXT("portrait-%s.png"),UTF8_TO_TCHAR(HalvethAppearance::Models[Stage]))
            :Stage==8?TEXT("world-body.png"):TEXT("meadow-ground.png");
        FScreenshotRequest::RequestScreenshot(Folder/Name,false,false);
        int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
        const auto* Camera=P->FindComponentByClass<UCameraComponent>();
        UE_LOG(LogTemp,Display,TEXT("APPEARANCE_FRAME file=%s stage=%d model=%s viewport=%dx%d component_fov_deg=%.6f actual_view_fov_deg=%.6f camera_world=%s camera_rotation=%s mesh_world=%s fixture=true skin_field=disabled purpose=material_pose_inspection"),*Name,Stage,*Subject->GetMesh()->GetSkeletalMeshAsset()->GetName(),Width,Height,Camera?Camera->FieldOfView:-1,PC->PlayerCameraManager?PC->PlayerCameraManager->GetFOVAngle():-1,PC->PlayerCameraManager?*PC->PlayerCameraManager->GetCameraLocation().ToString():TEXT("UNKNOWN"),*PC->GetControlRotation().ToString(),*Base.ToString());
        if(auto* Pose=Cast<UHALVETHGuideAnimInstance>(Subject->GetMesh()->GetAnimInstance())){
            auto* Mesh=Subject->GetMesh();
            UE_LOG(LogTemp,Display,TEXT("APPEARANCE_ACTUAL_POSE file=%s time=%.6f yaw=%.6f pitch=%.6f blink=%.6f talk=%.6f breath=%.6f eye_left=%s eye_right=%s head_rotation=%s"),*Name,Pose->Time,Pose->LookYaw,Pose->LookPitch,Mesh->GetMorphTarget(TEXT("Blink")),Mesh->GetMorphTarget(TEXT("Talk")),Mesh->GetMorphTarget(TEXT("Breath")),*Mesh->GetBoneLocation(TEXT("eye_L")).ToString(),*Mesh->GetBoneLocation(TEXT("eye_R")).ToString(),*Mesh->GetBoneQuaternion(TEXT("head")).ToString());
        }
        MotionTiming+=FString::Printf(TEXT("%d\t%.9f\t%d\t%s\n"),MotionFrame,T,Stage,*Name);MotionFrame++;
    }
}
