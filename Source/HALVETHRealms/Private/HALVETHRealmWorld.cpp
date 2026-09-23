#include "HALVETHRealmWorld.h"
#include "HALVETHTrainingTarget.h"
#include "Engine/World.h"
#include "RealmLayout.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    FLinearColor RealmColor(int32 Realm)
    {
        switch (Realm)
        {
            case 1: return FLinearColor(1.0f, 0.045f, 0.22f);
            case 2: return FLinearColor(0.035f, 0.63f, 1.0f);
            case 3: return FLinearColor(0.78f, 0.34f, 0.055f);
            default: return FLinearColor(0.63f, 0.18f, 0.8f);
        }
    }
}

AHALVETHRealmWorld::AHALVETHRealmWorld()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    CubeMesh = Cube.Object; SphereMesh = Sphere.Object; CylinderMesh = Cylinder.Object; ConeMesh = Cone.Object;
    Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
    Sun->SetupAttachment(RootComponent);
    Sun->SetMobility(EComponentMobility::Movable);
    Sun->SetRelativeRotation(FRotator(-28, -35, 0));
    Sun->SetIntensity(3.0f);
    Sun->SetAtmosphereSunLight(true);
    Sky = CreateDefaultSubobject<USkyLightComponent>(TEXT("Sky"));
    Sky->SetupAttachment(RootComponent);
    Sky->SetMobility(EComponentMobility::Movable);
    Sky->SetRealTimeCaptureEnabled(true);
    Sky->SetIntensity(0.6f);
    Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
    Atmosphere->SetupAttachment(RootComponent);
    Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
    Fog->SetupAttachment(RootComponent);
    Fog->SetFogDensity(0.018f);
    Fog->SetStartDistance(180);
    PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
    PostProcess->SetupAttachment(RootComponent);
    PostProcess->bUnbound = true;
    PostProcess->Settings.bOverride_BloomIntensity = true;
    PostProcess->Settings.BloomIntensity = 0.8f;
    PostProcess->Settings.bOverride_VignetteIntensity = true;
    PostProcess->Settings.VignetteIntensity = 0.25f;
}

FString AHALVETHRealmWorld::RealmName(int32 Realm)
{
    switch (Realm)
    {
        case 1: return TEXT("SCARLET GARDEN");
        case 2: return TEXT("LUCINET TIDELIGHT");
        case 3: return TEXT("RACHEL MEMORY GARDEN");
        default: return TEXT("HALVETH - THE CONFLUENCE");
    }
}

FString AHALVETHRealmWorld::RealmDescription(int32 Realm)
{
    switch (Realm)
    {
        case 1: return TEXT("Crimson canopies. A garden that answers with light.");
        case 2: return TEXT("Crystal tides. Luminous geometry above a quiet sea.");
        case 3: return TEXT("Golden archives. A place for stories yet to be written.");
        default: return TEXT("Three paths. One home. Every journey begins with care.");
    }
}

UStaticMeshComponent* AHALVETHRealmWorld::Shape(UStaticMesh* Mesh, FVector Position, FVector Scale,
    FLinearColor Color, float Glow, bool Collision, FRotator Rotation)
{
    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this);
    AddInstanceComponent(Component);
    Component->SetupAttachment(RootComponent);
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetStaticMesh(Mesh);
    Component->SetRelativeLocation(Position);
    Component->SetRelativeRotation(Rotation);
    Component->SetRelativeScale3D(Scale);
    Component->SetCollisionProfileName(Collision ? TEXT("BlockAll") : TEXT("NoCollision"));
    Component->SetGenerateOverlapEvents(false);
    Component->SetCastShadow(Glow < 2);
    if (SurfaceMaterial)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SurfaceMaterial, Component);
        Material->SetVectorParameterValue(TEXT("Tint"), Color);
        Material->SetScalarParameterValue(TEXT("Glow"), Glow);
        Component->SetMaterial(0, Material);
    }
    Component->RegisterComponent();
    Generated.Add(Component);
    return Component;
}

