// Copyright (c) 2026 Juri Janovski. SPDX-License-Identifier: LicenseRef-HALVETH-PIRL-2.0
#include "HALVETHMeadow.h"
#include "HALVETHRealmWorld.h"
#include "GardenField.h"
#include "MeadowModel.h"
#include "HALVETHCharacter.h"
#include "HALVETHKnowledgeComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "HighResScreenshot.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Serialization/JsonSerializer.h"

AHALVETHMeadow::AHALVETHMeadow(){
    PrimaryActorTick.bCanEverTick=true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("MeadowRoot")));
    Cells.Init(FIntPoint(MAX_int32,MAX_int32),Grid*Grid);
    LivePerCell.Init(0,Grid*Grid);
}
void AHALVETHMeadow::Initialize(AHALVETHRealmWorld* Source){
    Field=Source;
    CaptureMode=FParse::Param(FCommandLine::Get(),TEXT("HalvethMeadowCapture"));
    Sequence=FParse::Param(FCommandLine::Get(),TEXT("HalvethMeadowSequence"));
    Benchmark=FParse::Param(FCommandLine::Get(),TEXT("HalvethMeadowBenchmark"));
    CellPhases.SetNum(4);for(auto& Values:CellPhases)Values.Init(0,Grid*Grid);
    for(int32 V=0;V<4;V++){
        const FString Path=FString::Printf(TEXT("/Game/Garden/Meadow_%d.Meadow_%d"),V,V);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
        if(!Mesh){UE_LOG(LogTemp,Error,TEXT("MEADOW_ASSET_MISSING variant=%d"),V);return;}
        auto* C=NewObject<UInstancedStaticMeshComponent>(this);
        AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetStaticMesh(Mesh);
        C->SetMobility(EComponentMobility::Movable);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCanEverAffectNavigation(false);C->SetCastShadow(false);
        C->SetCullDistances(1900,3500);C->SetBoundsScale(1.2f);C->NumCustomDataFloats=1;C->RegisterComponent();
        TArray<FTransform> Empty;Empty.Init(FTransform(FQuat::Identity,FVector(0,0,-20000),FVector::ZeroVector),Grid*Grid);
        C->AddInstances(Empty,false,false,false);Groups.Add(C);
    }
}
void AHALVETHMeadow::RefreshCell(int32 Slot,FIntPoint Cell){
    RootedClumps-=LivePerCell[Slot];LivePerCell[Slot]=0;
    // One deterministic stream per absolute ground cell, independent of travel
    // order and recycling. Every return restores the same species/pose/roots.
    const uint32 Seed=uint32(Cell.X)*374761393u+uint32(Cell.Y)*668265263u+uint32(Realm+1)*1442695041u;
    FRandomStream Random{int32(Seed)};
    for(int32 V=0;V<4;V++){
        const double X=Cell.X*200.+(V%2)*100.+Random.FRandRange(8.,92.);
        const double Y=Cell.Y*200.+(V/2)*100.+Random.FRandRange(8.,92.);
        const FVector4f Biome=GardenField::Color(X,Y,Realm);
        FVector Ground,Normal;
        const bool Architecture=Realm==0&&X>1270&&X<3730&&Y>-1730&&Y<3580;
        const bool InWorld=FMath::Abs(X)<29200&&FMath::Abs(Y)<29200;
        const bool Suitable=InWorld&&!Architecture&&Biome.Y>.43f&&Biome.X<.52f&&
            Random.FRand()<FMath::Clamp((double(Biome.Y)-.28)*1.6,0.,.98)&&
            Field->TraceMeadowGround(X,Y,Ground,Normal)&&Ground.Z>-30&&Normal.Z>.82;
        FTransform Pose(FQuat::Identity,FVector(0,0,-20000),FVector::ZeroVector);
        if(Suitable){
            const FQuat Align=FQuat::FindBetweenNormals(FVector::UpVector,Normal);
            const FQuat Yaw(FVector::UpVector,Random.FRandRange(0.,2*PI));
            const float Scale=Random.FRandRange(.88f,1.18f);
            Pose=FTransform(Align*Yaw,Ground-Normal*1.2,FVector(Scale));LivePerCell[Slot]++;
        }
        Groups[V]->UpdateInstanceTransform(Slot,Pose,false,false,true);
        CellPhases[V][Slot]=Random.FRand();Groups[V]->SetCustomDataValue(Slot,0,CellPhases[V][Slot],false);
    }
    RootedClumps+=LivePerCell[Slot];Cells[Slot]=Cell;
}
void AHALVETHMeadow::Tick(float Dt){
    Super::Tick(Dt);if(!Field||Groups.Num()!=4)return;
    auto* Pawn=UGameplayStatics::GetPlayerPawn(this,0);if(!Pawn)return;
    if(Realm!=Field->GetCurrentRealm()){
        Realm=Field->GetCurrentRealm();Cells.Init(FIntPoint(MAX_int32,MAX_int32),Grid*Grid);
        LivePerCell.Init(0,Grid*Grid);RootedClumps=0;Cursor=0;Announced=false;
        // Hide old-realm geometry immediately, then repopulate under a ray budget.
        for(auto& Group:Groups)for(int32 I=0;I<Grid*Grid;I++)
            Group->UpdateInstanceTransform(I,FTransform(FQuat::Identity,FVector(0,0,-20000),FVector::ZeroVector),false,false,true);
    }
    const FVector P=Pawn->GetActorLocation();
    const FIntPoint Low(FMath::FloorToInt(P.X/200.)-Grid/2,FMath::FloorToInt(P.Y/200.)-Grid/2);
    auto Mod=[](int32 V){return (V%Grid+Grid)%Grid;};
    int32 Changed=0,Checked=0;
    // At most 96 terrain traces per frame. No per-blade CPU tick and no
    // unbounded landscape-wide instance allocation as the player explores.
    while(Changed<24&&Checked<Grid*Grid){
        const int32 Slot=Cursor;Cursor=(Cursor+1)%(Grid*Grid);Checked++;
        const FIntPoint Cell(Low.X+Mod(Slot%Grid-Mod(Low.X)),Low.Y+Mod(Slot/Grid-Mod(Low.Y)));
        if(Cells[Slot]!=Cell){RefreshCell(Slot,Cell);Changed++;}
    }
    if(Changed)for(auto& Group:Groups)Group->MarkRenderStateDirty();
    if(!Announced&&!Cells.Contains(FIntPoint(MAX_int32,MAX_int32))){
        Announced=true;UE_LOG(LogTemp,Display,TEXT("MEADOW_WINDOW_READY realm=%d rooted_clumps=%d slots=%d variants=4 blade_cpu_ticks=0 ground=triangle_trace architecture_exclusion=1"),Realm,RootedClumps,GetSlotBudget());
    }
    if(CaptureMode)TickCapture(Dt);
}

