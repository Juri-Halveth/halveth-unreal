// SPDX-License-Identifier: MIT
#include "HALVETHGameMode.h"
#include "HALVETHRealmWorld.h"
#include "HALVETHCharacter.h"
#include "HALVETHAdventureComponent.h"
#include "HALVETHGuideCharacter.h"
#include "HALVETHGuideAnimInstance.h"
#include "HALVETHTissueComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"
#include "HAL/PlatformFileManager.h"

// Explicit test process only. The real player viewpoint moves around one live
// guide, addresses it, then a real blocking wall enters/leaves the sight line.
// No identity swaps, forced facial pose or replacement perception booleans.
void AHALVETHGameMode::RunGazeCapture(float Dt){
    auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* PC=GetWorld()->GetFirstPlayerController();
    auto* Subject=RealmWorld?RealmWorld->GetGuideCharacter(1):nullptr;
    if(!P||!PC||!Subject)return;
    auto* Mesh=Subject->GetMesh();auto* Motion=Cast<UHALVETHGuideAnimInstance>(Mesh->GetAnimInstance());
    if(!Motion)return;
    MotionElapsed+=Dt;if(MotionElapsed<6)return;
    const double T=MotionElapsed-6;
    if(MotionStage<0){
        MotionStage=0;P->DisableInput(PC);PC->SetIgnoreLookInput(true);PC->SetIgnoreMoveInput(true);
        P->GetCharacterMovement()->SetMovementMode(MOVE_Flying);P->SetAppearanceInspectionFov(42);
        Subject->SetActorRotation(FRotator::ZeroRotator);Subject->GetCharacterMovement()->StopMovementImmediately();
        if(auto* HUD=PC->GetHUD())HUD->bShowHUD=false;
        ApplyQuality(1,false);
        GazeEyeBaseline=Mesh->GetBoneQuaternion(TEXT("eye_L"),EBoneSpaces::ComponentSpace);
        GazeHeadBaseline=Mesh->GetBoneQuaternion(TEXT("head"),EBoneSpaces::ComponentSpace);
        FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*(FPaths::ProjectDir()/TEXT("Preview/Gaze")));
        MotionTiming=TEXT("frame\tsimulation_seconds\tvisible_target\teye_contact\tspeaking\ttarget_yaw\thead_yaw\teye_residual_yaw\ttarget_pitch\thead_pitch\teye_residual_pitch\tcamera\thead_quaternion\teye_quaternion\n");
    }
    if(T>=11){
        bool Field=false;auto* Tissue=Subject->FindComponentByClass<UHALVETHTissueComponent>();
        if(Tissue){Tissue->LogAudit();Field=Tissue->VerifyGpuField()&&Tissue->HasDeformedSurface();}
        const bool Passed=GazeVisibleSamples>30&&GazeOccludedSamples>5&&GazeLeadSamples>2&&GazeSpeakingSamples>5&&GazeEyeTravel>.02&&GazeHeadTravel>.02&&Field;
        FFileHelper::SaveStringToFile(MotionTiming,*(FPaths::ProjectDir()/TEXT("QA/gaze-frames.tsv")));
        const double ObservedHz=MotionFrame>1?(MotionFrame-1)/(MotionLastSample-MotionFirstSample):0;
        const FString Receipt=FString::Printf(TEXT("{\"schema\":\"halveth.gaze-native.v1\",\"passed\":%s,\"frames\":%d,\"observedHz\":%.9f,\"visibleSamples\":%d,\"occludedSamples\":%d,\"eyeLeadSamples\":%d,\"speakingSamples\":%d,\"observedEyeTravelRadians\":%.9f,\"observedHeadTravelRadians\":%.9f,\"sharedFieldPassed\":%s,\"isFrameRateBenchmark\":false,\"scope\":\"Declared moving player camera; one stationary live guide, actual interaction and blocking wall; no forced gaze values\"}\n"),Passed?TEXT("true"):TEXT("false"),MotionFrame,ObservedHz,GazeVisibleSamples,GazeOccludedSamples,GazeLeadSamples,GazeSpeakingSamples,GazeEyeTravel,GazeHeadTravel,Field?TEXT("true"):TEXT("false"));
        FFileHelper::SaveStringToFile(Receipt,*(FPaths::ProjectDir()/TEXT("QA/native-gaze.json")));
        UE_LOG(LogTemp,Display,TEXT("GAZE_NATIVE_WITNESS result=%s frames=%d visible=%d occluded=%d eye_lead=%d speaking=%d actual_eye_travel_rad=%.6f actual_head_travel_rad=%.6f field=%d"),Passed?TEXT("PASS"):TEXT("FAIL"),MotionFrame,GazeVisibleSamples,GazeOccludedSamples,GazeLeadSamples,GazeSpeakingSamples,GazeEyeTravel,GazeHeadTravel,Field);
        bMotionCapture=false;FPlatformMisc::RequestExitWithStatus(false,Passed?0:4);return;
    }
    Subject->GetCharacterMovement()->StopMovementImmediately();
    const FVector Face=Mesh->GetComponentTransform().TransformPosition(Motion->RefGazeOrigin);
    const FVector Front=Mesh->GetComponentTransform().TransformVectorNoScale(Motion->Forward).GetSafeNormal();
    const FVector Right=Mesh->GetComponentTransform().TransformVectorNoScale(Motion->Right).GetSafeNormal();
    const double Side=T<1?0:T<2.8?-64:T<4.6?64:0;
    const FVector Eye=Face+Front*170+Right*Side+FVector(0,0,5);
    P->SetActorLocation(Eye-FVector(0,0,66),false,nullptr,ETeleportType::TeleportPhysics);
    PC->SetControlRotation((Face-Eye).Rotation());
    if(T>=4.6&&MotionStage==0){P->GetAdventure()->InteractWithNearbyCharacter();MotionStage=1;}
    if(T>=6&&T<7.2)PC->SetControlRotation((Eye-Face).Rotation());
    if(T>=7.2&&T<8.4&&!GazeOccluder){
        auto* Wall=GetWorld()->SpawnActor<AStaticMeshActor>();
        Wall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        Wall->SetActorLocation((Face+Eye)*.5);Wall->SetActorRotation(Front.Rotation());
        Wall->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Wall->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Characters/M_AppearanceBackdrop.M_AppearanceBackdrop")));
        Wall->SetActorScale3D(FVector(.06,2,2));Wall->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
        GazeOccluder=Wall;
    }
    if(T>=8.4&&GazeOccluder){GazeOccluder->Destroy();GazeOccluder=nullptr;}
    GazeEyeTravel=FMath::Max(GazeEyeTravel,double(Mesh->GetBoneQuaternion(TEXT("eye_L"),EBoneSpaces::ComponentSpace).AngularDistance(GazeEyeBaseline)));
    GazeHeadTravel=FMath::Max(GazeHeadTravel,double(Mesh->GetBoneQuaternion(TEXT("head"),EBoneSpaces::ComponentSpace).AngularDistance(GazeHeadBaseline)));
    if(T+1e-6>=double(MotionFrame)/15.){
        if(MotionFirstSample<0)MotionFirstSample=T;MotionLastSample=T;
        GazeVisibleSamples+=Motion->VisibleLookTarget?1:0;
        GazeOccludedSamples+=(T>7.45&&T<8.35&&!Motion->VisibleLookTarget)?1:0;
        GazeLeadSamples+=(Motion->VisibleLookTarget&&FMath::Abs(Motion->EyeYaw)>.06&&FMath::Abs(Motion->DesiredLookYaw-Motion->LookYaw)>.08)?1:0;
        GazeSpeakingSamples+=Motion->Speaking?1:0;
        const FString Name=FString::Printf(TEXT("frame-%04d.png"),MotionFrame);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/TEXT("Preview/Gaze")/Name,false,false);
        const FQuat Head=Mesh->GetBoneQuaternion(TEXT("head"),EBoneSpaces::ComponentSpace),Eyes=Mesh->GetBoneQuaternion(TEXT("eye_L"),EBoneSpaces::ComponentSpace);
        MotionTiming+=FString::Printf(TEXT("%d\t%.9f\t%d\t%d\t%d\t%.9f\t%.9f\t%.9f\t%.9f\t%.9f\t%.9f\t%s\t%s\t%s\n"),MotionFrame,T,Motion->VisibleLookTarget,Motion->ViewerEyeContact,Motion->Speaking,Motion->DesiredLookYaw,Motion->LookYaw,Motion->EyeYaw,Motion->DesiredLookPitch,Motion->LookPitch,Motion->EyePitch,*Eye.ToString(),*Head.ToString(),*Eyes.ToString());
        Motion->LogPoseAudit(TEXT("gaze_frame"));MotionFrame++;
    }
}