UPointLightComponent* AHALVETHRealmWorld::Light(FVector Position, FLinearColor Color, float Intensity, float Radius)
{
    UPointLightComponent* Component = NewObject<UPointLightComponent>(this);
    AddInstanceComponent(Component);
    Component->SetupAttachment(RootComponent);
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetRelativeLocation(Position);
    Component->SetIntensityUnits(ELightUnits::Lumens);
    Component->SetLightColor(Color);
    Component->SetIntensity(Intensity);
    Component->SetAttenuationRadius(Radius);
    Component->SetCastShadows(false);
    Component->RegisterComponent();
    Generated.Add(Component);
    return Component;
}

void AHALVETHRealmWorld::Label(FVector Position, const FString& Text, FLinearColor Color, float Size)
{
    UTextRenderComponent* Component = NewObject<UTextRenderComponent>(this);
    AddInstanceComponent(Component);
    Component->SetupAttachment(RootComponent);
    Component->SetRelativeLocation(Position);
    Component->SetRelativeRotation(FRotator(0, -90, 0));
    Component->SetText(FText::FromString(Text));
    Component->SetHorizontalAlignment(EHTA_Center);
    Component->SetWorldSize(Size);
    Component->SetTextRenderColor(Color.ToFColor(true));
    Component->RegisterComponent();
    Generated.Add(Component);
}

void AHALVETHRealmWorld::Portal(FVector Position, int32 Destination, FLinearColor Color)
{
    const FLinearColor Stone(0.11f, 0.10f, 0.16f);
    Shape(CubeMesh, Position + FVector(-165, 0, 205), FVector(0.48f, 0.75f, 4.1f), Stone);
    Shape(CubeMesh, Position + FVector(165, 0, 205), FVector(0.48f, 0.75f, 4.1f), Stone);
    Shape(CubeMesh, Position + FVector(0, 0, 420), FVector(3.85f, 0.8f, 0.45f), Stone);
    Shape(CubeMesh, Position + FVector(0, 0, 20), FVector(3.7f, 1.15f, 0.4f), Stone);
    for (int32 Index = 0; Index < 7; ++Index)
    {
        const float Angle = PI * Index / 6;
        Shape(SphereMesh, Position + FVector(FMath::Cos(Angle) * 175, -45, 305 + FMath::Sin(Angle) * 125),
            FVector(0.23f), Color, 4, false);
    }
    FHALVETHPortal Link;
    Link.Position = Position;
    Link.Destination = Destination;
    Link.Veil = Shape(SphereMesh, Position + FVector(0, 8, 215), FVector(2.8f, 0.10f, 3.7f), Color * 0.3f, 2, false);
    Link.Light = Light(Position + FVector(0, -110, 240), Color, 1900, 850);
    Portals.Add(Link);
    Label(Position + FVector(0, -60, 485), RealmName(Destination), Color, 22);
}

