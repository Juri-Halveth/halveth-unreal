#include "HALVETHAdventureComponent.h"
#include "HALVETHKnowledgeComponent.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    constexpr float AbilityCosts[3] = {20, 15, 30};
    const FLinearColor LoveColor(1.0f, 0.04f, 0.24f);
    const FLinearColor SparkColor(1.0f, 0.52f, 0.07f);
    const FLinearColor ShieldColor(0.05f, 0.6f, 1.0f);

    UMaterialInstanceDynamic* MakeSpellMaterial(UStaticMeshComponent* Mesh, FLinearColor Color)
    {
        UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Materials/M_HalvethSurface.M_HalvethSurface"));
        if (!Base) Base = UMaterial::GetDefaultMaterial(MD_Surface);
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Mesh);
        Material->SetVectorParameterValue(TEXT("Tint"), Color);
        Material->SetScalarParameterValue(TEXT("Glow"), 5);
        Mesh->SetMaterial(0, Material);
        return Material;
    }
}

UHALVETHAdventureComponent::UHALVETHAdventureComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UHALVETHAdventureComponent::BeginPlay()
{
    Super::BeginPlay();
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->GetRootComponent()) return;
    Owner->OnTakeAnyDamage.AddDynamic(this, &UHALVETHAdventureComponent::OnOwnerDamaged);
    SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Aura = NewObject<UStaticMeshComponent>(Owner);
    Owner->AddInstanceComponent(Aura);
    Aura->SetupAttachment(Owner->GetRootComponent());
    Aura->SetStaticMesh(SphereMesh);
    Aura->SetRelativeLocation(FVector(0, 0, -79));
    Aura->SetRelativeScale3D(FVector(2.2f, 2.2f, 0.025f));
    Aura->SetMobility(EComponentMobility::Movable);
    Aura->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Aura->SetCastShadow(false);
    AuraMaterial = MakeSpellMaterial(Aura, LoveColor);
    Aura->SetVisibility(false);
    Aura->RegisterComponent();
    AuraLight = NewObject<UPointLightComponent>(Owner);
    Owner->AddInstanceComponent(AuraLight);
    AuraLight->SetupAttachment(Owner->GetRootComponent());
    AuraLight->SetRelativeLocation(FVector(0, 0, 45));
    AuraLight->SetMobility(EComponentMobility::Movable);
    AuraLight->SetAttenuationRadius(700);
    AuraLight->SetCastShadows(false);
    AuraLight->SetIntensity(0);
    AuraLight->RegisterComponent();
}

void UHALVETHAdventureComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (AActor* Owner = GetOwner())
        Owner->OnTakeAnyDamage.RemoveDynamic(this, &UHALVETHAdventureComponent::OnOwnerDamaged);
    ClearTransientEffects();
    if (Aura) Aura->DestroyComponent();
    if (AuraLight) AuraLight->DestroyComponent();
    Super::EndPlay(Reason);
}

void UHALVETHAdventureComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0) return;
    Elapsed += DeltaTime;
    Mana = FMath::Min(GetMaxMana(), Mana + DeltaTime * 8);
    Stamina = FMath::Min(100.0f, Stamina + DeltaTime * 20);
    AbilityCooldown = FMath::Max(0.0f, AbilityCooldown - DeltaTime);
    DodgeCooldown = FMath::Max(0.0f, DodgeCooldown - DeltaTime);
    ShieldRemaining = FMath::Max(0.0f, ShieldRemaining - DeltaTime);
    LoveRemaining = FMath::Max(0.0f, LoveRemaining - DeltaTime);
    const bool bLit = LoveRemaining > 0 || IsShieldActive();
    const FLinearColor Color = LoveRemaining > 0 ? LoveColor : ShieldColor;
    if (Aura)
    {
        Aura->SetVisibility(bLit);
        const float Radius = 2.1f + FMath::Sin(Elapsed * 5) * 0.15f;
        Aura->SetRelativeScale3D(FVector(Radius, Radius, 0.025f));
    }
    if (AuraMaterial) AuraMaterial->SetVectorParameterValue(TEXT("Tint"), Color);
    if (AuraLight)
    {
        AuraLight->SetLightColor(Color);
        AuraLight->SetIntensity(bLit ? (LoveRemaining > 0 ? 4500 : 1800) : 0);
    }
    UpdateSparks(DeltaTime);
}

