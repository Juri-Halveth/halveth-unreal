#include "HALVETHGameMode.h"
#include "HALVETHCharacter.h"
#include "HALVETHAdventureComponent.h"
#include "HALVETHKnowledgeComponent.h"
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

namespace
{
    int32 SavedQualityProfile(const UGameUserSettings* Settings)
    {
        if (!Settings) return 1;
        // The engine has already loaded these settings before BeginPlay. Its
        // overall-level getter also compares resolution scale, whereas our
        // profiles deliberately use their own 67 / 85 / 100 percent scales.
        const auto& Quality = Settings->ScalabilityQuality;
        const int32 Level = Quality.ViewDistanceQuality;
        const int32 Groups[] = {Quality.AntiAliasingQuality, Quality.ShadowQuality,
            Quality.GlobalIlluminationQuality, Quality.ReflectionQuality,
            Quality.PostProcessQuality, Quality.TextureQuality, Quality.EffectsQuality,
            Quality.FoliageQuality, Quality.ShadingQuality, Quality.LandscapeQuality};
        for (const int32 Group : Groups) if (Group != Level) return 1;
        if (Level == 0) return 0;
        if (Level == 3) return 2;
        return 1; // Balanced is the fallback for unknown or mixed custom settings.
    }
}

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
    bVisualTest = FParse::Param(FCommandLine::Get(), TEXT("HalvethVisual"));
    RealmWorld = GetWorld()->SpawnActor<AHALVETHRealmWorld>();
    if (!RealmWorld)
    {
        if (bSmokeTest) FinishSmoke(false, TEXT("world_actor_spawn_failed"));
        return;
    }
    RealmWorld->BuildRealm(0, static_cast<uint32>(WorldSeed));
    if (auto* Pawn = Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        Pawn->ResetToSpawn();
        if (Pawn->GetKnowledge()) Pawn->GetKnowledge()->InitializeWorld(static_cast<uint32>(WorldSeed), 0);
        if (Pawn->GetKnowledge() && FParse::Param(FCommandLine::Get(), TEXT("HalvethLibrary"))) Pawn->GetKnowledge()->ToggleReading();
        int32 InitialPanel = 0;
        if (FParse::Value(FCommandLine::Get(), TEXT("HalvethPanel="), InitialPanel)) Pawn->SetReaderPanel(InitialPanel);
    }
    if (APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0))
    {
        Player->SetInputMode(FInputModeGameOnly());
        Player->bShowMouseCursor = false;
    }
    int32 RequestedQuality = SavedQualityProfile(GEngine ? GEngine->GetGameUserSettings() : nullptr);
    FParse::Value(FCommandLine::Get(), TEXT("HalvethQuality="), RequestedQuality);
    ApplyQuality(FMath::Clamp(RequestedQuality, 0, 2), false);
    Notify(TEXT("Welcome home. B opens your living library. E gathers, speaks or enters a portal. L sends LOVE."));
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
    if (auto* Pawn = Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        Pawn->ResetToSpawn();
        if (Pawn->GetKnowledge()) Pawn->GetKnowledge()->OnRealmChanged(Destination);
    }
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
    if(bVisualTest) { VisualElapsed+=DeltaSeconds; RunVisualStep(); }
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
        if(!RealmWorld->VerifyLandscape()) { FinishSmoke(false,TEXT("landscape_geometry_or_assets_failed")); return; }
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
        if(!RealmWorld->VerifyLandscape()) { FinishSmoke(false,TEXT("destination_landscape_failed")); return; }
        // Walk the actual resource interactions across all four realms, twice per node.
        const FVector GatherPoints[] = {FVector(-320,-870,100), FVector(350,-520,100), FVector(-340,150,100), FVector(330,620,100)};
        for (int32 Local = 0; Local < 4; ++Local)
        {
            auto* Knowledge = Pawn->GetKnowledge();
            if (!Knowledge) { FinishSmoke(false, TEXT("knowledge_missing_on_portal")); return; }
            const int32 Material = (Destination * 4 + Local + static_cast<uint32>(WorldSeed) % 7u) % 7u;
            const int32 Before = Knowledge->GetMaterialCount(Material);
            Pawn->SetActorLocation(GatherPoints[Local]);
            if (!Knowledge->GatherNearby() || !Knowledge->GatherNearby() || Knowledge->GetMaterialCount(Material) < Before + 2)
            { FinishSmoke(false, TEXT("renewable_native_gather_failed")); return; }
        }
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
        Pawn->SetActorLocation(FVector(0, 0, -12500));
    }
    else if (SmokeStep == 9)
    {
        if (Pawn->GetActorLocation().Z < 0) { FinishSmoke(false, TEXT("fall_recovery_failed")); return; }
        ApplyQuality(0, false);
        const auto* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
        if (!Settings || SavedQualityProfile(Settings) != 0)
        { FinishSmoke(false, TEXT("performance_quality_restore_failed")); return; }
        auto* GI = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicGlobalIlluminationMethod"));
        if (!GI || GI->GetInt() != 0) { FinishSmoke(false, TEXT("performance_quality_not_applied")); return; }
        ApplyQuality(1, false);
        if (SavedQualityProfile(Settings) != 1)
        { FinishSmoke(false, TEXT("balanced_quality_restore_failed")); return; }
        ApplyQuality(2, false);
        if (SavedQualityProfile(Settings) != 2)
        { FinishSmoke(false, TEXT("epic_quality_restore_failed")); return; }
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
        if(!RealmWorld->VerifyCharacters()) {FinishSmoke(false,TEXT("character_skeleton_animation_morphs_or_temporal_pose_failed"));return;}
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
    }
    else if (SmokeStep == 24)
    {
        auto* Knowledge = Pawn->GetKnowledge();
        if (!Knowledge || RealmWorld->FindReadable(RealmWorld->GetReadablePosition(0)) != 0)
        { FinishSmoke(false, TEXT("physical_library_missing")); return; }
        Pawn->SetActorLocation(RealmWorld->GetReadablePosition(0) + FVector(0,0,100));
        if (Knowledge->GatherNearby()) { FinishSmoke(false, TEXT("resource_shadows_book_interaction")); return; }
        Knowledge->OpenBook(0);
        if (Knowledge->RecallPage(1) || !Knowledge->RecallPage(0) || Knowledge->RecallPage(0)
            || !Knowledge->CraftSelected() || !Knowledge->PackDraught() || Pawn->GetAdventure()->GetItemCount(0) != 3)
        { FinishSmoke(false, TEXT("read_recall_craft_pack_failed")); return; }
    }
    else if (SmokeStep == 25)
    {
        auto* Knowledge = Pawn->GetKnowledge();
        Knowledge->OpenBook(2);
        if (!Knowledge->RecallPage(1)) { FinishSmoke(false, TEXT("architecture_learning_failed")); return; }
        Knowledge->ToggleReading();
        SmokeReadingMilliseconds = Knowledge->GetActiveReadingMilliseconds();
        Pawn->SetActorLocation(FVector(0,0,100));
        Pawn->SetActorRotation(FRotator(0,90,0));
        if (Pawn->GetController()) Pawn->GetController()->SetControlRotation(FRotator(0,90,0));
        if (!Knowledge->BuildSelected() || Knowledge->GetBuiltCount() != 1 || Knowledge->GetVisibleBuiltCount() != 1)
        { FinishSmoke(false, TEXT("visible_world_construction_failed")); return; }
    }
    else if (SmokeStep == 26)
    {
        auto* Knowledge = Pawn->GetKnowledge();
        if (Knowledge->GetActiveReadingMilliseconds() != SmokeReadingMilliseconds || Knowledge->BuildSelected()
            || Knowledge->GetBuiltCount() != 1 || !Knowledge->SaveProgress() || !Travel(1))
        { FinishSmoke(false, TEXT("closed_reading_build_spacing_save_failed")); return; }
    }
    else if (SmokeStep == 27)
    {
        auto* Knowledge = Pawn->GetKnowledge();
        if (Knowledge->GetBuiltCount() != 1 || Knowledge->GetVisibleBuiltCount() != 0
            || RealmWorld->FindReadable(RealmWorld->GetReadablePosition(3)) != 3 || !Travel(0))
        { FinishSmoke(false, TEXT("construction_realm_isolation_failed")); return; }
    }
    else if (SmokeStep == 28)
    {
        auto* Knowledge = Pawn->GetKnowledge();
        if (Knowledge->GetVisibleBuiltCount() != 1 || !Knowledge->ReloadProgress() || Pawn->GetAdventure()->GetItemCount(0) != 3)
        { FinishSmoke(false, TEXT("construction_reload_failed")); return; }
        Knowledge->OpenBook(0);
        if (Knowledge->RecallPage(0)) { FinishSmoke(false, TEXT("learned_page_lost_on_reload")); return; }
        Knowledge->NextPage();
        if (!Knowledge->RecallPage(1)) { FinishSmoke(false, TEXT("ward_ink_learning_failed")); return; }
        Knowledge->CycleRecipe();
        if (!Knowledge->CraftSelected()) { FinishSmoke(false, TEXT("ward_ink_crafting_failed")); return; }
        Knowledge->OpenBook(1);
        if (!Knowledge->RecallPage(1)) { FinishSmoke(false, TEXT("lantern_first_page_failed")); return; }
        Knowledge->NextPage();
        if (!Knowledge->RecallPage(2)) { FinishSmoke(false, TEXT("lantern_second_page_failed")); return; }
        Knowledge->CycleRecipe(); Knowledge->CycleRecipe();
        if (!Knowledge->CraftSelected() || Pawn->GetAdventure()->GetMaxMana() != 110)
        { FinishSmoke(false, TEXT("enchantment_maximum_mana_failed")); return; }
    }
    else if (SmokeStep == 29)
    {
        auto* Knowledge = Pawn->GetKnowledge();
        Knowledge->OpenBook(0); Knowledge->NextPage(); Knowledge->NextPage();
        if (!Knowledge->RecallPage(2)) { FinishSmoke(false, TEXT("resin_learning_failed")); return; }
        for (int32 I=0; I<4; ++I) Knowledge->CycleRecipe(); // 3 -> 2: resin.
        if (!Knowledge->CraftSelected()) { FinishSmoke(false, TEXT("resin_crafting_failed")); return; }
        for (int32 I=0; I<3; ++I) Knowledge->CycleRecipe(); // 2 -> 0: draught kept as reagent.
        if (!Knowledge->CraftSelected()) { FinishSmoke(false, TEXT("reagent_draught_crafting_failed")); return; }
        Knowledge->OpenBook(1); Knowledge->NextPage(); Knowledge->NextPage();
        if (!Knowledge->RecallPage(0)) { FinishSmoke(false, TEXT("quiet_step_learning_failed")); return; }
        Knowledge->OpenBook(4);
        if (!Knowledge->RecallPage(2)) { FinishSmoke(false, TEXT("scribble_learning_failed")); return; }
        for (int32 I=0; I<4; ++I) Knowledge->CycleRecipe();
        if (!Knowledge->CraftSelected() || !FMath::IsNearlyEqual(Pawn->GetAdventure()->GetSelectedAbilityCost(), 14.25f))
        { FinishSmoke(false, TEXT("scribble_enchantment_spell_cost_failed")); return; }
        if (!Knowledge->ChooseQuest(0) || Knowledge->ChooseQuest(0))
        { FinishSmoke(false, TEXT("council_one_time_choice_failed")); return; }
        Knowledge->CycleQuest();
        if (!Knowledge->ChooseQuest(2)) { FinishSmoke(false, TEXT("council_archive_branch_failed")); return; }
        Knowledge->CycleQuest();
        if (!Knowledge->ChooseQuest(1) || !Knowledge->SaveProgress())
        { FinishSmoke(false, TEXT("council_construction_branch_failed")); return; }
    }
    else if (SmokeStep == 30)
    {
        auto* Knowledge = Pawn->GetKnowledge();
        Pawn->GetAdventure()->ReceiveDamage(40);
        if (!Pawn->GetAdventure()->UseItem() || Pawn->GetAdventure()->GetItemCount(0) != 2 || !Knowledge->ReloadProgress()
            || Pawn->GetAdventure()->GetItemCount(0) != 3 || Knowledge->GetBuiltCount() != 1
            || Knowledge->GetVisibleBuiltCount() != 1 || Pawn->GetAdventure()->GetMaxMana() != 110
            || !FMath::IsNearlyEqual(Pawn->GetAdventure()->GetSelectedAbilityCost(), 14.25f) || Knowledge->ChooseQuest(0))
        { FinishSmoke(false, TEXT("knowledge_save_restore_inventory_enchants_quests_failed")); return; }
        Knowledge->ToggleReading();
        FinishSmoke(true, TEXT("portals_love_collision_recovery_quality_inventory_shield_npc_dodge_projectiles_crystal_library_recall_crafting_enchantments_scribble_council_construction_realm_persistence_save_restore"));
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