void AHALVETHRealmWorld::BuildRealm(int32 Realm, uint32 Seed)
{
    if (!HalvethLayout::ValidRealm(Realm)) return;
    if (TrainingTarget) { TrainingTarget->Destroy(); TrainingTarget = nullptr; }
    GuideHeads.Empty();
    Portals.Empty(); Fireflies.Empty(); FireflyOrigins.Empty();
    LoveOrb = nullptr; LoveLight = nullptr;
    for (UActorComponent* Component : Generated) if (Component) Component->DestroyComponent();
    Generated.Empty();
    CurrentRealm = Realm;
    LoveRemaining = 0;
    Accent = RealmColor(Realm);
    SurfaceMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_HalvethSurface.M_HalvethSurface"));
    if (!SurfaceMaterial)
    {
        SurfaceMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
        UE_LOG(LogTemp, Warning, TEXT("HALVETH_ASSETS_MISSING: run HALVETHPrepare before rendering or packaging."));
    }
    Sun->SetLightColor(FMath::Lerp(FLinearColor::White, Accent, 0.24f));
    Sun->SetIntensity(Realm == 2 ? 2.3f : 3.2f);
    Fog->SetFogInscatteringColor(Accent * 0.035f);
    Sky->SetLightColor(FMath::Lerp(FLinearColor::White, Accent, 0.3f));
    const FLinearColor Floor = Realm == 1 ? FLinearColor(0.065f, 0.045f, 0.070f)
        : Realm == 2 ? FLinearColor(0.026f, 0.065f, 0.09f)
        : Realm == 3 ? FLinearColor(0.13f, 0.10f, 0.045f) : FLinearColor(0.075f, 0.065f, 0.115f);
    auto* Ground = Shape(CylinderMesh, FVector(0, 0, -75), FVector(48, 48, 1.5f), Floor);
    const TCHAR* GroundPath = Realm == 1
        ? TEXT("/Game/Materials/M_PH_ForestGround04.M_PH_ForestGround04")
        : TEXT("/Game/Materials/M_PH_CobblestoneFloor03.M_PH_CobblestoneFloor03");
    if (auto* GroundMaterial = LoadObject<UMaterialInterface>(nullptr, GroundPath)) Ground->SetMaterial(0, GroundMaterial);
    Shape(SphereMesh, FVector(0, 0, -480), FVector(44, 44, 10), Floor * 0.6f, 0, false);
    // These luminous rings are decorative; the cylinder remains the collision floor.
    for (int32 Index = 0; Index < 48; ++Index)
    {
        const float Angle = 2 * PI * Index / 48;
        Shape(CubeMesh, FVector(FMath::Cos(Angle) * 2210, FMath::Sin(Angle) * 2210, 12),
            FVector(2.7f, 0.08f, 0.10f), Accent, 2, false, FRotator(0, FMath::RadiansToDegrees(Angle) + 90, 0));
    }
    for (int32 Index = -5; Index < 9; ++Index)
    {
        Shape(CubeMesh, FVector(0, Index * 150, 4), FVector(4.3f, 1.3f, 0.08f), Floor * 1.8f);
        Shape(CubeMesh, FVector(-230, Index * 150, 12), FVector(0.05f, 0.65f, 0.05f), Accent, 3, false);
        Shape(CubeMesh, FVector(230, Index * 150, 12), FVector(0.05f, 0.65f, 0.05f), Accent, 3, false);
    }
    const auto Layout = HalvethLayout::Build(Seed, Realm);
    for (const HalvethLayout::Prop& Prop : Layout)
    {
        const FVector Base(Prop.X, Prop.Y, 0);
        const float Height = static_cast<float>(Prop.Height);
        if (Realm == 1)
        {
            Shape(CylinderMesh, Base + FVector(0, 0, Height / 2), FVector(0.16f, 0.16f, Height / 100), FLinearColor(0.18f, 0.10f, 0.11f));
            Shape(SphereMesh, Base + FVector(0, 0, Height), FVector(1.7f, 1.7f, 0.85f), Accent * (0.25f + Prop.Variant * 0.14f), 0.18f, false);
            Shape(SphereMesh, Base + FVector(45, 0, Height + 28), FVector(0.3f), FLinearColor(1, 0.24f, 0.18f), 4, false);
        }
        else if (Realm == 2)
        {
            Shape(ConeMesh, Base + FVector(0, 0, Height / 2), FVector(0.8f, 0.8f, Height / 100), Accent * 0.32f, 0.4f, true, FRotator(10, Prop.Rotation, 0));
            Shape(ConeMesh, Base + FVector(70, 25, Height / 4), FVector(0.4f, 0.4f, Height / 200), Accent * 0.65f, 0.25f, true, FRotator(-18, Prop.Rotation, 0));
        }
        else if (Realm == 3)
        {
            Shape(CubeMesh, Base + FVector(0, 0, Height / 2), FVector(0.7f, 0.6f, Height / 100), Floor * 1.6f, 0, true, FRotator(0, Prop.Rotation, 0));
            Shape(SphereMesh, Base + FVector(0, 0, Height + 35), FVector(0.5f), Accent, 3, false);
            Shape(CubeMesh, Base + FVector(0, 0, 16), FVector(1.25f, 1.25f, 0.3f), Floor * 2);
        }
        else
        {
            Shape(CylinderMesh, Base + FVector(0, 0, Height / 2), FVector(0.45f, 0.45f, Height / 100), Floor * 1.8f);
            Shape(SphereMesh, Base + FVector(0, 0, Height + 22), FVector(0.65f, 0.65f, 0.85f), RealmColor(1 + Prop.Variant), 1.5f, false);
        }
    }
    if (Realm == 0)
    {
        Guide(FVector(-380, -530, 0), 1);
        Guide(FVector(380, 0, 0), 2);
        Guide(FVector(-380, 600, 0), 3);
        TrainingTarget = GetWorld()->SpawnActor<AHALVETHTrainingTarget>(FVector(430, -590, 140), FRotator::ZeroRotator);
        Portal(FVector(-470, 1050, 0), 1, RealmColor(1));
        Portal(FVector(0, 1270, 0), 2, RealmColor(2));
        Portal(FVector(470, 1050, 0), 3, RealmColor(3));
    }
    else
    {
        Guide(FVector(-380, -470, 0), Realm);
        Portal(FVector(0, 1180, 0), 0, RealmColor(0));
        // A central landmark gives each destination a silhouette, not only a palette.
        if (Realm == 1)
        {
            Shape(CylinderMesh, FVector(0, 200, 190), FVector(0.8f, 0.8f, 3.8f), Floor * 2);
            Shape(SphereMesh, FVector(-85, 200, 465), FVector(2, 1.3f, 2), Accent, 0.3f, false);
            Shape(SphereMesh, FVector(85, 200, 465), FVector(2, 1.3f, 2), Accent, 0.3f, false);
            Shape(ConeMesh, FVector(0, 200, 365), FVector(3.2f, 1.4f, 2.7f), Accent, 0.3f, false, FRotator(180, 0, 0));
        }
        else if (Realm == 2)
        {
            for (int32 Index = 0; Index < 8; ++Index)
                Shape(CubeMesh, FVector(0, 250, 260 + Index * 80), FVector(2.6f - Index * 0.2f, 2.6f - Index * 0.2f, 0.2f), Accent * 0.45f, 0.8f, false, FRotator(0, Index * 18, 0));
        }
        else
        {
            for (int32 Index = 0; Index < 3; ++Index)
            {
                Shape(CubeMesh, FVector(-200, 250, 150 + Index * 140), FVector(0.7f, 0.8f, 1.4f), Floor * 1.8f);
                Shape(CubeMesh, FVector(200, 250, 150 + Index * 140), FVector(0.7f, 0.8f, 1.4f), Floor * 1.8f);
                Shape(CubeMesh, FVector(0, 250, 220 + Index * 140), FVector(4.7f, 1.0f, 0.35f), Accent * 0.6f, 0.2f);
            }
        }
    }
    for (int32 Index = 0; Index < 30; ++Index)
    {
        const HalvethLayout::Prop& Prop = Layout[Index];
        const FVector Position(Prop.X * 0.76f, Prop.Y * 0.8f, 140 + Prop.Height);
        FireflyOrigins.Add(Position);
        Fireflies.Add(Shape(SphereMesh, Position, FVector(0.045f), Accent, 6, false));
    }
    LoveOrb = Shape(SphereMesh, FVector(0, 0, 120), FVector(0.01f), FLinearColor(1, 0.1f, 0.25f), 5, false);
    LoveOrb->SetVisibility(false);
    LoveLight = Light(FVector(0, 0, 250), FLinearColor(1, 0.22f, 0.37f), 0, 1600);
    Label(FVector(0, 1630, 650), RealmName(Realm), FLinearColor(0.96f, 0.82f, 0.55f), 55);
    UE_LOG(LogTemp, Display, TEXT("HALVETH_REALM_READY realm=%d seed=%u props=%d portals=%d fingerprint=%llu"),
        Realm, Seed, HalvethLayout::PropCount, Portals.Num(), static_cast<unsigned long long>(HalvethLayout::Fingerprint(Layout)));
}