FString UHALVETHAdventureComponent::AbilityName(int32 Index)
{
    switch (Index)
    {
        case 1: return TEXT("SPARK");
        case 2: return TEXT("AEGIS SHIELD");
        default: return TEXT("LOVE");
    }
}

FString UHALVETHAdventureComponent::ItemName(int32 Index)
{
    switch (Index)
    {
        case 1: return TEXT("Moonwater");
        case 2: return TEXT("Sunfruit");
        default: return TEXT("Lumen Draught");
    }
}

void UHALVETHAdventureComponent::SetFeedback(const FString& Message)
{
    InteractionText = Message;
    UE_LOG(LogTemp, Display, TEXT("HALVETH_ADVENTURE %s"), *Message);
}

void UHALVETHAdventureComponent::CycleAbility()
{
    SelectedAbility = (SelectedAbility + 1) % 3;
    SetFeedback(GetAbilityText());
}

bool UHALVETHAdventureComponent::ActivateAbility()
{
    if (!GetOwner() || !GetWorld()) return false;
    if (AbilityCooldown > 0)
    {
        SetFeedback(TEXT("Spell is recovering. Try again in a moment."));
        return false;
    }
    const float Cost = GetSelectedAbilityCost();
    if (Mana < Cost)
    {
        SetFeedback(FString::Printf(TEXT("%s needs %.0f mana. Mana returns over time; Moonwater restores 45."),
            *AbilityName(SelectedAbility), Cost));
        return false;
    }
    if (SelectedAbility == 2 && IsShieldActive())
    {
        SetFeedback(FString::Printf(TEXT("Aegis already protects you for %.1f seconds."), ShieldRemaining));
        return false;
    }
    if (SelectedAbility == 1 && !LaunchSpark()) return false;
    Mana -= Cost;
    AbilityCooldown = 0.55f;
    if (SelectedAbility == 0)
    {
        const float Before = Health;
        Health = FMath::Min(100.0f, Health + 35);
        LoveRemaining = 4;
        SetFeedback(FString::Printf(TEXT("LOVE: +%.0f health, warm light for 4s. Cost %.1f mana."), Health - Before, Cost));
    }
    else if (SelectedAbility == 1)
        SetFeedback(FString::Printf(TEXT("SPARK: a travelling light bolt, 25 impact damage. Cost %.1f mana."), Cost));
    else
    {
        ShieldRemaining = 6;
        SetFeedback(FString::Printf(TEXT("AEGIS: 75%% less incoming damage for 6s. Cost %.1f mana."), Cost));
    }
    return true;
}

void UHALVETHAdventureComponent::CycleItem()
{
    SelectedItem = (SelectedItem + 1) % 3;
    SetFeedback(GetInventoryText());
}

int32 UHALVETHAdventureComponent::GetItemCount(int32 Index) const
{
    return Index >= 0 && Index < 3 ? ItemCounts[Index] : 0;
}

bool UHALVETHAdventureComponent::GrantConsumable(int32 Index, int32 Amount)
{
    if (Index < 0 || Index >= 3 || Amount <= 0 || Amount > 99 || ItemCounts[Index] > 99 - Amount) return false;
    ItemCounts[Index] += Amount;
    SetFeedback(FString::Printf(TEXT("Crafted %s x%d. Stored in your inventory."), *ItemName(Index), Amount));
    return true;
}

bool UHALVETHAdventureComponent::RestoreConsumables(int32 HealthItems, int32 ManaItems, int32 StaminaItems)
{
    if (HealthItems < 0 || HealthItems > 99 || ManaItems < 0 || ManaItems > 99 || StaminaItems < 0 || StaminaItems > 99) return false;
    ItemCounts[0] = HealthItems; ItemCounts[1] = ManaItems; ItemCounts[2] = StaminaItems;
    return true;
}

float UHALVETHAdventureComponent::GetMaxMana() const
{
    const auto* Knowledge = GetOwner() ? GetOwner()->FindComponentByClass<UHALVETHKnowledgeComponent>() : nullptr;
    return 100.0f + (Knowledge ? FMath::Clamp(Knowledge->GetMaxManaBonus(), 0.0f, 100.0f) : 0.0f);
}

