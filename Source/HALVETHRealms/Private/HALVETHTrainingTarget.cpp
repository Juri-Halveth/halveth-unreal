#include "HALVETHTrainingTarget.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AHALVETHTrainingTarget::AHALVETHTrainingTarget()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Tags.Add(TEXT("HALVETH_TRAINING_TARGET"));
    SetCanBeDamaged(true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Crystal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crystal"));
    Crystal->SetupAttachment(RootComponent);
    Crystal->SetStaticMesh(Cone.Object);
    Crystal->SetRelativeLocation(FVector(0, 0, 28));
    Crystal->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.0f));
    Crystal->SetMobility(EComponentMobility::Movable);
    Crystal->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Crystal->SetCollisionResponseToAllChannels(ECR_Ignore);
    Crystal->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Crystal->SetGenerateOverlapEvents(false);
    LowerCrystal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LowerCrystal"));
    LowerCrystal->SetupAttachment(RootComponent);
    LowerCrystal->SetStaticMesh(Cone.Object);
    LowerCrystal->SetRelativeLocation(FVector(0, 0, -22));
    LowerCrystal->SetRelativeRotation(FRotator(180, 0, 0));
    LowerCrystal->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.65f));
    LowerCrystal->SetMobility(EComponentMobility::Movable);
    LowerCrystal->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    LowerCrystal->SetCollisionResponseToAllChannels(ECR_Ignore);
    LowerCrystal->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    LowerCrystal->SetGenerateOverlapEvents(false);
    for (int32 Index = 0; Index < 6; ++Index)
    {
        UStaticMeshComponent* Fragment = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("Fragment%d"), Index));
        Fragment->SetupAttachment(RootComponent);
        Fragment->SetStaticMesh(Sphere.Object);
        Fragment->SetRelativeScale3D(FVector(0.13f));
        Fragment->SetMobility(EComponentMobility::Movable);
        Fragment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Fragment->SetCastShadow(false);
        Fragment->SetVisibility(false);
        Fragments.Add(Fragment);
    }
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(0, 0, 122));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(19);
    Label->SetTextRenderColor(FColor(255, 216, 142));
    Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
    Glow->SetupAttachment(RootComponent);
    Glow->SetAttenuationRadius(350);
    Glow->SetIntensity(1300);
    Glow->SetLightColor(FLinearColor(0.04f, 0.7f, 1));
    Glow->SetCastShadows(false);
    Glow->SetMobility(EComponentMobility::Movable);
}

void AHALVETHTrainingTarget::BeginPlay()
{
    Super::BeginPlay();
    Home = GetActorLocation();
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Materials/M_HalvethSurface.M_HalvethSurface"));
    if (!Base) Base = UMaterial::GetDefaultMaterial(MD_Surface);
    Material = UMaterialInstanceDynamic::Create(Base, this);
    Crystal->SetMaterial(0, Material);
    LowerCrystal->SetMaterial(0, Material);
    for (UStaticMeshComponent* Fragment : Fragments) Fragment->SetMaterial(0, Material);
    UpdateAppearance();
}

float AHALVETHTrainingTarget::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (IsBroken() || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0) return 0;
    const float Applied = FMath::Min(Health, DamageAmount);
    Health -= Applied;
    HitFlash = 0.25f;
    if (Health <= 0)
    {
        ++BreakCount;
        RespawnRemaining = 5;
        Crystal->SetVisibility(false);
        LowerCrystal->SetVisibility(false);
        Crystal->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        LowerCrystal->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        for (UStaticMeshComponent* Fragment : Fragments) Fragment->SetVisibility(true);
        UE_LOG(LogTemp, Display, TEXT("HALVETH_TRAINING_BREAK count=%d respawn=5"), BreakCount);
    }
    UpdateAppearance();
    Super::TakeDamage(Applied, DamageEvent, EventInstigator, DamageCauser);
    UE_LOG(LogTemp, Display, TEXT("HALVETH_TRAINING_HIT damage=%.1f health=%.1f"), Applied, Health);
    return Applied;
}

void AHALVETHTrainingTarget::ResetTarget()
{
    Health = 100;
    RespawnRemaining = 0;
    HitFlash = 0;
    Crystal->SetVisibility(true);
    LowerCrystal->SetVisibility(true);
    Crystal->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    LowerCrystal->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    for (UStaticMeshComponent* Fragment : Fragments) Fragment->SetVisibility(false);
    UpdateAppearance();
    UE_LOG(LogTemp, Display, TEXT("HALVETH_TRAINING_RESET health=100 breaks=%d"), BreakCount);
}

void AHALVETHTrainingTarget::UpdateAppearance()
{
    const float Ratio = Health / 100;
    const FLinearColor Color = HitFlash > 0 ? FLinearColor(1, 0.9f, 0.55f)
        : FMath::Lerp(FLinearColor(1, 0.08f, 0.16f), FLinearColor(0.04f, 0.7f, 1), Ratio);
    if (Material)
    {
        Material->SetVectorParameterValue(TEXT("Tint"), Color);
        Material->SetScalarParameterValue(TEXT("Glow"), HitFlash > 0 ? 9 : 2.2f);
    }
    Glow->SetLightColor(Color);
    Glow->SetIntensity(IsBroken() ? 450 : (HitFlash > 0 ? 3200 : 1300));
    const float Width = 0.56f + Ratio * 0.24f;
    Crystal->SetRelativeScale3D(FVector(Width, Width, 0.8f + Ratio * 0.2f));
    LowerCrystal->SetRelativeScale3D(FVector(Width, Width, 0.5f + Ratio * 0.15f));
    Label->SetText(FText::FromString(IsBroken()
        ? FString::Printf(TEXT("TRAINING CRYSTAL\nReforming in %.1fs"), RespawnRemaining)
        : FString::Printf(TEXT("TRAINING CRYSTAL\n%.0f / 100 HP - try SPARK"), Health)));
}

void AHALVETHTrainingTarget::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0) return;
    Elapsed += DeltaSeconds;
    SetActorLocation(Home + FVector(0, 0, FMath::Sin(Elapsed * 1.3f) * 12));
    HitFlash = FMath::Max(0.0f, HitFlash - DeltaSeconds);
    if (IsBroken())
    {
        RespawnRemaining = FMath::Max(0.0f, RespawnRemaining - DeltaSeconds);
        const float Expansion = FMath::Min(1.0f, (5 - RespawnRemaining) * 2);
        for (int32 Index = 0; Index < Fragments.Num(); ++Index)
        {
            const float Angle = Index * PI / 3 + Elapsed * 0.8f;
            Fragments[Index]->SetRelativeLocation(FVector(FMath::Cos(Angle) * 85 * Expansion,
                FMath::Sin(Angle) * 85 * Expansion, FMath::Sin(Angle * 1.7f) * 40));
            Fragments[Index]->SetRelativeScale3D(FVector(0.10f + RespawnRemaining * 0.012f));
        }
        if (RespawnRemaining <= 0) ResetTarget();
    }
    else
    {
        Crystal->SetRelativeRotation(FRotator(0, Elapsed * 28, 0));
        LowerCrystal->SetRelativeRotation(FRotator(180, -Elapsed * 28, 0));
    }
    if (APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0))
    {
        FVector ViewLocation;
        FRotator ViewRotation;
        Player->GetPlayerViewPoint(ViewLocation, ViewRotation);
        Label->SetWorldRotation((ViewLocation - Label->GetComponentLocation()).Rotation());
    }
    UpdateAppearance();
}