void AHALVETHRealmWorld::Guide(FVector Position, int32 Identity)
{
    const FLinearColor Color = RealmColor(Identity);
    const FName GuideTags[] = {TEXT("HALVETH_NPC_SCARLET"), TEXT("HALVETH_NPC_LUCINET"), TEXT("HALVETH_NPC_RACHEL")};
    const FString Names[] = {TEXT("SCARLET"), TEXT("LUCINET"), TEXT("RACHEL")};
    auto* Body = Shape(ConeMesh, Position + FVector(0, 0, 73), FVector(0.85f, 0.65f, 1.45f), Color * 0.25f, 0.1f);
    Body->ComponentTags.Add(GuideTags[Identity - 1]);
    auto* Head = Shape(SphereMesh, Position + FVector(0, 0, 170), FVector(0.43f), FLinearColor(0.72f, 0.58f, 0.40f), 0.25f, false);
    GuideHeads.Add(Head);
    Shape(SphereMesh, Position + FVector(-7, -20, 174), FVector(0.055f), Color, 4, false);
    Shape(SphereMesh, Position + FVector(7, -20, 174), FVector(0.055f), Color, 4, false);
    Shape(CylinderMesh, Position + FVector(48, 0, 104), FVector(0.055f, 0.055f, 2.05f), FLinearColor(0.30f, 0.21f, 0.10f));
    Shape(SphereMesh, Position + FVector(48, 0, 216), FVector(0.24f), Color, 4, false);
    Light(Position + FVector(0, -65, 160), Color, 450, 290);
    Label(Position + FVector(0, 0, 253), Names[Identity - 1] + TEXT("  [E]"), FLinearColor(0.96f, 0.84f, 0.60f), 19);
}