void AHALVETHMeadow::TickCapture(float Dt){
    if(CaptureDone)return;CaptureAge+=Dt;
    const double Now=FPlatformTime::Seconds();if(LastFrame>0&&CaptureAge>3)FrameMs.Add((Now-LastFrame)*1000);LastFrame=Now;
    auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* PC=UGameplayStatics::GetPlayerController(this,0);if(!P||!PC)return;
    const int32 Stage=Benchmark?0:FMath::FloorToInt(CaptureAge/4.0);
    if((Benchmark&&CaptureAge>=12)||Stage>=4){
        CaptureDone=true;const bool Contact=ExportScene();FrameMs.Sort();
        const double Median=FrameMs.IsEmpty()?0:FrameMs[FrameMs.Num()/2];
        const double P95=FrameMs.IsEmpty()?0:FrameMs[FMath::Min(FrameMs.Num()-1,FMath::FloorToInt(FrameMs.Num()*.95))];
        const bool Isolated=P->GetKnowledge()&&P->GetKnowledge()->UsesIsolatedSave();
        const bool Good=RootedClumps>500&&Groups.Num()==4&&Isolated&&Contact;
        UE_LOG(LogTemp,Display,TEXT("MEADOW_SAVE_ISOLATION actual_test_slot=%d"),Isolated);
        UE_LOG(LogTemp,Display,TEXT("MEADOW_NATIVE_CAPTURE result=%s rooted_clumps=%d budget=%d frame_samples=%d median_ms=%.3f p95_ms=%.3f includes_capture_and_scene_changes=1"),Good?TEXT("PASS"):TEXT("FAIL"),RootedClumps,GetSlotBudget(),FrameMs.Num(),Median,P95);
        if(Benchmark)UE_LOG(LogTemp,Display,TEXT("MEADOW_QUIET_FRAME_SAMPLE warmup_s=3 measured_simulation_s=9 frames=%d median_ms=%.3f p95_ms=%.3f screenshots=0 camera_changes_after_setup=0 quality=1 cap_fps=60"),FrameMs.Num(),Median,P95);
        FPlatformMisc::RequestExitWithStatus(false,Good?0:6);return;
    }
    if(Stage!=CaptureStage){
        CaptureStage=Stage;
        if(!P->IsAstralFlying())P->ToggleAstralFlight();P->GetCharacterMovement()->StopMovementImmediately();
        const FVector2D XY[]={FVector2D(4650,-4300),FVector2D(4700,-3600),FVector2D(6800,-4700),FVector2D(9000,-9800)};
        if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;
        const double Heights[]={165,105,180,1900};const FRotator Views[]={FRotator(-8,119,0),FRotator(-36,133,0),FRotator(-7,-45,0),FRotator(-14,123,0)};
        FVector Ground,Normal;if(!Field->TraceMeadowGround(XY[Stage].X,XY[Stage].Y,Ground,Normal))Ground=FVector(XY[Stage],0);
        P->SetActorLocation(Ground+FVector(0,0,Heights[Stage]),false,nullptr,ETeleportType::TeleportPhysics);PC->SetControlRotation(Views[Stage]);
        UE_LOG(LogTemp,Display,TEXT("MEADOW_CAPTURE_CAMERA stage=%d world_cm=(%.4f,%.4f,%.4f) pitch=%.4f yaw=%.4f"),Stage,P->GetActorLocation().X,P->GetActorLocation().Y,P->GetActorLocation().Z,Views[Stage].Pitch,Views[Stage].Yaw);
    }
    // Native screenshot requests execute on a later frame. Keep the pose for a
    // full second after requesting it before moving to another scene.
    const double Within=FMath::Fmod(CaptureAge,4.0);
    if(Sequence&&Stage==1&&Within>.3&&Within<3.7&&FApp::CanEverRender()){
        const int32 Bucket=FMath::FloorToInt((Within-.3)*8);
        if(Bucket>SequenceFrame){
            SequenceFrame=Bucket;const FString Directory=FPaths::ProjectDir()/TEXT("Preview/MeadowWind");IFileManager::Get().MakeDirectory(*Directory,true);
            FScreenshotRequest::RequestScreenshot(Directory/FString::Printf(TEXT("WIND_%03d.png"),Bucket),false,false);
            UE_LOG(LogTemp,Display,TEXT("MEADOW_SEQUENCE_FRAME bucket=%d request_time_s=%.6f"),Bucket,CaptureAge);
        }
    }
    const double ShotAt=Sequence&&Stage==1?3.8:2.6;
    if(!Benchmark&&Within>=ShotAt&&Within<ShotAt+Dt&&FApp::CanEverRender()){
        const TCHAR* Names[]={TEXT("MEADOW_GATE"),TEXT("MEADOW_ROOTS"),TEXT("MEADOW_FIELD"),TEXT("MEADOW_FLIGHT")};
        const FString Directory=FPaths::ProjectDir()/TEXT("Preview");IFileManager::Get().MakeDirectory(*Directory,true);
        FScreenshotRequest::RequestScreenshot(Directory/(FString(Names[Stage])+TEXT(".png")),true,false);
    }
}