float UHALVETHAdventureComponent::GetSelectedAbilityCost() const
{
    const auto* Knowledge = GetOwner() ? GetOwner()->FindComponentByClass<UHALVETHKnowledgeComponent>() : nullptr;
    return AbilityCosts[SelectedAbility] * (Knowledge ? FMath::Clamp(Knowledge->GetSpellCostMultiplier(), 0.25f, 1.0f) : 1.0f);
}

bool UHALVETHAdventureComponent::UseItem()
{
    if (ItemCounts[SelectedItem] <= 0)
    {
        SetFeedback(TEXT("That item is empty. Select another item."));
        return false;
    }
    float& Value = SelectedItem == 0 ? Health : (SelectedItem == 1 ? Mana : Stamina);
    const float Maximum = SelectedItem == 1 ? GetMaxMana() : 100.0f;
    if (Value >= Maximum)
    {
        SetFeedback(TEXT("Already full; your item is kept."));
        return false;
    }
    const float Before = Value;
    Value = FMath::Min(Maximum, Value + (SelectedItem == 0 ? 40.0f : 45.0f));
    --ItemCounts[SelectedItem];
    SetFeedback(FString::Printf(TEXT("Used %s: +%.0f %s. %d left."), *ItemName(SelectedItem), Value - Before,
        SelectedItem == 0 ? TEXT("health") : (SelectedItem == 1 ? TEXT("mana") : TEXT("stamina")),
        ItemCounts[SelectedItem]));
    return true;
}

bool UHALVETHAdventureComponent::Dodge()
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character || !Character->GetCharacterMovement()) return false;
    if (DodgeCooldown > 0 || Character->GetCharacterMovement()->IsFalling())
    {
        SetFeedback(TEXT("Dodge needs solid ground and a 0.9s recovery."));
        return false;
    }
    if (Stamina < 28)
    {
        SetFeedback(TEXT("Dodge needs 28 stamina. Wait or use Sunfruit."));
        return false;
    }
    FVector Direction = Character->GetLastMovementInputVector();
    Direction.Z = 0;
    if (!Direction.Normalize()) Direction = Character->GetActorForwardVector().GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return false;
    Character->LaunchCharacter(Direction * 950 + FVector(0, 0, 100), true, false);
    Stamina -= 28;
    DodgeCooldown = 0.9f;
    SetFeedback(TEXT("DODGE: quick movement in your input direction. Cost 28 stamina; no invulnerability."));
    return true;
}

float UHALVETHAdventureComponent::ReceiveDamage(float RawDamage)
{
    if (!FMath::IsFinite(RawDamage) || RawDamage <= 0) return 0;
    const float Effective = RawDamage * (IsShieldActive() ? 0.25f : 1.0f);
    const float Before = Health;
    Health = FMath::Max(0.0f, Health - Effective);
    SetFeedback(FString::Printf(TEXT("%s%.0f damage. Health %.0f/100.%s"),
        IsShieldActive() ? TEXT("Aegis softened the hit: ") : TEXT(""), Before - Health, Health,
        Health <= 0 ? TEXT(" Use LOVE or a Lumen Draught to recover; death is not simulated in this prototype.") : TEXT("")));
    return Before - Health;
}

void UHALVETHAdventureComponent::OnOwnerDamaged(AActor*, float Damage, const UDamageType*, AController*, AActor*)
{
    ReceiveDamage(Damage);
}

