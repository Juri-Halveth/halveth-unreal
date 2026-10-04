#include "VoidGameMode.h"
#include "VoidWorld.h"
#include "VoidField.h"
#include "VoidCharacter.h"
#include "VoidHUD.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "Serialization/JsonSerializer.h"
#include "HighResScreenshot.h"
AVoidGameMode::AVoidGameMode() {
    DefaultPawnClass=AVoidCharacter::StaticClass(); HUDClass=AVoidHUD::StaticClass();
    PrimaryActorTick.bCanEverTick=true;
}
void AVoidGameMode::StartPlay() {
    FString SoulFile=FPaths::ProjectSavedDir()/TEXT("VOID-active-soul.json");
    FParse::Value(FCommandLine::Get(),TEXT("VoidSoul="),SoulFile);
    if(!Soul.Load(SoulFile)) { UE_LOG(LogTemp,Error,TEXT("VOID_SOUL_LOAD_FAILED: resolve a soulseed through VOID.sh")); FPlatformMisc::RequestExitWithStatus(false,2); return; }
    bTest=FParse::Param(FCommandLine::Get(),TEXT("VoidTest"));
    bVisual=FParse::Param(FCommandLine::Get(),TEXT("VoidVisual"));
    WorldBuilder=GetWorld()->SpawnActorDeferred<AVoidWorld>(AVoidWorld::StaticClass(), FTransform::Identity);
    WorldBuilder->Soul=Soul; WorldBuilder->FinishSpawning(FTransform::Identity);
    Super::StartPlay();
    if(APlayerController* PC=GetWorld()->GetFirstPlayerController()) {
        if(APawn* P=PC->GetPawn()) P->SetActorLocation(VoidField::Start(Soul),false,nullptr,ETeleportType::TeleportPhysics);
        PC->SetControlRotation(FRotator(-5,48,0));
    }
    GetWorld()->GetWorldSettings()->KillZ=-1000000;
}
void AVoidGameMode::Tick(float Delta) {
    Super::Tick(Delta);
    if(!bTest && !bVisual) return;
    TestTime+=Delta;
    if(TestTime>2.0) TestStep();
}
void AVoidGameMode::TestStep() {
    APlayerController* PC=GetWorld()->GetFirstPlayerController();
    AVoidCharacter* P=PC ? Cast<AVoidCharacter>(PC->GetPawn()) : nullptr;
    if(!P || !WorldBuilder || !WorldBuilder->bTerrainReady) {
        UE_LOG(LogTemp,Error,TEXT("VOID_TEST missing terrain or player")); FPlatformMisc::RequestExitWithStatus(false,2); return;
    }
    if(TestStage==0) {
        FCollisionQueryParams Q; Q.AddIgnoredActor(P);
        Q.bTraceComplex=true;
        for(int32 Y=-24000;Y<=24000;Y+=2400) for(int32 X=-24000;X<=24000;X+=2400) {
            FHitResult Hit;
            const double Expected=VoidField::Height(X,Y,Soul);
            bool Found=WorldBuilder->GetTerrain()->LineTraceComponent(Hit,FVector(X,Y,20000),FVector(X,Y,-20000),Q);
            // Trace the terrain itself, so a prop cannot mask missing ground.
            if(Found && FMath::Abs(Hit.ImpactPoint.Z-Expected)<=55 && Hit.ImpactNormal.Z>0) SupportPass++;
            else {
                SupportFail++;
                if(SupportFail<=5) UE_LOG(LogTemp,Display,TEXT("VOID_PROBE x=%d y=%d found=%d z=%.2f expected=%.2f normal_z=%.3f component=%s"),X,Y,Found,Hit.ImpactPoint.Z,Expected,Hit.ImpactNormal.Z,*GetNameSafe(Hit.Component.Get()));
            }
        }
        WalkOrigin=P->GetActorLocation(); TestStage=1;
    }
    if(TestStage==1) {
        if(TestTime<5.0) return;
        if(!P->GetCharacterMovement()->IsMovingOnGround()) Failures.Add(TEXT("spawn did not settle on solid terrain"));
        WalkOrigin=P->GetActorLocation();
        PC->SetControlRotation(FRotator(0,180,0));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W,IE_Pressed,1.0f));
        TestStage=2;
    }
    if(TestStage==2) {
        if(TestTime<9.0) return;
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W,IE_Released,0.0f));
        const FVector Now=P->GetActorLocation();
        if(FVector2D::Distance(FVector2D(Now.X,Now.Y),FVector2D(WalkOrigin.X,WalkOrigin.Y))<500) Failures.Add(TEXT("physical walk covered less than five metres"));
        if(!P->GetCharacterMovement()->IsMovingOnGround()) Failures.Add(TEXT("physical walk lost terrain support"));
        if(SupportFail>0) Failures.Add(FString::Printf(TEXT("%d of %d ground probes failed"),SupportFail,SupportPass+SupportFail));
        TestStage=3;
        TSharedRef<FJsonObject> Doc=MakeShared<FJsonObject>();
        Doc->SetStringField(TEXT("schema"),TEXT("halveth-void-native-qa/2.0"));
        Doc->SetStringField(TEXT("soulseed"),Soul.Text); Doc->SetStringField(TEXT("soul_id"),Soul.ID);
        Doc->SetNumberField(TEXT("ground_probe_pass"),SupportPass); Doc->SetNumberField(TEXT("ground_probe_fail"),SupportFail);
        Doc->SetNumberField(TEXT("trees"),WorldBuilder->TreeCount); Doc->SetNumberField(TEXT("rocks"),WorldBuilder->RockCount); Doc->SetNumberField(TEXT("ferns"),WorldBuilder->FernCount);
        Doc->SetNumberField(TEXT("walk_distance_cm"),FVector2D::Distance(FVector2D(Now.X,Now.Y),FVector2D(WalkOrigin.X,WalkOrigin.Y)));
        Doc->SetNumberField(TEXT("character_z_cm"),Now.Z);
        Doc->SetNumberField(TEXT("field_z_cm"),VoidField::Height(Now.X,Now.Y,Soul));
        Doc->SetBoolField(TEXT("passed"),Failures.IsEmpty());
        Doc->SetStringField(TEXT("scope"),TEXT("441 direct terrain traces within 55 cm of field; one physical spawn; four-second simulated W-key input through native binding; finite snapshot"));
        TArray<TSharedPtr<FJsonValue>> Errors; for(const FString& E:Failures) Errors.Add(MakeShared<FJsonValueString>(E));
        Doc->SetArrayField(TEXT("failures"),Errors); FString Out;
        FJsonSerializer::Serialize(Doc,TJsonWriterFactory<>::Create(&Out));
        FFileHelper::SaveStringToFile(Out,*(FPaths::ProjectSavedDir()/TEXT("VOID-QA.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        UE_LOG(LogTemp,Display,TEXT("VOID_QA passed=%d terrain=%d/%d"),Failures.IsEmpty(),SupportPass,SupportPass+SupportFail);
        if(bTest && !bVisual) { FPlatformMisc::RequestExitWithStatus(false,Failures.IsEmpty()?0:3); return; }
    }
    if(bVisual && TestStage==3 && TestTime>18) {
        P->SetActorLocation(VoidField::Start(Soul),false,nullptr,ETeleportType::TeleportPhysics);
        PC->SetControlRotation(FRotator(-4,43,0));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("VOID-shore.png"),false,false);
        TestStage=4;
    }
    if(bVisual && TestStage==4 && TestTime>21) {
        P->SetActorLocation(FVector(-17000,-15000,VoidField::Height(-17000,-15000,Soul)+700),false,nullptr,ETeleportType::TeleportPhysics);
        PC->SetControlRotation(FRotator(-7,40,0)); TestStage=5;
    }
    if(bVisual && TestStage==5 && TestTime>26) {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("VOID-overlook.png"),false,false); TestStage=6;
    }
    if(bVisual && TestStage==6 && TestTime>30) {
        P->Inventory(); // Input state and hover UI are exercised in the native view.
        int32 VW=0,VH=0; PC->GetViewportSize(VW,VH);
        PC->SetMouseLocation(FMath::RoundToInt(VW*.22f),FMath::RoundToInt(VH*.38f));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("VOID-inventory.png"),true,false); TestStage=7;
    }
    if(bVisual && TestStage==7 && TestTime>33) FPlatformMisc::RequestExitWithStatus(false,Failures.IsEmpty()?0:3);
}