int32 AHALVETHRealmWorld::FindPortal(const FVector& Position) const
{
    float Best = 310.0f * 310.0f;
    int32 Result = INDEX_NONE;
    for (const FHALVETHPortal& Portal : Portals)
    {
        const float Distance = FVector::DistSquared2D(Position, Portal.Position);
        if (Distance < Best && FMath::Abs(Position.Z - Portal.Position.Z) < 350)
        {
            Best = Distance; Result = Portal.Destination;
        }
    }
    return Result;
}

FVector AHALVETHRealmWorld::GetPortalPosition(int32 Destination) const
{
    for (const FHALVETHPortal& Portal : Portals) if (Portal.Destination == Destination) return Portal.Position;
    return FVector::ZeroVector;
}

void AHALVETHRealmWorld::BeginLovePulse()
{
    LoveRemaining = 4;
    if (LoveOrb) LoveOrb->SetVisibility(true);
}

void AHALVETHRealmWorld::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;
    for (int32 Index = 0; Index < GuideHeads.Num(); ++Index)
        if (GuideHeads[Index]) GuideHeads[Index]->SetRelativeScale3D(FVector(0.43f + FMath::Sin(Elapsed * 1.5f + Index) * 0.006f));
    LoveRemaining = FMath::Max(0.0f, LoveRemaining - DeltaSeconds);
    const float Strength = GetLoveStrength();
    for (int32 Index = 0; Index < Fireflies.Num(); ++Index)
    {
        const float Phase = Elapsed * 0.7f + Index * 1.7f;
        Fireflies[Index]->SetRelativeLocation(FireflyOrigins[Index] + FVector(FMath::Sin(Phase) * 30, FMath::Cos(Phase) * 20, FMath::Sin(Phase * 1.3f) * 38));
        Fireflies[Index]->SetRelativeScale3D(FVector(0.045f + Strength * 0.10f));
    }
    for (const FHALVETHPortal& Portal : Portals)
        if (Portal.Light) Portal.Light->SetIntensity(1750 + FMath::Sin(Elapsed * 1.6f) * 220 + Strength * 1600);
    if (LoveOrb)
    {
        LoveOrb->SetVisibility(LoveRemaining > 0);
        LoveOrb->SetRelativeScale3D(FVector((1 - Strength) * 1.8f + 0.2f));
        LoveOrb->SetRelativeLocation(FVector(0, 0, 170 + (1 - Strength) * 360));
    }
    if (LoveLight) LoveLight->SetIntensity(Strength * 15000);
}