bool AHALVETHMeadow::ExportScene(){
    int32 Slot=INDEX_NONE;FTransform Pose;
    auto* Pawn=UGameplayStatics::GetPlayerPawn(this,0);double Distance=TNumericLimits<double>::Max();FTransform Candidate;
    for(int32 I=0;I<Grid*Grid;I++)if(Groups[0]->GetInstanceTransform(I,Candidate,true)&&Candidate.GetScale3D().Z>.1){
        const double D=Pawn?FVector::DistSquared2D(Candidate.GetLocation(),Pawn->GetActorLocation()):I;
        if(D<Distance){Distance=D;Slot=I;Pose=Candidate;}
    }
    if(Slot==INDEX_NONE)return false;
    auto Doc=MakeShared<FJsonObject>();Doc->SetStringField(TEXT("schema"),TEXT("halveth.meadow-source-scene.v1"));
    Doc->SetStringField(TEXT("evidence"),TEXT("NATIVE_GENERATED_GEOMETRY_AND_DECLARED_PROJECTIONS_NOT_PIXEL_READBACK"));
    Doc->SetStringField(TEXT("coordinates"),TEXT("Unreal world centimetres, Z-up; CPU double model; no GPU position readback"));
    Doc->SetNumberField(TEXT("cellX"),Cells[Slot].X);Doc->SetNumberField(TEXT("cellY"),Cells[Slot].Y);
    Doc->SetNumberField(TEXT("instancePhase"),CellPhases[0][Slot]);
    TArray<TSharedPtr<FJsonValue>> Points;double MaxRootGap=0,MaxLiftedGap=0;int32 RootChecks=0;
    for(uint32 I=0;I<16;I++)for(int32 J=0;J<3;J++){
        const auto Blade=HalvethMeadow::MakeBlade(0,I);const auto Local=HalvethMeadow::BladePoint(Blade,J*.5,0);
        const FVector World=Pose.TransformPosition(FVector(Local.x,Local.y,Local.z));
        if(J==0){FVector Ground,Normal;if(Field->TraceMeadowGround(World.X,World.Y,Ground,Normal)){MaxRootGap=FMath::Max(MaxRootGap,FMath::Abs(World.Z-Ground.Z));MaxLiftedGap=FMath::Max(MaxLiftedGap,FMath::Abs(World.Z+50-Ground.Z));RootChecks++;}}
        const auto Wind=HalvethMeadow::Wind({World.X,World.Y,World.Z},J*.5,1.,Blade.phase+CellPhases[0][Slot]*2*PI);
        auto Point=MakeShared<FJsonObject>();Point->SetNumberField(TEXT("id"),I*3+J);Point->SetNumberField(TEXT("blade"),I);Point->SetNumberField(TEXT("heightFraction"),J*.5);
        Point->SetNumberField(TEXT("x"),World.X);Point->SetNumberField(TEXT("y"),World.Y);Point->SetNumberField(TEXT("z"),World.Z);
        Point->SetNumberField(TEXT("windXAt1s"),Wind.x);Point->SetNumberField(TEXT("windYAt1s"),Wind.y);Point->SetNumberField(TEXT("windZAt1s"),Wind.z);
        Points.Add(MakeShared<FJsonValueObject>(Point));
    }
    Doc->SetArrayField(TEXT("points"),Points);
    Doc->SetNumberField(TEXT("rootChecks"),RootChecks);Doc->SetNumberField(TEXT("maxRootGapCm"),MaxRootGap);
    const bool Grounded=RootChecks==16&&MaxRootGap<5;
    Doc->SetBoolField(TEXT("sampledRootContactPassed"),Grounded);
    const bool LiftRejected=RootChecks==16&&MaxLiftedGap>=5;
    Doc->SetBoolField(TEXT("synthetic50cmLiftRejected"),LiftRejected);
    UE_LOG(LogTemp,Display,TEXT("MEADOW_ROOT_CONTACT samples=%d max_gap_cm=%.6f tolerance_cm=5 synthetic_50cm_lift_rejected=%d passed=%d"),RootChecks,MaxRootGap,LiftRejected,Grounded);
    FString Text;FJsonSerializer::Serialize(Doc,TJsonWriterFactory<>::Create(&Text));
    const FString Directory=FPaths::ProjectDir()/TEXT("QA");IFileManager::Get().MakeDirectory(*Directory,true);
    const bool Saved=FFileHelper::SaveStringToFile(Text,*(Directory/TEXT("meadow-source-scene.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    return Grounded&&LiftRejected&&Saved;
}