bool UHALVETHAdventureComponent::LaunchSpark()
{
    AActor* Owner = GetOwner();
    if (!Owner || !GetWorld() || !SphereMesh) return false;
    if (Sparks.Num() >= 8)
    {
        SetFeedback(TEXT("Let the current sparks fade before casting again."));
        return false;
    }
    FVector ViewLocation;
    FRotator ViewRotation;
    Owner->GetActorEyesViewPoint(ViewLocation, ViewRotation);
    if (const ACharacter* Character = Cast<ACharacter>(Owner))
        if (const APlayerController* Player = Cast<APlayerController>(Character->GetController()))
            Player->GetPlayerViewPoint(ViewLocation, ViewRotation);
    FHALVETHSparkFlight Flight;
    // Start close to the eye and sweep from there; do not skip nearby walls.
    Flight.Position = ViewLocation + ViewRotation.Vector() * 8;
    Flight.Velocity = ViewRotation.Vector() * 1800;
    Flight.Remaining = 2;
    Flight.Mesh = NewObject<UStaticMeshComponent>(Owner);
    Owner->AddInstanceComponent(Flight.Mesh);
    Flight.Mesh->SetStaticMesh(SphereMesh);
    Flight.Mesh->SetMobility(EComponentMobility::Movable);
    Flight.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Flight.Mesh->SetCastShadow(false);
    Flight.Mesh->SetWorldScale3D(FVector(0.16f));
    Flight.Mesh->SetWorldLocation(Flight.Position);
    MakeSpellMaterial(Flight.Mesh, SparkColor);
    Flight.Mesh->RegisterComponent();
    Flight.Light = NewObject<UPointLightComponent>(Owner);
    Owner->AddInstanceComponent(Flight.Light);
    Flight.Light->SetMobility(EComponentMobility::Movable);
    Flight.Light->SetLightColor(SparkColor);
    Flight.Light->SetIntensity(1800);
    Flight.Light->SetAttenuationRadius(250);
    Flight.Light->SetCastShadows(false);
    Flight.Light->SetWorldLocation(Flight.Position);
    Flight.Light->RegisterComponent();
    Sparks.Add(Flight);
    return true;
}

void UHALVETHAdventureComponent::UpdateSparks(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World) return;
    for (int32 Index = Sparks.Num() - 1; Index >= 0; --Index)
    {
        FHALVETHSparkFlight& Flight = Sparks[Index];
        Flight.Remaining -= DeltaTime;
        if (Flight.Remaining <= 0)
        {
            if (Flight.Mesh) Flight.Mesh->DestroyComponent();
            if (Flight.Light) Flight.Light->DestroyComponent();
            Sparks.RemoveAtSwap(Index);
            continue;
        }
        if (!Flight.bImpact)
        {
            const FVector Next = Flight.Position + Flight.Velocity * DeltaTime;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(HALVETHSpark), false, GetOwner());
            FHitResult Hit;
            if (World->SweepSingleByChannel(Hit, Flight.Position, Next, FQuat::Identity,
                ECC_Visibility, FCollisionShape::MakeSphere(8), Query))
            {
                Flight.Position = Hit.ImpactPoint + Hit.ImpactNormal * 10;
                Flight.bImpact = true;
                Flight.Remaining = 0.35f;
                ++SparkImpacts;
                if (AActor* Target = Hit.GetActor())
                {
                    ACharacter* Character = Cast<ACharacter>(GetOwner());
                    UGameplayStatics::ApplyPointDamage(Target, 25, Flight.Velocity.GetSafeNormal(), Hit,
                        Character ? Character->GetController() : nullptr, GetOwner(), UDamageType::StaticClass());
                }
                if (UPrimitiveComponent* TargetComponent = Hit.GetComponent())
                    if (TargetComponent->IsSimulatingPhysics())
                        TargetComponent->AddImpulseAtLocation(Flight.Velocity.GetSafeNormal() * 25000, Hit.ImpactPoint);
                SetFeedback(TEXT("SPARK struck a surface. Impact light and damage event delivered; static scenery remains intact."));
                UE_LOG(LogTemp, Display, TEXT("HALVETH_SPARK_IMPACT count=%d"), SparkImpacts);
            }
            else Flight.Position = Next;
        }
        if (Flight.Mesh)
        {
            Flight.Mesh->SetWorldLocation(Flight.Position);
            if (Flight.bImpact) Flight.Mesh->SetWorldScale3D(FVector(0.3f + (0.35f - Flight.Remaining) * 2));
        }
        if (Flight.Light)
        {
            Flight.Light->SetWorldLocation(Flight.Position);
            Flight.Light->SetIntensity(Flight.bImpact ? Flight.Remaining * 12000 : 1800);
        }
    }
}

void UHALVETHAdventureComponent::ClearTransientEffects()
{
    for (FHALVETHSparkFlight& Flight : Sparks)
    {
        if (Flight.Mesh) Flight.Mesh->DestroyComponent();
        if (Flight.Light) Flight.Light->DestroyComponent();
    }
    Sparks.Empty();
    ShieldRemaining = 0;
    LoveRemaining = 0;
    if (Aura) Aura->SetVisibility(false);
    if (AuraLight) AuraLight->SetIntensity(0);
}

