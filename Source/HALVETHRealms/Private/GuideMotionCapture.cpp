#include "HALVETHGameMode.h"
#include "HALVETHRealmWorld.h"
#include "HALVETHCharacter.h"
#include "HALVETHAdventureComponent.h"
#include "HALVETHGuideCharacter.h"
#include "HALVETHGuideAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "HighResScreenshot.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformMisc.h"

// Explicit native capture only; ordinary play keeps its input and camera.
void AHALVETHGameMode::RunMotionCapture(float Dt){
    auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* PC=GetWorld()->GetFirstPlayerController();if(!P||!PC||!RealmWorld)return;
    MotionElapsed+=Dt;if(MotionElapsed<5)return;
    const bool Sparse=FParse::Param(FCommandLine::Get(),TEXT("HalvethSparseCapture"));
    double T=MotionElapsed-5;int Stage=T<5?0:T<10?1:T<15?2:T<20?3:4;
    FVector Guide=RealmWorld->GetGuidePosition(1);
    if(Stage!=MotionStage){
        MotionStage=Stage;
        P->DisableInput(PC);PC->SetIgnoreLookInput(true);PC->SetIgnoreMoveInput(true);
        P->GetCharacterMovement()->SetMovementMode(MOVE_Flying);ApplyQuality(2,false);
        if(auto* HUD=PC->GetHUD())HUD->bShowHUD=false;
        const FVector Offsets[]={FVector(0,-800,64),FVector(0,-260,54),FVector(-190,-260,86),FVector(0,-800,150)};
        if(Stage<4)P->SetActorLocation(Guide+Offsets[Stage],false,nullptr,ETeleportType::TeleportPhysics);
        if(Stage==0)if(auto* Actor=RealmWorld->GetGuideCharacter(1)){
            MotionDropStartZ=Actor->GetActorLocation().Z;
            Actor->SetActorLocation(Actor->GetActorLocation()+FVector(0,0,150),false,nullptr,ETeleportType::TeleportPhysics);
            Actor->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        }
        if(Stage==1||Stage==2)P->GetAdventure()->InteractWithNearbyCharacter();
        UE_LOG(LogTemp,Display,TEXT("GARDEN_MOTION_STAGE stage=%d simulation_time=%.5f"),Stage,T);
    }
    if(Stage>=4){
        const bool GravityPassed=MotionDropFalling&&MotionDropLanded&&MotionDropMinVz<-100&&MotionDropLandingError<4;
        const bool Passed=RealmWorld->VerifyCharacters()&&RealmWorld->VerifyGroundDynamics()&&RealmWorld->VerifySkyDynamics()&&GravityPassed&&(!Sparse||RealmWorld->VerifyGuidePatrol());
        UE_LOG(LogTemp,Display,TEXT("GARDEN_GRAVITY_DROP_AUDIT passed=%d lift_cm=150 falling_observed=%d landed=%d min_vertical_velocity_cm_s=%.5f landing_height_difference_cm=%.5f"),GravityPassed,MotionDropFalling,MotionDropLanded,MotionDropMinVz,MotionDropLandingError);
        const FString TimingFile=Sparse?TEXT("world-scale-motion-frames.tsv"):TEXT("motion-frames.tsv");
        FFileHelper::SaveStringToFile(MotionTiming,*(FPaths::ProjectDir()/TEXT("QA")/TimingFile));
        FString Receipt=FString::Printf(TEXT("{\"nativeMotionPassed\":%s,\"requestedFrames\":%d,\"simulationSeconds\":%.5f,\"nominalSamplingHz\":%d,\"timingFile\":\"%s\",\"scope\":\"Native walking, proximity stop, dialogue and gaze sequence\"}"),Passed?TEXT("true"):TEXT("false"),MotionFrame,T,Sparse?2:24,*TimingFile);
        FFileHelper::SaveStringToFile(Receipt,*(FPaths::ProjectDir()/TEXT("QA")/(Sparse?TEXT("native-world-scale-motion.json"):TEXT("native-motion-capture.json"))));
        bMotionCapture=false;FPlatformMisc::RequestExitWithStatus(false,Passed?0:4);return;
    }
    if(auto* Actor=RealmWorld->GetGuideCharacter(1)){
        const auto* Movement=Actor->GetCharacterMovement();MotionDropMinVz=FMath::Min(MotionDropMinVz,double(Movement->Velocity.Z));
        if(Movement->IsFalling())MotionDropFalling=true;
        if(MotionDropFalling&&Movement->IsMovingOnGround()&&!MotionDropLanded){MotionDropLanded=true;MotionDropLandingError=FMath::Abs(Actor->GetActorLocation().Z-MotionDropStartZ);}
    }
    const FVector FollowOffsets[]={FVector(0,-800,64),FVector(0,-260,54),FVector(-190,-260,86),FVector(0,-800,150)};
    P->SetActorLocation(Guide+FollowOffsets[Stage],false,nullptr,ETeleportType::TeleportPhysics);
    FVector Target=Guide+FVector(0,0,Stage==2?150:105);
    PC->SetControlRotation((Target-(P->GetActorLocation()+FVector(0,0,66))).Rotation());
    if(T>=MotionNextFrame){
        for(int Identity=1;Identity<=3;Identity++)if(auto* Actor=RealmWorld->GetGuideCharacter(Identity))
            if(auto* Motion=Cast<UHALVETHGuideAnimInstance>(Actor->GetMesh()->GetAnimInstance()))Motion->LogPoseAudit(TEXT("motion_frame"));
        const FString Folder=FPaths::ProjectDir()/(Sparse?TEXT("Preview/WorldScale"):TEXT("Preview/Motion"));
        FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*Folder);
        FString Name=FString::Printf(TEXT("frame-%05d.png"),MotionFrame);
        FScreenshotRequest::RequestScreenshot(Folder/Name,false,false);
        MotionTiming+=FString::Printf(TEXT("%d\t%.9f\t%d\t%s\t%.3f\n"),MotionFrame,T,Stage,*Name,RealmWorld->GetSkyClockUnix());
        MotionFrame++;MotionNextFrame=T+(Sparse?.5:1./24);
    }
}
