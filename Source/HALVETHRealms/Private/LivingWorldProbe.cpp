#include "HALVETHLivingWorld.h"
#include "HALVETHCharacter.h"
#include "HALVETHCitizen.h"
#include "HALVETHRealmWorld.h"
#include "HALVETHAstralSanctum.h"
#include "HALVETHAdventureComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HighResScreenshot.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

void AHALVETHLivingWorld::Shot(const TCHAR* Name){
    if(!CaptureMode||!FApp::CanEverRender())return;
    const FString Directory=FPaths::ProjectDir()/TEXT("Preview");IFileManager::Get().MakeDirectory(*Directory,true);
    const FString Path=Directory/(FString(Name)+TEXT(".png"));GetHighResScreenshotConfig().SetFilename(Path);FScreenshotRequest::RequestScreenshot(Path,false,false);
    UE_LOG(LogTemp,Display,TEXT("LIVING_CAPTURE_SHOT name=%s time=%.3f bodies=%d notice=%d"),Name,Time,Bodies.Num(),int32(Notice.stage));
}
void AHALVETHLivingWorld::FinishProbe(bool Passed,const TCHAR* Reason){
    ProbeFinished=true;int32 Moved=0,PawnBases=0;bool RejectsCharacterStep=true,StepNegativeControl=false;
    auto* Player=UGameplayStatics::GetPlayerPawn(this,0);
    for(const auto& B:Bodies)if(B.Actor){
        Moved+=B.Travel>20;
        if(auto* Base=B.Actor->GetMovementBase())PawnBases+=Base->GetOwner()&&Base->GetOwner()->IsA<APawn>();
        auto* Capsule=B.Actor->GetCapsuleComponent();RejectsCharacterStep&=!Capsule->CanCharacterStepUp(Player);
        if(!StepNegativeControl){const auto Previous=Capsule->CanCharacterStepUpOn;Capsule->CanCharacterStepUpOn=ECB_Yes;StepNegativeControl=Capsule->CanCharacterStepUp(Player);Capsule->CanCharacterStepUpOn=Previous;}
    }
    const bool Grounded=PawnBases==0&&RejectsCharacterStep&&StepNegativeControl;Passed&=Grounded;
    UE_LOG(LogTemp,Display,TEXT("LIVING_GROUNDING pawn_bases=%d character_step_rejected=%d engine_step_negative_control=%d passed=%d"),PawnBases,RejectsCharacterStep,StepNegativeControl,Grounded);
    if(FrameTimes.Num()>10){FrameTimes.Sort();UE_LOG(LogTemp,Display,TEXT("LIVING_FRAME_SAMPLE frames=%d median_ms=%.3f p95_ms=%.3f body_brain_ms=%.3f remote_cohort_ms=%.3f"),FrameTimes.Num(),FrameTimes[FrameTimes.Num()/2],FrameTimes[FMath::Min(FrameTimes.Num()-1,int(FrameTimes.Num()*.95))],MeasuredActorMs,MeasuredBackgroundMs);}
    UE_LOG(LogTemp,Display,TEXT("LIVING_NATIVE_PROBE result=%s reason=%s records=%u physical=%d moved=%d free_flight=%d indoor_flight=%d wall_occlusion=%d warnings=%d holds=%d released=%d"),Passed?TEXT("PASS"):TEXT("FAIL"),Reason,uint32(Residents.size()),Bodies.Num(),Moved,ProbeFreeFlight,ProbeIndoor,ProbeOcclusion,Notice.warnings,Notice.holds,Notice.stage==HalvethLife::Stage::Cooling);
    FPlatformMisc::RequestExitWithStatus(false,Passed?0:5);
}
void AHALVETHLivingWorld::Capture(){
    const double Now=FPlatformTime::Seconds();if(LastFrame>0&&Time>2&&!ProbeFinished)FrameTimes.Add((Now-LastFrame)*1000);LastFrame=Now;
}
void AHALVETHLivingWorld::RunProbe(){
    auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));auto* PC=GetWorld()->GetFirstPlayerController();
    auto* Sanctum=Realm?Realm->GetAstralSanctum():nullptr;auto* G=Guard();
    if(Time<3)return;
    if(!P||!PC||!Sanctum||!G||!G->Actor){UE_LOG(LogTemp,Error,TEXT("LIVING_FIXTURE player=%d controller=%d sanctum=%d guard=%d"),P!=nullptr,PC!=nullptr,Sanctum!=nullptr,G&&G->Actor);FinishProbe(false,TEXT("missing_scene_or_guard"));return;}
    const FVector Base=Sanctum->GetActorLocation();auto* Move=P->GetCharacterMovement();
    if(Time>40){FinishProbe(false,TEXT("sequence_timeout"));return;}
    if(ProbeStep==0){
        if(Bodies.Num()<10){FinishProbe(false,TEXT("population_not_materialized"));return;}
        const bool Locked=Sanctum->IsAttuned();ProbeFreeFlight=!Locked&&P->ToggleAstralFlight()&&P->IsAstralFlying();
        if(!ProbeFreeFlight){FinishProbe(false,TEXT("flight_still_locked"));return;}
        P->SetActorLocation(Base+FVector(-500,-1900,440),false,nullptr,ETeleportType::TeleportPhysics);Move->StopMovementImmediately();PC->SetControlRotation(FRotator(-5,75,0));
        ProbeStep=1;ProbeAt=Time;return;
    }
    if(ProbeStep==1){
        if(Time-ProbeAt<2)return;Shot(TEXT("LIVING_DISTRICT"));ProbeStep=11;ProbeAt=Time;return;
    }
    if(ProbeStep==11){
        // Screenshot requests are consumed at the end of a rendered frame.
        // Leave the camera in place before entering the next fixture.
        if(Time-ProbeAt<.25)return;
        P->SetActorLocation(Base+FVector(-350,2500,1320),false,nullptr,ETeleportType::TeleportPhysics);Move->StopMovementImmediately();PC->SetControlRotation(FRotator(0,0,0));
        ProbeFlightStart=P->GetActorLocation();ProbeStep=2;ProbeAt=Time;return;
    }
    if(ProbeStep==2){
        P->AddMovementInput(FVector(1,0,0),1,true);
        if(Time-ProbeAt<.8)return;
        const FVector Delta=P->GetActorLocation()-ProbeFlightStart;ProbeIndoor=P->IsAstralFlying()&&Delta.X>200&&FMath::Abs(Delta.Z)<5;
        UE_LOG(LogTemp,Display,TEXT("LIVING_INDOOR_FLIGHT passed=%d dx_cm=%.3f dz_cm=%.3f rings_attuned=%d"),ProbeIndoor,Delta.X,Delta.Z,Sanctum->IsAttuned());
        if(!ProbeIndoor){FinishProbe(false,TEXT("indoor_flight_failed"));return;}Move->StopMovementImmediately();Shot(TEXT("LIVING_INDOOR_FLIGHT"));ProbeStep=12;ProbeAt=Time;return;
    }
    if(ProbeStep==12){
        if(Time-ProbeAt<.25)return;
        Move->StopMovementImmediately();P->SetActorLocation(Base+FVector(0,650,480),false,nullptr,ETeleportType::TeleportPhysics);PC->SetControlRotation(FRotator(0,90,0));
        ProbeStep=3;ProbeAt=Time;return;
    }
    if(ProbeStep==3){
        P->AddMovementInput(FVector(0,1,0),1,true);if(Time-ProbeAt<1.3)return;
        const double DoorY=P->GetActorLocation().Y-Base.Y;
        UE_LOG(LogTemp,Display,TEXT("LIVING_FLIGHT_COLLISION passed=%d local_y=%.3f expected_max_y=870"),DoorY<870&&DoorY>740,DoorY);
        if(DoorY>=870||DoorY<=740){FinishProbe(false,TEXT("closed_door_collision_failed"));return;}
        Move->StopMovementImmediately();FVector GuardAt=Base+FVector(-300,-300,0);if(!Supported(GuardAt,G->Actor)){FinishProbe(false,TEXT("fixture_guard_floor_missing"));return;}
        G->Actor->SetActorLocation(GuardAt,false,nullptr,ETeleportType::TeleportPhysics);
        G->Goal=GuardAt;G->Steering=FVector::ZeroVector;G->TalkUntil=Time+20;G->Actor->GetCharacterMovement()->StopMovementImmediately();G->Actor->SetActorRotation(FRotator(0,90,0));
        for(auto& B:Bodies)if(B.Actor&&Residents[B.Id].guard&&B.Id!=G->Id){
            FVector OtherAt=GuardAt+FVector(-1500,-900,0);if(Supported(OtherAt,B.Actor))B.Actor->SetActorLocation(OtherAt,false,nullptr,ETeleportType::TeleportPhysics);
            B.Actor->SetActorRotation(FRotator(0,-90,0));B.Goal=B.Actor->GetActorLocation();B.Steering=FVector::ZeroVector;B.TalkUntil=Time+25;B.Actor->GetCharacterMovement()->StopMovementImmediately();
        }
        P->SetActorLocation(GuardAt+FVector(0,850,150),false,nullptr,ETeleportType::TeleportPhysics);PC->SetControlRotation((GuardAt+FVector(0,0,50)-P->GetActorLocation()-FVector(0,0,66)).Rotation());
        const bool Clear=Perceives(G->Actor,P);
        WitnessWall=NewObject<UStaticMeshComponent>(this,TEXT("SightNegativeControl"));AddInstanceComponent(WitnessWall);WitnessWall->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        WitnessWall->SetWorldLocation(GuardAt+FVector(0,425,110));WitnessWall->SetWorldScale3D(FVector(2,.25,4));WitnessWall->SetCollisionProfileName(TEXT("BlockAll"));WitnessWall->SetHiddenInGame(true);WitnessWall->RegisterComponent();
        ProbeOcclusion=Clear&&!Perceives(G->Actor,P);WitnessWall->DestroyComponent();WitnessWall=nullptr;
        UE_LOG(LogTemp,Display,TEXT("LIVING_PERCEPTION clear=%d wall_blocks=%d"),Clear,ProbeOcclusion);
        if(!ProbeOcclusion){FinishProbe(false,TEXT("wall_did_not_occlude_guard"));return;}
        ActiveGuard=G->Id;Notice={};ProbeFlightStart=GuardAt;ProbeHealth=P->GetAdventure()->GetHealth();ProbeStep=4;ProbeAt=Time;return;
    }
    if(ProbeStep==4||ProbeStep==5){
        if(Notice.stage==HalvethLife::Stage::Held){
            if(!P->IsCivicHeld()||P->GetAdventure()->GetHealth()!=ProbeHealth){FinishProbe(false,TEXT("hold_missing_or_damaging"));return;}
            const double Chased=FVector::Dist2D(G->Actor->GetActorLocation(),ProbeChaseStart);UE_LOG(LogTemp,Display,TEXT("LIVING_CHASE physical_travel_cm=%.3f since=pursuit_transition guard_id=%d"),Chased,G->Id);
            if(!ProbeSawPursuit||Chased<200){FinishProbe(false,TEXT("guard_did_not_physically_chase"));return;}
            ProbeFlightStart=P->GetActorLocation();ProbeStep=6;ProbeAt=Time;Shot(TEXT("LIVING_WARNING_HOLD"));return;
        }
        // The test fixes a dangerous fly-by in sight and near a real physical
        // resident. Normal runtime detection/perception/state transitions run unchanged.
        const FVector At=ProbeFlightStart;G->Actor->SetActorRotation(FRotator(0,90,0));
        if(Notice.stage==HalvethLife::Stage::Pursuit&&!ProbeSawPursuit){ProbeSawPursuit=true;ProbeChaseStart=G->Actor->GetActorLocation();}
        if(Notice.stage==HalvethLife::Stage::Pursuit)G->TalkUntil=0;else G->Steering=FVector::ZeroVector;
        P->SetActorLocation(At+FVector(0,850,150),false,nullptr,ETeleportType::TeleportPhysics);Move->Velocity=FVector(1200,0,0);
        // Keep actual flight input held through screenshot stalls; setting a
        // one-frame velocity alone permits normal braking before observation.
        P->AddMovementInput(FVector(1,0,0),1,true);
        for(auto& B:Bodies)if(B.Actor&&!Residents[B.Id].guard){B.Actor->SetActorLocation(At+FVector(160,850,0),false,nullptr,ETeleportType::TeleportPhysics);B.Goal=B.Actor->GetActorLocation();B.TalkUntil=Time+10;B.Steering=FVector::ZeroVector;B.Actor->GetCharacterMovement()->StopMovementImmediately();break;}
        if(Notice.stage==HalvethLife::Stage::Warning&&ProbeStep==4){ProbeStep=5;Shot(TEXT("LIVING_WARNING"));}
        return;
    }
    if(ProbeStep==6){
        if(Time-ProbeAt<2){if(!P->IsCivicHeld()||FVector::Dist(P->GetActorLocation(),ProbeFlightStart)>35){FinishProbe(false,TEXT("hold_not_stationary"));}return;}
        if(Time-ProbeAt<2.6)return;
        if(P->IsCivicHeld()||!P->IsAstralFlying()||!Move->IsFlying()||P->GetAdventure()->GetHealth()!=ProbeHealth){FinishProbe(false,TEXT("release_or_hover_restore_failed"));return;}
        Shot(TEXT("LIVING_RELEASED"));ProbeStep=7;ProbeAt=Time;ProbeFlightStart=P->GetActorLocation();return;
    }
    if(ProbeStep==7){
        P->AddMovementInput(FVector::UpVector,1,true);if(Time-ProbeAt<1)return;
        int32 Moved=0;for(const auto& B:Bodies)Moved+=B.Travel>20;
        const bool Released=P->GetActorLocation().Z-ProbeFlightStart.Z>100&&Notice.holds==1&&Notice.stage==HalvethLife::Stage::Cooling;
        FinishProbe(Released&&Moved>=3&&ProbeIndoor&&ProbeOcclusion&&ProbeFreeFlight,Released?TEXT("free_flight_population_occlusion_warning_hold_release"):TEXT("movement_after_release_failed"));
    }
}