FName UHALVETHAdventureComponent::FindDialogueTag(const TArray<FName>& Tags)
{
    for (const FName Tag : {FName(TEXT("HALVETH_NPC_SCARLET")), FName(TEXT("HALVETH_NPC_LUCINET")), FName(TEXT("HALVETH_NPC_RACHEL"))})
        if (Tags.Contains(Tag)) return Tag;
    return NAME_None;
}

FString UHALVETHAdventureComponent::AuthoredDialogue(FName Identity, int32 Step)
{
    const bool bFirst = Step % 2 == 0;
    if (Identity == TEXT("HALVETH_NPC_SCARLET"))
        return bFirst ? TEXT("SCARLET: Welcome, traveller. LOVE heals and lights the garden. We grow it through acts of care.")
            : TEXT("SCARLET: Take the crimson portal to my garden. Practise light before force; your choices can write its next chapter.");
    if (Identity == TEXT("HALVETH_NPC_LUCINET"))
        return bFirst ? TEXT("LUCINET: Every portal keeps a path home. The blue tide holds questions, not a promised destiny.")
            : TEXT("LUCINET: SPARK travels until it meets the world. Observe its impact. A map is useful only when you can test its paths.");
    return bFirst ? TEXT("RACHEL: A small beginning is still a beginning. Your pack has Lumen Draught, Moonwater and Sunfruit.")
        : TEXT("RACHEL: Lumen Draught restores health, Moonwater mana, Sunfruit stamina. Items stay in your pack when that resource is full.");
}

bool UHALVETHAdventureComponent::InteractWithNearbyCharacter()
{
    if (!GetOwner() || !GetWorld()) return false;
    FName Nearest = NAME_None;
    float Best = FMath::Square(350.0f);
    const FVector PlayerPosition = GetOwner()->GetActorLocation();
    auto Consider = [&](FName Tag, const FVector& Position)
    {
        const float Distance = FVector::DistSquared(PlayerPosition, Position);
        if (!Tag.IsNone() && Distance < Best) { Best = Distance; Nearest = Tag; }
    };
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (*It == GetOwner() || It->IsHidden()) continue;
        Consider(FindDialogueTag(It->Tags), It->GetActorLocation());
        TInlineComponentArray<USceneComponent*> Components;
        It->GetComponents(Components);
        for (USceneComponent* Component : Components)
            if (Component && Component->IsVisible())
                Consider(FindDialogueTag(Component->ComponentTags), Component->GetComponentLocation());
    }
    if (Nearest.IsNone()) return false;
    int32& Step = ConversationSteps.FindOrAdd(Nearest);
    SetFeedback(AuthoredDialogue(Nearest, Step++) + TEXT(" [Authored dialogue]"));
    return true;
}

FString UHALVETHAdventureComponent::GetStatusText() const
{
    return FString::Printf(TEXT("HEALTH %.0f/100   MANA %.0f/%.0f (+8/s)   STAMINA %.0f/100 (+20/s)%s"),
        Health, Mana, GetMaxMana(), Stamina, IsShieldActive() ? *FString::Printf(TEXT("   SHIELD %.1fs"), ShieldRemaining) : TEXT(""));
}

FString UHALVETHAdventureComponent::GetAbilityText() const
{
    static const TCHAR* Descriptions[] = {
        TEXT("Heal 35 + light 4s"), TEXT("Travelling bolt | 25 impact damage"),
        TEXT("75% damage reduction for 6s")};
    return FString::Printf(TEXT("%s [%d/3] - %s | %.1f mana"), *AbilityName(SelectedAbility), SelectedAbility + 1, Descriptions[SelectedAbility], GetSelectedAbilityCost());
}

FString UHALVETHAdventureComponent::GetInventoryText() const
{
    static const TCHAR* Benefits[] = {TEXT("+40 health"), TEXT("+45 mana"), TEXT("+45 stamina")};
    return FString::Printf(TEXT("%s x%d [%d/3] - %s | Pack: Lumen Draught %d, Moonwater %d, Sunfruit %d"),
        *ItemName(SelectedItem), ItemCounts[SelectedItem], SelectedItem + 1, Benefits[SelectedItem],
        ItemCounts[0], ItemCounts[1], ItemCounts[2]);
}
