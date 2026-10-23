#include "HALVETHViewProbe.h"
#include "HALVETHGameMode.h"
#include "HALVETHRealmWorld.h"
#include "HALVETHAstralSanctum.h"
#include "HALVETHCharacter.h"
#include "HALVETHPlayerController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HighResScreenshot.h"

AHALVETHViewProbe::AHALVETHViewProbe(){PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickGroup=TG_PostUpdateWork;}
void AHALVETHViewProbe::Tick(float Dt){
    Super::Tick(Dt);if(Finished)return;Elapsed+=Dt;if(Elapsed<1)return;
    if(FParse::Param(FCommandLine::Get(),TEXT("HalvethNightCapture")))Capture();else Probe();
}
void AHALVETHViewProbe::Finish(bool Passed,const TCHAR* Detail){
    Finished=true;UE_LOG(LogTemp,Display,TEXT("HALVETH_VIEW_PROBE result=%s detail=%s"),Passed?TEXT("PASS"):TEXT("FAIL"),Detail);
    FPlatformMisc::RequestExitWithStatus(false,Passed?0:4);
}
void AHALVETHViewProbe::Probe(){
    auto* PC=GetWorld()->GetFirstPlayerController();auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!PC||!P||!Cast<AHALVETHPlayerCameraManager>(PC->PlayerCameraManager)){Finish(false,TEXT("camera_missing_or_wrong_class"));return;}
    if(Phase==0){
        // Same engine camera clamp as the old path, challenged with continued input.
        auto* Legacy=GetWorld()->SpawnActor<APlayerCameraManager>();FRotator View=FRotator::ZeroRotator;
        for(int32 I=0;I<300;I++){FRotator Delta(10,0,0);Legacy->ProcessViewRotation(1.f/60,View,Delta);}
        LegacyRejected=View.Pitch<90&&View.Pitch>80;Legacy->Destroy();
        PC->SetControlRotation(FRotator::ZeroRotator);Phase=1;Frame=0;return;
    }
    if(Phase==3){
        const FQuat Target=FRotator(24,-130,0).Quaternion();
        const double Error=PC->PlayerCameraManager->GetCameraRotation().Quaternion().AngularDistance(Target);
        auto* Mode=Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this));
        const bool Sky=Mode&&Mode->GetRealmWorld()&&Mode->GetRealmWorld()->VerifySkyDynamics();
        const bool Passed=LegacyRejected&&Zenith>=2&&Nadir>=2&&PitchArc>4*PI&&Arc>4*PI&&MaxStep<.20&&MinBodyUp>.999&&Error<.001&&Sky;
        UE_LOG(LogTemp,Display,TEXT("HALVETH_VIEW_WITNESS pitch_arc_deg=%.3f yaw_arc_deg=%.3f zenith=%d nadir=%d max_step_deg=%.3f body_up_min=%.6f rebase_error_deg=%.6f legacy_clamp_rejected=%d source=controller_input_and_camera_component"),FMath::RadiansToDegrees(PitchArc),FMath::RadiansToDegrees(Arc),Zenith,Nadir,FMath::RadiansToDegrees(MaxStep),MinBodyUp,FMath::RadiansToDegrees(Error),LegacyRejected);
        Finish(Passed,TEXT("full_rotation_poles_rebase_and_sky"));return;
    }
    const FQuat Current=PC->PlayerCameraManager->GetCameraRotation().Quaternion();
    if(Frame>3){
        if(HavePrevious){const double Step=Previous.AngularDistance(Current);Arc+=Step;MaxStep=FMath::Max(MaxStep,Step);}
        const double Z=Current.GetForwardVector().Z;const bool Up=Z>.98,Down=Z<-.98;
        if(Phase==1){if(Up&&!WasUp)Zenith++;if(Down&&!WasDown)Nadir++;}WasUp=Up;WasDown=Down;
        HavePrevious=true;MinBodyUp=FMath::Min(MinBodyUp,P->GetActorUpVector().Z);
        auto* Camera=P->FindComponentByClass<UCameraComponent>();
        if(!Camera||Camera->GetComponentQuat().AngularDistance(Current)>.001){UE_LOG(LogTemp,Error,TEXT("HALVETH_VIEW_DIAGNOSTIC frame=%d control=%s cache=%s component=%s"),Frame,*PC->GetControlRotation().ToString(),*PC->PlayerCameraManager->GetCameraRotation().ToString(),Camera?*Camera->GetComponentRotation().ToString():TEXT("NONE"));Finish(false,TEXT("camera_component_differs_from_controller"));return;}
    }
    Previous=Current;
    if(++Frame>=300){
        if(Phase==1){PitchArc=Arc;Arc=0;Phase=2;Frame=0;HavePrevious=false;PC->SetControlRotation(FRotator::ZeroRotator);}
        else{Phase=3;PC->SetControlRotation(FRotator(24,-130,0));}
        return;
    }
    // This is the same controller-input path used by the pawn's mouse bindings.
    if(Phase==1)P->AddControllerPitchInput(4);else P->AddControllerYawInput(4);
}
void AHALVETHViewProbe::Capture(){
    const double Times[]={1,7,8,12,13,17,18,22,23,27,28,32,34};
    if(CaptureStep>=UE_ARRAY_COUNT(Times)||Elapsed<Times[CaptureStep])return;
    auto* PC=GetWorld()->GetFirstPlayerController();auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* Mode=Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this));
    auto* Sanctum=Mode&&Mode->GetRealmWorld()?Mode->GetRealmWorld()->GetAstralSanctum():nullptr;
    if(!PC||!P||!Sanctum){Finish(false,TEXT("capture_scene_missing"));return;}
    auto Aim=[&](FVector At,FRotator View){P->GetCharacterMovement()->StopMovementImmediately();P->SetActorLocation(Sanctum->GetActorLocation()+At,false,nullptr,ETeleportType::TeleportPhysics);PC->SetControlRotation(View);};
    auto Shot=[&](const TCHAR* Name){
        const FString Directory=FPaths::ProjectDir()/TEXT("Preview");IFileManager::Get().MakeDirectory(*Directory,true);
        const FString File=Directory/(FString(Name)+TEXT(".png"));GetHighResScreenshotConfig().SetFilename(File);FScreenshotRequest::RequestScreenshot(File,false,false);
        UE_LOG(LogTemp,Display,TEXT("HALVETH_NIGHT_SHOT name=%s camera=%s position=%s"),Name,*PC->PlayerCameraManager->GetCameraRotation().ToString(),*PC->PlayerCameraManager->GetCameraLocation().ToString());
    };
    switch(CaptureStep){
        case 0:Aim(FVector(0,-1550,200),FRotator(5,90,0));break;
        case 1:Shot(TEXT("NIGHT_HORIZON"));break;
        case 2:Aim(FVector(0,-1550,200),FRotator(82,90,0));break;
        case 3:Shot(TEXT("NIGHT_ZENITH"));break;
        case 4:P->AddControllerPitchInput(-12);break;
        case 5:Shot(TEXT("NIGHT_OVER_ZENITH"));break;
        case 6:Aim(FVector(0,-1550,200),FRotator(-35,45,0));break;
        case 7:Shot(TEXT("NIGHT_GROUND"));break;
        case 8:Aim(FVector(0,-1550,200),FRotator(18,-90,0));break;
        case 9:Shot(TEXT("NIGHT_REVERSE"));break;
        case 10:Aim(FVector(350,-1950,200),FRotator(10,72,0));break;
        case 11:Shot(TEXT("NIGHT_GATE"));break;
        case 12:Finished=true;UE_LOG(LogTemp,Display,TEXT("HALVETH_NIGHT_CAPTURE result=PASS shots=6 scope=rendered_review_not_visual_acceptance"));FPlatformMisc::RequestExitWithStatus(false,0);break;
    }
    CaptureStep++;
}
