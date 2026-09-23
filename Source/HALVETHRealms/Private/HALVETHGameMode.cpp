#include "HALVETHGameMode.h"
#include "HALVETHCharacter.h"
#include "HALVETHAdventureComponent.h"
#include "HALVETHTrainingTarget.h"
#include "HALVETHHUD.h"
#include "HALVETHRealmWorld.h"
#include "RealmLayout.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"

AHALVETHGameMode::AHALVETHGameMode()
{
    DefaultPawnClass = AHALVETHCharacter::StaticClass();
    HUDClass = AHALVETHHUD::StaticClass();
    PrimaryActorTick.bCanEverTick = true;
}

void AHALVETHGameMode::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(), TEXT("HalvethSeed="), WorldSeed);
    bSmokeTest = FParse::Param(FCommandLine::Get(), TEXT("HalvethSmokeTest"));
    RealmWorld = GetWorld()->SpawnActor<AHALVETHRealmWorld>();
    if (!RealmWorld)
    {
        if (bSmokeTest) FinishSmoke(false, TEXT("world_actor_spawn_failed"));
        return;
    }
    RealmWorld->BuildRealm(0, static_cast<uint32>(WorldSeed));
    if (auto* Pawn = Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this, 0))) Pawn->ResetToSpawn();
    if (APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0))
    {
        Player->SetInputMode(FInputModeGameOnly());
        Player->bShowMouseCursor = false;
    }
    int32 RequestedQuality = 2;
    FParse::Value(FCommandLine::Get(), TEXT("HalvethQuality="), RequestedQuality);
    ApplyQuality(FMath::Clamp(RequestedQuality, 0, 2), false);
    Notify(TEXT("Welcome home. Walk to a portal and press E. L sends LOVE."));
}

void AHALVETHGameMode::Notify(const FString& Text)
{
    Message = Text;
    MessageRemaining = 5;
}

bool AHALVETHGameMode::Travel(int32 Destination)
{
    if (!RealmWorld || TravelCooldown > 0 || !HalvethLayout::CanTravel(RealmWorld->GetCurrentRealm(), Destination)) return false;
    if (auto* Pawn = Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
        if (Pawn->GetAdventure()) Pawn->GetAdventure()->ClearTransientEffects();
    RealmWorld->BuildRealm(Destination, static_cast<uint32>(WorldSeed));
    TravelCooldown = 0.65f;
    if (auto* Pawn = Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this, 0))) Pawn->ResetToSpawn();
    if (APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0))
        if (Player->PlayerCameraManager) Player->PlayerCameraManager->StartCameraFade(1, 0, 0.65f, FLinearColor(0.08f, 0.015f, 0.045f));
    Notify(AHALVETHRealmWorld::RealmDescription(Destination));
    return true;
}

void AHALVETHGameMode::Interact(APawn* Pawn)
{
    if (!RealmWorld || !Pawn) return;
    const int32 Destination = RealmWorld->FindPortal(Pawn->GetActorLocation());
    if (Destination != INDEX_NONE) Travel(Destination);
    else Notify(TEXT("Move closer to a luminous portal, then press E."));
}

void AHALVETHGameMode::CastLove()
{
    if (RealmWorld)
    {
        RealmWorld->BeginLovePulse();
        Notify(TEXT("LOVE - a light for every path."));
    }
}

void AHALVETHGameMode::ApplyQuality(int32 Quality, bool Persist)
{
    QualityIndex = FMath::Clamp(Quality, 0, 2);
    UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
    if (!Settings) return;
    Settings->SetOverallScalabilityLevel(QualityIndex == 0 ? 0 : QualityIndex + 1);
    Settings->SetResolutionScaleValueEx(QualityIndex == 0 ? 67 : QualityIndex == 1 ? 85 : 100);
    Settings->SetFrameRateLimit(60);
    Settings->ApplyNonResolutionSettings();
    if (Persist) Settings->SaveSettings();
    auto Set = [](const TCHAR* Name, int32 Value)
    {
        if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name)) Variable->Set(Value, ECVF_SetByCode);
    };
    Set(TEXT("r.DynamicGlobalIlluminationMethod"), QualityIndex == 0 ? 0 : 1);
    Set(TEXT("r.ReflectionMethod"), QualityIndex == 0 ? 0 : 1);
    Set(TEXT("r.Shadow.Virtual.Enable"), QualityIndex == 0 ? 0 : 1);
    Set(TEXT("r.VolumetricFog"), QualityIndex == 2 ? 1 : 0);
    Notify(QualityIndex == 0 ? TEXT("Graphics: Performance") : QualityIndex == 1 ? TEXT("Graphics: Balanced") : TEXT("Graphics: Epic"));
}

void AHALVETHGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    MessageRemaining = FMath::Max(0.0f, MessageRemaining - DeltaSeconds);
    TravelCooldown = FMath::Max(0.0f, TravelCooldown - DeltaSeconds);
    if (bSmokeTest && !bSmokeFailed)
    {
        SmokeElapsed += DeltaSeconds;
        if (SmokeElapsed >= 1.2f)
        {
            SmokeElapsed = 0;
            RunSmokeStep();
        }
    }
}

void AHALVETHGameMode::RunSmokeStep()
{
    auto* Pawn = Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!Pawn || !RealmWorld) { FinishSmoke(false, TEXT("pawn_or_world_missing")); return; }
    if (SmokeStep == 0)
    {
        if (!LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_HalvethSurface.M_HalvethSurface")))
        { FinishSmoke(false, TEXT("generated_material_missing")); return; }
        if (RealmWorld->GetPortalCount() != 3 || RealmWorld->GetCurrentRealm() != 0)
        { FinishSmoke(false, TEXT("hub_portal_count")); return; }
        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(Pawn);
        if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(0, -1050, 500), FVector(0, -1050, -500), ECC_Visibility, Params))
        { FinishSmoke(false, TEXT("floor_collision_missing")); return; }
    }
    else if (SmokeStep <= 6)
    {
        const int32 Destination = SmokeStep % 2 == 1 ? (SmokeStep + 1) / 2 : 0;
        Pawn->SetActorLocation(RealmWorld->GetPortalPosition(Destination) + FVector(0, -160, 115), false, nullptr, ETeleportType::TeleportPhysics);
        Interact(Pawn);
        if (RealmWorld->GetCurrentRealm() != Destination || RealmWorld->GetPortalCount() != (Destination == 0 ? 3 : 1))
        { FinishSmoke(false, TEXT("portal_roundtrip_failed")); return; }
    }
    else if (SmokeStep == 7)
    {
        CastLove();
        if (RealmWorld->GetLoveStrength() < 0.99f) { FinishSmoke(false, TEXT("love_pulse_failed")); return; }
    }
    else if (SmokeStep == 8)
    {
        if (Travel(-1) || Travel(99) || RealmWorld->GetCurrentRealm() != 0)
        { FinishSmoke(false, TEXT("invalid_destination_accepted")); return; }
        Pawn->SetActorLocation(FVector(0, 0, -900));
    }
    else if (SmokeStep == 9)
    {
        if (Pawn->GetActorLocation().Z < 0) { FinishSmoke(false, TEXT("fall_recovery_failed")); return; }
        ApplyQuality(0, false);
        auto* GI = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicGlobalIlluminationMethod"));
        if (!GI || GI->GetInt() != 0) { FinishSmoke(false, TEXT("performance_quality_not_applied")); return; }
        ApplyQuality(1, false); ApplyQuality(2, false);
        if (GI->GetInt() != 1) { FinishSmoke(false, TEXT("epic_quality_not_applied")); return; }
    }
    else if (SmokeStep == 10)
    {
        auto* Adventure = Pawn->GetAdventure();
        if (!Adventure) { FinishSmoke(false, TEXT("adventure_missing")); return; }
        Adventure->ReceiveDamage(40);
        if (Adventure->GetHealth() != 60 || !Adventure->UseItem() || Adventure->GetHealth() != 100
            || Adventure->GetItemCount(0) != 2 || Adventure->UseItem() || Adventure->GetItemCount(0) != 2)
        { FinishSmoke(false, TEXT("inventory_health_consumption")); return; }
        Adventure->CycleAbility(); Adventure->CycleAbility();
        if (!Adventure->ActivateAbility() || !Adventure->IsShieldActive() || Adventure->ReceiveDamage(40) != 10)
        { FinishSmoke(false, TEXT("shield_damage_reduction")); return; }
    }
    else if (SmokeStep == 11)
    {
        auto* Adventure = Pawn->GetAdventure();
        Adventure->CycleAbility();
        if (!Adventure->ActivateAbility() || Adventure->GetHealth() != 100)
        { FinishSmoke(false, TEXT("love_health_recovery")); return; }
    }
    else if (SmokeStep == 12)
    {
        Pawn->SetActorLocation(FVector(-380, -700, 100));
        if (!Pawn->GetAdventure()->InteractWithNearbyCharacter() || !Pawn->GetAdventure()->Dodge()
            || Pawn->GetAdventure()->GetStamina() > 73)
        { FinishSmoke(false, TEXT("npc_dialogue_or_dodge")); return; }
    }
    else if (SmokeStep == 13)
    {
        if (FVector::Dist2D(Pawn->GetActorLocation(), FVector(-380, -700, 100)) < 20)
        { FinishSmoke(false, TEXT("dodge_movement_missing")); return; }
        auto* Target = Cast<AHALVETHTrainingTarget>(UGameplayStatics::GetActorOfClass(this, AHALVETHTrainingTarget::StaticClass()));
        if (!Target) { FinishSmoke(false, TEXT("training_target_missing")); return; }
        Target->ResetTarget();
        Pawn->ResetToSpawn();
        Pawn->SetActorLocation(FVector(430, -940, 100));
        const FRotator Aim = (Target->GetActorLocation() + FVector(0, 0, 10) - (Pawn->GetActorLocation() + FVector(0, 0, 66))).Rotation();
        if (Pawn->GetController()) Pawn->GetController()->SetControlRotation(Aim);
        Pawn->GetAdventure()->CycleAbility();
    }
    else if (SmokeStep >= 14 && SmokeStep <= 18)
    {
        auto* Target = Cast<AHALVETHTrainingTarget>(UGameplayStatics::GetActorOfClass(this, AHALVETHTrainingTarget::StaticClass()));
        if (!Target || Target->GetHealth() != FMath::Max(0, 100 - (SmokeStep - 14) * 25))
        { FinishSmoke(false, TEXT("spark_did_not_damage_crystal")); return; }
        if (SmokeStep < 18 && !Pawn->GetAdventure()->ActivateAbility())
        { FinishSmoke(false, TEXT("spark_cast_failed")); return; }
        if (SmokeStep == 18 && (!Target->IsBroken() || Target->GetBreakCount() != 1))
        { FinishSmoke(false, TEXT("crystal_break_missing")); return; }
    }
    else if (SmokeStep == 23)
    {
        auto* Target = Cast<AHALVETHTrainingTarget>(UGameplayStatics::GetActorOfClass(this, AHALVETHTrainingTarget::StaticClass()));
        if (!Target || Target->IsBroken() || Target->GetHealth() != 100)
        { FinishSmoke(false, TEXT("crystal_regeneration_missing")); return; }
        FinishSmoke(true, TEXT("portals_love_collision_recovery_quality_inventory_shield_npc_dodge_four_projectile_hits_crystal_regeneration"));
        return;
    }
    ++SmokeStep;
}

void AHALVETHGameMode::FinishSmoke(bool Success, const FString& Detail)
{
    bSmokeTest = false;
    bSmokeFailed = !Success;
    const FString Result = FString::Printf(TEXT("{\"schema\":1,\"passed\":%s,\"seed\":%d,\"detail\":\"%s\",\"engine\":\"%s\",\"renderingVerified\":false}\n"),
        Success ? TEXT("true") : TEXT("false"), WorldSeed, *Detail, *FEngineVersion::Current().ToString());
    FFileHelper::SaveStringToFile(Result, *(FPaths::ProjectSavedDir() / TEXT("HALVETH-runtime-smoke.json")));
    UE_LOG(LogTemp, Display, TEXT("HALVETH_RUNTIME_SMOKE_%s %s"), Success ? TEXT("PASS") : TEXT("FAIL"), *Detail);
    FPlatformMisc::RequestExitWithStatus(false, Success ? 0 : 1);
}
