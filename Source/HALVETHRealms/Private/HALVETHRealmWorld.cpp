#include "HALVETHRealmWorld.h"
#include "HALVETHTrainingTarget.h"
#include "HALVETHCharacter.h"
#include "HALVETHAdventureComponent.h"
#include "HALVETHGuideAnimInstance.h"
#include "HALVETHGuideCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "EarthDynamicsMath.h"
#include "CollisionShape.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Engine/World.h"
#include "RealmLayout.h"
#include "KnowledgeSystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GuideGenome.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
THIRD_PARTY_INCLUDES_START
#include <openssl/sha.h>
THIRD_PARTY_INCLUDES_END

namespace
{
    bool HashBodyData(const void* Data,uint32 Bytes,FSHA256Signature& Out){
        return SHA256(static_cast<const unsigned char*>(Data),Bytes,Out.Signature)!=nullptr;
    }
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
    // The realm origin remains fixed; animated guides/lights are movable children.
    // Static terrain and foliage must be attached to a compatible static parent.
    RootComponent->SetMobility(EComponentMobility::Static);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    CubeMesh = Cube.Object; SphereMesh = Sphere.Object; CylinderMesh = Cylinder.Object; ConeMesh = Cone.Object;
    Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
    Sun->SetupAttachment(RootComponent);
    Sun->SetMobility(EComponentMobility::Movable);
    Sun->SetRelativeRotation(FRotator(-28, -35, 0));
    Sun->SetIntensity(48000);
    Sun->SetLightSourceAngle(.7f);
    Sun->SetAtmosphereSunLight(true);
    Sun->DynamicShadowDistanceMovableLight=30000;
    Sun->SetDynamicShadowCascades(4);
    // Authored night fill preserves navigation; it is not a NASA Moon model.
    NightFill=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("AuthoredNightFill"));
    NightFill->SetupAttachment(RootComponent);NightFill->SetMobility(EComponentMobility::Movable);
    NightFill->SetRelativeRotation(FRotator(-42,75,0));NightFill->SetLightColor(FLinearColor(.35f,.48f,.72f));
    NightFill->SetIntensity(0);NightFill->SetCastShadows(false);
    Sky = CreateDefaultSubobject<USkyLightComponent>(TEXT("Sky"));
    Sky->SetupAttachment(RootComponent);
    Sky->SetMobility(EComponentMobility::Movable);
    Sky->SetRealTimeCaptureEnabled(true);
    Sky->SetIntensity(1.4f);
    Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
    Atmosphere->SetupAttachment(RootComponent);
    Clouds=CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));Clouds->SetupAttachment(RootComponent);
    Clouds->SetLayerBottomAltitude(1.2f);Clouds->SetLayerHeight(4.5f);
    Clouds->SetMaterial(LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst")));
    Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
    Fog->SetupAttachment(RootComponent);
    Fog->SetFogDensity(.007f);
    Fog->SetFogHeightFalloff(.15f);
    Fog->SetVolumetricFog(true);
    Fog->SetRelativeLocation(FVector(0,0,-220));
    Fog->SetStartDistance(180);
    PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
    PostProcess->SetupAttachment(RootComponent);
    PostProcess->bUnbound = true;
    PostProcess->Settings.bOverride_BloomIntensity = true;
    PostProcess->Settings.BloomIntensity = .22f;
    PostProcess->Settings.bOverride_AutoExposureMethod=true;
    PostProcess->Settings.AutoExposureMethod=AEM_Histogram;
    PostProcess->Settings.bOverride_AutoExposureMinBrightness=true;
    PostProcess->Settings.bOverride_AutoExposureMaxBrightness=true;
    PostProcess->Settings.AutoExposureMinBrightness=12;
    PostProcess->Settings.AutoExposureMaxBrightness=12;
    PostProcess->Settings.bOverride_VignetteIntensity = true;
    PostProcess->Settings.VignetteIntensity = .12f;
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
        Material->SetScalarParameterValue(TEXT("Glow"), Glow*1400);
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
    if(auto* Arch=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Garden/PortalArch.PortalArch"))) {
        auto* Frame=Shape(Arch,Position,FVector(1),FLinearColor::White);
        Frame->SetMaterial(0,Arch->GetMaterial(0));
    }
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
    if(BodyOrigin.IsEmpty()){
        if(!FParse::Value(FCommandLine::Get(),TEXT("HalvethBodyOrigin="),BodyOrigin))BodyOrigin=FString::Printf(TEXT("HALVETH/body-origin/v1/seed/%u"),Seed);
    }
    if (TrainingTarget) { TrainingTarget->Destroy(); TrainingTarget = nullptr; }
    for(auto GuideActor:GuideActors)if(GuideActor)GuideActor->Destroy();GuideActors.Empty();
    GuideHeads.Empty();
    GuideBodies.Empty(); GuideOrigins.Empty(); GuideIdentities.Empty();
    GuideBoneBaseline.Empty();GuidePoseChanged.Empty();
    GuidePatrolTime.Empty();GuideSpatialTravel.Empty();GuideHandBaseline.Empty();GuideHandTravel.Empty();GuideHeadBaseline.Empty();GuideHeadTravel.Empty();
    GuideChoices.Empty();GuideGoals.Empty();GuideDecisionAt.Empty();GuideRestUntil.Empty();GuideBlockedTime.Empty();GuideDecisionCount.Empty();
    GuideProfiles.Empty();GuideBodyStates.Empty();
    SoilComponents.Empty();SoilCenters.Empty();SoilVertices.Empty();SoilColors.Empty();SoilLoads.Empty();SoilDepths.Empty();Stars.Empty();
    Portals.Empty(); Fireflies.Empty(); FireflyOrigins.Empty();
    LoveOrb = nullptr; LoveLight = nullptr;
    for (UActorComponent* Component : Generated) if (Component) Component->DestroyComponent();
    Generated.Empty();
    TerrainComponent=nullptr;
    CurrentRealm = Realm;
    LoveRemaining = 0;
    Accent = RealmColor(Realm);
    SurfaceMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_HalvethSurface.M_HalvethSurface"));
    if (!SurfaceMaterial)
    {
        SurfaceMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
        UE_LOG(LogTemp, Warning, TEXT("HALVETH_ASSETS_MISSING: run HALVETHPrepare before rendering or packaging."));
    }
    Sun->SetLightColor(FMath::Lerp(FLinearColor(1,.96,.90),Accent,.055f));
    Sun->SetIntensity(Realm==1 ? 38000 : 48000);
    Fog->SetFogInscatteringColor(Accent * 0.035f);
    Sky->SetLightColor(FMath::Lerp(FLinearColor::White,Accent,.03f));
    const FLinearColor Floor = Realm == 1 ? FLinearColor(0.065f, 0.045f, 0.070f)
        : Realm == 2 ? FLinearColor(0.026f, 0.065f, 0.09f)
        : Realm == 3 ? FLinearColor(0.13f, 0.10f, 0.045f) : FLinearColor(0.075f, 0.065f, 0.115f);
    BuildLandscape(Seed);
    GetWorld()->GetWorldSettings()->bGlobalGravitySet=true;
    GetWorld()->GetWorldSettings()->GlobalGravityZ=-100*EarthDynamics::GravityMps2;
    InitializeEarthSky();
    const auto Layout = HalvethLayout::Build(Seed, Realm);
    ReadablePositions.Empty();
    if (Realm == 0)
    {
        Readable(FVector(-680, -960, 0), 0);
        Readable(FVector(670, 450, 0), 1);
        Readable(FVector(-670, 450, 0), 2);
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
        Readable(FVector(720, -250, 0), Realm + 2);
        Guide(FVector(-380, -470, 0), Realm);
        Portal(FVector(0, 1180, 0), 0, RealmColor(0));
        // Distinct destinations now have terrain, vegetation and a lake rather
        // than primitive block towers. Their books and routes are the same game.
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

    UE_LOG(LogTemp, Display, TEXT("HALVETH_REALM_READY realm=%d seed=%u props=%d portals=%d fingerprint=%llu"),
        Realm, Seed, HalvethLayout::PropCount, Portals.Num(), static_cast<unsigned long long>(HalvethLayout::Fingerprint(Layout)));
}

void AHALVETHRealmWorld::Guide(FVector Position, int32 Identity)
{
    const FLinearColor Color = RealmColor(Identity);
    const FName GuideTags[] = {TEXT("HALVETH_NPC_SCARLET"), TEXT("HALVETH_NPC_LUCINET"), TEXT("HALVETH_NPC_RACHEL")};
    const FString Names[] = {TEXT("SCARLET"), TEXT("LUCINET"), TEXT("RACHEL")};
    const FString CharacterName=Identity==1?TEXT("Scarlet"):Identity==2?TEXT("Lucinet"):TEXT("Rachel");
    const FString Path=TEXT("/Game/Characters/")+CharacterName+TEXT(".")+CharacterName;
    BuildSoil(Position);
    FActorSpawnParameters Spawn;Spawn.Owner=this;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* GuideActor=GetWorld()->SpawnActor<AHALVETHGuideCharacter>(GetActorLocation()+Position+FVector(0,0,96),FRotator::ZeroRotator,Spawn);
    if(!GuideActor)return;GuideActors.Add(GuideActor);
    GuideActor->GetCharacterMovement()->Mass=Identity==1?68:Identity==2?82:61;
    auto* Body=GuideActor->GetMesh();
    Body->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr,*Path));
    Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    Body->SetAnimInstanceClass(UHALVETHGuideAnimInstance::StaticClass());
    Body->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->ComponentTags.Add(GuideTags[Identity-1]);GuideActor->Tags.Add(GuideTags[Identity-1]);
    const FString IdlePath=TEXT("/Game/Characters/")+CharacterName+TEXT("Idle.")+CharacterName+TEXT("Idle");
    auto* Idle=LoadObject<UAnimSequence>(nullptr,*IdlePath);
    if(auto* Motion=Cast<UHALVETHGuideAnimInstance>(Body->GetAnimInstance())){Motion->BaseIdle=Idle;Motion->Identity=Identity;}
    GuideBodies.Add(Body); GuideOrigins.Add(Position); GuideIdentities.Add(Identity);
    GuideBoneBaseline.Add(FQuat::Identity);GuidePoseChanged.Add(false);
    GuidePatrolTime.Add(Identity*3.7);GuideSpatialTravel.Add(0);GuideHandBaseline.Add(FVector::ZeroVector);GuideHandTravel.Add(0);GuideHeadBaseline.Add(FQuat::Identity);GuideHeadTravel.Add(0);
    GuideChoices.Emplace(Identity*7919+CurrentRealm*104729);GuideGoals.Add(GuideActor->GetActorLocation());
    GuideDecisionAt.Add(0);GuideRestUntil.Add(0);GuideBlockedTime.Add(0);GuideDecisionCount.Add(0);
    if(!BodyLineages.Contains(Identity)){
        FHALVETHBodyLineage Line;Line.Profile=HalvethBody::Profile::ForIdentity(Identity);
        Line.JournalPath=FPaths::ProjectSavedDir()/TEXT("BodyLineage")/(FGuid::NewGuid().ToString(EGuidFormats::Digits)+FString::Printf(TEXT("-entity-%d.jsonl"),Identity));
        BodyLineages.Add(Identity,MoveTemp(Line));GrowBodyLineage(Identity,TEXT("birth"));
    }
    GuideProfiles.Add(BodyLineages[Identity].Profile);GuideBodyStates.Add(BodyLineages[Identity].State);
    GuideActor->GetCharacterMovement()->MaxWalkSpeed=GuideProfiles.Last().paceCmS;
    GuideActor->GetCharacterMovement()->Mass=GuideProfiles.Last().massKg;
    UE_LOG(LogTemp,Display,TEXT("GARDEN_GUIDE_READY identity=%d skeletal=%d idle=%d own_clothing=true cc0_human=true morphs=%d"),Identity,Body->GetSkeletalMeshAsset()!=nullptr,Idle!=nullptr,Body->GetSkeletalMeshAsset()?Body->GetSkeletalMeshAsset()->GetMorphTargets().Num():0);
    auto* Glow=Light(Position + FVector(0, -65, 160), Color, 180, 290);
    Glow->AttachToComponent(Body,FAttachmentTransformRules::KeepWorldTransform);
    Label(Position + FVector(0, 0, 213), Names[Identity - 1] + TEXT("  [E]"), FLinearColor(0.96f, 0.84f, 0.60f), 12);
    if(auto* Name=Cast<USceneComponent>(Generated.Last()))Name->AttachToComponent(Body,FAttachmentTransformRules::KeepWorldTransform);
}

FVector AHALVETHRealmWorld::GetGuidePosition(int32 Identity) const {
    for(int I=0;I<GuideIdentities.Num();I++)if(GuideIdentities[I]==Identity&&GuideBodies[I])return GuideBodies[I]->GetComponentLocation();
    return FVector::ZeroVector;
}

AHALVETHGuideCharacter* AHALVETHRealmWorld::GetGuideCharacter(int32 Identity) const {
    for(int I=0;I<GuideIdentities.Num();I++)if(GuideIdentities[I]==Identity&&GuideActors.IsValidIndex(I))return GuideActors[I];
    return nullptr;
}

void AHALVETHRealmWorld::GrowBodyLineage(int32 Identity,const FString& Event){
    auto* Line=BodyLineages.Find(Identity);if(!Line)return;
    const FString Parent=Line->Digest;
    auto Record=MakeShared<FJsonObject>();
    Record->SetStringField(TEXT("schema"),TEXT("halveth.virtual-body-lineage.v2"));
    Record->SetStringField(TEXT("lineageId"),FPaths::GetBaseFilename(Line->JournalPath));
    Record->SetStringField(TEXT("entityId"),FString::Printf(TEXT("halveth-guide-%d"),Identity));
    Record->SetStringField(TEXT("event"),Event);Record->SetStringField(TEXT("recordedAt"),FDateTime::UtcNow().ToIso8601());
    Record->SetNumberField(TEXT("simulationSeconds"),Elapsed);Record->SetStringField(TEXT("parentDigest"),Parent);
    Record->SetStringField(TEXT("generation"),LexToString(Line->Generation));
    Record->SetNumberField(TEXT("muscleReserve"),Line->State.muscleReserve);
    if(Line->Generation==0){
        FTCHARToUTF8 OriginBytes(*BodyOrigin);FSHA256Signature OriginSignature;
        if(!HashBodyData(OriginBytes.Get(),OriginBytes.Length(),OriginSignature))return;
        Record->SetStringField(TEXT("originDigest"),OriginSignature.ToString());
    }
    FString Payload;FJsonSerializer::Serialize(Record,TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Payload));
    FTCHARToUTF8 Bytes(*Payload);FSHA256Signature Signature;
    if(!HashBodyData(Bytes.Get(),Bytes.Length(),Signature)){
        UE_LOG(LogTemp,Warning,TEXT("GARDEN_GENOME_WRITE identity=%d result=hash_unavailable"),Identity);return;
    }
    HalvethGenome::Digest Data;for(int J=0;J<32;J++)Data[J]=Signature.Signature[J];
    const std::string Bases=HalvethGenome::Encode(Data);auto NextProfile=Line->Profile;
    if(Line->Generation==0){
        NextProfile.massKg*=.97+.06*double(Data[0])/255.;
        NextProfile.paceCmS=56+28*double(Data[1])/255.;
        NextProfile.curiosity=double(Data[2])/255.;NextProfile.reactionSeconds=.14+.12*double(Data[3])/255.;
    }else{
        NextProfile.curiosity+=.01*(Line->State.muscleReserve-.5);
        NextProfile.paceCmS+=.025*(Line->State.muscleReserve-.5);
    }
    Record->SetStringField(TEXT("sourcePayload"),Payload);
    Record->SetStringField(TEXT("sourceDigest"),Signature.ToString());Record->SetStringField(TEXT("newBases"),UTF8_TO_TCHAR(Bases.c_str()));
    Record->SetNumberField(TEXT("massKg"),NextProfile.massKg);Record->SetNumberField(TEXT("paceCmS"),NextProfile.paceCmS);
    Record->SetNumberField(TEXT("curiosity"),NextProfile.curiosity);Record->SetNumberField(TEXT("reactionSeconds"),NextProfile.reactionSeconds);
    FString Encoded;FJsonSerializer::Serialize(Record,TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Encoded));
    FTCHARToUTF8 RecordBytes(*Encoded);FSHA256Signature RecordSignature;
    if(!HashBodyData(RecordBytes.Get(),RecordBytes.Length(),RecordSignature))return;
    FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FPaths::GetPath(Line->JournalPath));
    if(!FFileHelper::SaveStringToFile(Encoded+TEXT("\n"),*Line->JournalPath,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_Append)){
        UE_LOG(LogTemp,Warning,TEXT("GARDEN_GENOME_WRITE identity=%d result=journal_failed"),Identity);return;
    }
    Line->Profile=NextProfile;Line->Digest=RecordSignature.ToString();Line->Genome+=UTF8_TO_TCHAR(Bases.c_str());Line->Generation++;
    UE_LOG(LogTemp,Display,TEXT("GARDEN_GENOME_WRITE identity=%d generation=%llu bases=%d parent=%s digest=%s result=appended"),Identity,Line->Generation,Line->Genome.Len(),*Parent,*Line->Digest);
}

bool AHALVETHRealmWorld::VerifyGuidePatrol() const
{
    if (GuideSpatialTravel.Num() != 3) return false;
    for (int32 I=0; I<GuideSpatialTravel.Num(); ++I)
    {
        const bool Passed = GuideSpatialTravel[I] >= 150.;
        UE_LOG(LogTemp,Display,TEXT("GARDEN_PATROL_AUDIT identity=%d passed=%d observed_max_distance_cm=%.5f required_distance_cm=150"),GuideIdentities[I],Passed,GuideSpatialTravel[I]);
        if (!Passed) return false;
    }
    return true;
}

bool AHALVETHRealmWorld::VerifyCharacters() const
{
    if(GuideBodies.Num()!=3)return false;
    for(int32 I=0;I<GuideBodies.Num();I++) {
        const auto* Body=GuideBodies[I].Get(); if(!Body)return false;
        const auto* Mesh=Body->GetSkeletalMeshAsset();
        const auto* Anim=Cast<UHALVETHGuideAnimInstance>(Body->GetAnimInstance());
        const auto* Idle=Anim?Anim->BaseIdle.Get():nullptr;
        const bool Bound=Mesh&&Mesh->GetSkeleton()&&Idle&&Idle->GetSkeleton()==Mesh->GetSkeleton()
            &&Mesh->GetRefSkeleton().GetNum()>=163&&Idle->GetPlayLength()>=11.9
            &&Mesh->FindMorphTarget(TEXT("Blink"))&&Mesh->FindMorphTarget(TEXT("Talk"))
            &&Mesh->FindMorphTarget(TEXT("ClothLeft"))&&Mesh->FindMorphTarget(TEXT("HairRight"))&&Mesh->FindMorphTarget(TEXT("Breath"))
            &&Body->GetBoneIndex(TEXT("spine02"))!=INDEX_NONE
            &&Body->GetBoneIndex(TEXT("wrist_R"))!=INDEX_NONE&&Body->GetBoneIndex(TEXT("upperleg01_L"))!=INDEX_NONE
            &&GuideActors.IsValidIndex(I)&&GuideActors[I]&&GuideActors[I]->GetCharacterMovement()->GravityScale==1;
        const bool Motion=GuidePoseChanged.IsValidIndex(I)&&GuidePoseChanged[I];
        UE_LOG(LogTemp,Display,TEXT("GARDEN_CHARACTER_AUDIT identity=%d skeleton_animation_morphs=%d sampled_internal_pose_change=%d"),GuideIdentities[I],Bound,Motion);
        UE_LOG(LogTemp,Display,TEXT("GARDEN_MOTION_AUDIT identity=%d hand_travel_cm=%.5f head_angle_radians=%.5f independent_bones=1 foot_ik=1"),GuideIdentities[I],GuideHandTravel[I],GuideHeadTravel[I]);
        UE_LOG(LogTemp,Display,TEXT("GARDEN_GUIDE_WALK_EXTENT identity=%d observed_max_distance_from_start_cm=%.5f decisions=%d locomotion=terrain_checked_individual_goal_selection"),GuideIdentities[I],GuideSpatialTravel[I],GuideDecisionCount[I]);
        const auto& Phys=GuideBodyStates[I];const auto& Genes=GuideProfiles[I];const auto Mass=HalvethBody::MassBudget(Genes.massKg);
        UE_LOG(LogTemp,Display,TEXT("GARDEN_BODY_AUDIT identity=%d model=fictional_gameplay_physiology mass_kg=%.5f tissue_budget_kg=%.5f muscle_reserve=%.5f muscle_power=%.5f oxygen_supply=%.5f heart_rate_bpm=%.5f breathing_rate_per_min=%.5f reaction_seconds=%.5f tendon_strain=%.5f decisions=%d"),GuideIdentities[I],Genes.massKg,Mass.total(),Phys.muscleReserve,Phys.musclePower,Phys.oxygenSupply,Phys.heartRateBpm,Phys.breathsPerMinute,Phys.reactionSeconds,Phys.tendonStrain,GuideDecisionCount[I]);
        if(!Bound||!Motion||GuideHandTravel[I]<1.0||GuideHeadTravel[I]<.01)return false;
    }
    return true;
}

void AHALVETHRealmWorld::Readable(FVector Position, int32 Book)
{
    if (Book < 0 || Book >= static_cast<int32>(halveth::knowledge::BookCount)) return;
    ReadablePositions.Add(Book, Position);
    const FLinearColor Cover = RealmColor(1 + Book % 3);
    Shape(CylinderMesh, Position + FVector(0, 0, 45), FVector(0.16f, 0.16f, 0.9f), FLinearColor(0.15f, 0.11f, 0.10f));
    Shape(CubeMesh, Position + FVector(0, 0, 92), FVector(0.95f, 0.65f, 0.09f), FLinearColor(0.22f, 0.13f, 0.11f));
    if (Book == 3)
    {
        Shape(CubeMesh, Position + FVector(0, 0, 102), FVector(0.65f, 0.48f, 0.015f), FLinearColor(0.88f, 0.76f, 0.50f));
        for (int32 Side : {-1, 1}) Shape(CylinderMesh, Position + FVector(Side * 34, 0, 105), FVector(0.09f, 0.09f, 0.61f), Cover, 0.3f, false, FRotator(90, 0, 0));
    }
    else
    {
        Shape(CubeMesh, Position + FVector(0, 0, 101), FVector(0.69f, 0.48f, Book == 4 ? 0.018f : 0.07f), Cover, 0.15f);
        Shape(CubeMesh, Position + FVector(-18, 0, 108), FVector(0.30f, 0.43f, 0.025f), FLinearColor(0.93f, 0.82f, 0.60f), 0, false, FRotator(0, 0, -8));
        Shape(CubeMesh, Position + FVector(18, 0, 108), FVector(0.30f, 0.43f, 0.025f), FLinearColor(0.93f, 0.82f, 0.60f), 0, false, FRotator(0, 0, 8));
    }
    for (int32 Line = 0; Line < 4; ++Line)
        Shape(CubeMesh, Position + FVector(0, -14 + Line * 9, 112), FVector(0.43f, 0.009f, 0.008f), Cover * 0.18f, 0, false);
    Light(Position + FVector(0, 0, 175), Cover, 150, 200);
    Label(Position + FVector(0, 0, 171), TEXT("E  /  READ"), FLinearColor(0.96f, 0.84f, 0.60f), 15);
}

int32 AHALVETHRealmWorld::FindReadable(const FVector& Position) const
{
    int32 Result = INDEX_NONE; float Best = 180.0f * 180.0f;
    for (const auto& Entry : ReadablePositions)
    {
        const float Distance = FVector::DistSquared2D(Position, Entry.Value);
        if (Distance < Best && FMath::Abs(Position.Z - Entry.Value.Z) < 220) { Best = Distance; Result = Entry.Key; }
    }
    return Result;
}

FVector AHALVETHRealmWorld::GetReadablePosition(int32 Book) const
{
    const FVector* Position = ReadablePositions.Find(Book);
    return Position ? *Position : FVector::ZeroVector;
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
    TickSoil(DeltaSeconds);TickEarthSky(DeltaSeconds);
    Elapsed += DeltaSeconds;
    const auto* Player=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    for(int32 I=0;I<GuideBodies.Num();I++) {
        auto* Body=GuideBodies[I].Get(); if(!Body)continue;
        auto* Motion=Cast<UHALVETHGuideAnimInstance>(Body->GetAnimInstance());if(!Motion)continue;
        if(Body->GetBoneIndex(TEXT("spine02"))!=INDEX_NONE) {
            const FQuat Bone=Body->GetBoneQuaternion(TEXT("spine02"),EBoneSpaces::ComponentSpace);
            if(GuideBoneBaseline[I].Equals(FQuat::Identity))GuideBoneBaseline[I]=Bone;
            else if(GuideBoneBaseline[I].AngularDistance(Bone)>.0005f)GuidePoseChanged[I]=true;
        }
        const float Phase=FMath::Fmod(Elapsed+I*1.37f,4.8f);
        const float Blink=Phase<.09f?Phase/.09f:Phase<.18f?( .18f-Phase)/.09f:0;
        Body->SetMorphTarget(TEXT("Blink"),Blink);
        auto* GuideActor=GuideActors.IsValidIndex(I)?GuideActors[I].Get():nullptr;if(!GuideActor)continue;
        auto* Movement=GuideActor->GetCharacterMovement();
        auto& Phys=GuideBodyStates[I];auto& Genes=GuideProfiles[I];
        Phys.advance(Genes,Movement->IsMovingOnGround()?Movement->Velocity.Size2D():0,DeltaSeconds);
        Movement->MaxWalkSpeed=Genes.paceCmS*Phys.musclePower;
        Movement->MaxAcceleration=260*Phys.musclePower;
        Motion->BodyBreathPhase=Phys.breathPhase;Motion->BodyHeartPhase=Phys.heartPhase;
        Motion->BodyLoad=1-Phys.oxygenSupply;Motion->ResponseSeconds=Phys.reactionSeconds;
        Motion->CruiseSpeed=Genes.paceCmS;BodyLineages[GuideIdentities[I]].State=Phys;
        const FVector Before=Body->GetComponentLocation();
        GuideSpatialTravel[I]=FMath::Max(GuideSpatialTravel[I],FVector::DistXY(Before,GetActorLocation()+GuideOrigins[I]));
        FVector Difference=Player?Player->GetActorLocation()-Body->GetComponentLocation():FVector(0,-5000,0);
        const bool Near=Difference.SizeSquared2D()<FMath::Square(360.f);
        const bool Speaking=Near&&Player&&Player->GetAdventure()&&Body->ComponentTags.Num()>0
            &&Player->GetAdventure()->IsSpeakingTo(Body->ComponentTags[0]);
        // Ordinary proximity permits movement. A conversation pauses it;
        // development still portraits explicitly hold their nearby subjects.
        const bool PortraitHold=Near&&FParse::Param(FCommandLine::Get(),TEXT("HalvethVisual"));
        if(!Speaking&&!PortraitHold) {
            if(Phys.muscleReserve<.22&&Elapsed>=GuideRestUntil[I]){
                GuideRestUntil[I]=Elapsed+12;GuideDecisionAt[I]=GuideRestUntil[I];
                UE_LOG(LogTemp,Display,TEXT("GARDEN_GUIDE_INTENT identity=%d event=muscle_recovery time_s=%.5f until_s=%.5f reserve=%.5f"),GuideIdentities[I],Elapsed,GuideRestUntil[I],Phys.muscleReserve);
                GrowBodyLineage(GuideIdentities[I],TEXT("muscle_recovery"));Genes=BodyLineages[GuideIdentities[I]].Profile;
            }
            FCollisionQueryParams Query(SCENE_QUERY_STAT(GuidePatrol),true);Query.AddIgnoredActor(GuideActor);
            auto& Choice=GuideChoices[I];
            if(GuideDecisionCount[I]>0&&Elapsed>=GuideRestUntil[I]&&FVector::Dist2D(Before,GuideGoals[I])<35&&Elapsed<GuideDecisionAt[I]){
                GuideRestUntil[I]=Elapsed+Choice.FRandRange(2.0,4.5);
                GuideDecisionAt[I]=GuideRestUntil[I];
                UE_LOG(LogTemp,Display,TEXT("GARDEN_GUIDE_INTENT identity=%d event=rest time_s=%.5f until_s=%.5f"),GuideIdentities[I],Elapsed,GuideRestUntil[I]);
                GrowBodyLineage(GuideIdentities[I],TEXT("rest"));Genes=BodyLineages[GuideIdentities[I]].Profile;
            }
            if(Elapsed>=GuideDecisionAt[I]&&Elapsed>=GuideRestUntil[I]){
                bool Chosen=false;FString Kind=TEXT("explore");
                for(int Attempt=0;Attempt<12&&!Chosen;Attempt++){
                    const double Angle=Choice.FRandRange(-PI,PI),Distance=Choice.FRandRange(450,800);
                    FVector Goal=Before+FVector(FMath::Cos(Angle)*Distance,FMath::Sin(Angle)*Distance,0);
                    // Curiosity can turn a later walk towards an actual book or portal.
                    if(Attempt==0&&GuideDecisionCount[I]>0&&Choice.FRand()<.2+.35*Genes.curiosity){
                        if(Choice.FRand()<.5&&Portals.Num()>0){Goal=Portals[Choice.RandRange(0,Portals.Num()-1)].Position+GetActorLocation()+FVector(80,-100,0);Kind=TEXT("visit_portal");}
                        else if(ReadablePositions.Num()>0){TArray<int32> Books;ReadablePositions.GetKeys(Books);Books.Sort();Goal=ReadablePositions[Books[Choice.RandRange(0,Books.Num()-1)]]+GetActorLocation()+FVector(80,-100,0);Kind=TEXT("visit_book");}
                    }else Kind=TEXT("explore");
                    const FVector LocalGoal=Goal-GetActorLocation();
                    if(FMath::Abs(LocalGoal.X)>27500||FMath::Abs(LocalGoal.Y)>27500||FVector::Dist2D(Goal,Before)<200)continue;
                    FHitResult Ground;
                    if(GetWorld()->LineTraceSingleByChannel(Ground,Goal+FVector(0,0,1500),Goal-FVector(0,0,1500),ECC_Visibility,Query)&&Ground.ImpactNormal.Z>.75){
                        Goal.Z=Ground.ImpactPoint.Z;GuideGoals[I]=Goal;Chosen=true;
                    }
                }
                if(Chosen){
                    GuideDecisionCount[I]++;GuideDecisionAt[I]=Elapsed+Choice.FRandRange(12,19);GuideBlockedTime[I]=0;
                    UE_LOG(LogTemp,Display,TEXT("GARDEN_GUIDE_INTENT identity=%d event=choose sequence=%d kind=%s time_s=%.5f target=%s"),GuideIdentities[I],GuideDecisionCount[I],*Kind,Elapsed,*GuideGoals[I].ToString());
                    GrowBodyLineage(GuideIdentities[I],Kind);Genes=BodyLineages[GuideIdentities[I]].Profile;
                }else {GuideDecisionAt[I]=Elapsed+1;GuideRestUntil[I]=Elapsed+1;}
            }
            FVector Direction=GuideGoals[I]-Before;Direction.Z=0;
            auto SafeDirection=[&](FVector Candidate) {
                const FVector Next=Before+Candidate*85;
                FHitResult Support;
                if(!GetWorld()->LineTraceSingleByChannel(Support,Next+FVector(0,0,150),Next-FVector(0,0,250),ECC_Visibility,Query)
                    ||Support.ImpactNormal.Z<.70||Support.ImpactPoint.Z-Before.Z>Movement->MaxStepHeight)return false;
                const FVector Centre=GuideActor->GetActorLocation();
                const auto* Capsule=GuideActor->GetCapsuleComponent();
                FHitResult Obstacle;
                return !GetWorld()->SweepSingleByChannel(Obstacle,Centre,Centre+Candidate*85,FQuat::Identity,ECC_Pawn,
                    FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight()-3),Query);
            };
            bool Advanced=false;
            if(Elapsed>=GuideRestUntil[I]&&Direction.Size2D()>35) {
                const FVector Forward=Direction.GetSafeNormal();
                for(double Turn:{0.,60.,-60.}) {
                    const FVector Candidate=Forward.RotateAngleAxis(Turn,FVector::UpVector);
                    if(SafeDirection(Candidate)) {GuideActor->AddMovementInput(Candidate,FMath::Clamp(Direction.Size2D()/85,0.,1.),true);Advanced=true;break;}
                }
            }
            GuideBlockedTime[I]=Advanced?0:GuideBlockedTime[I]+DeltaSeconds;
            if(Elapsed>=GuideRestUntil[I]&&GuideBlockedTime[I]>1.5){GuideDecisionAt[I]=Elapsed;GuideBlockedTime[I]=0;}
        }
        const FVector Velocity=Movement->Velocity;
        FRotator Facing=GuideActor->GetActorRotation();
        if(Velocity.SizeSquared2D()>25)Facing.Yaw=Velocity.Rotation().Yaw;
        else if(Near){
            FVector Local=Body->GetComponentTransform().InverseTransformVectorNoScale(Difference);
            double Angle=FMath::Atan2(FVector::DotProduct(Local,Motion->Right),FVector::DotProduct(Local,Motion->Forward));
            if(FMath::Abs(Angle)>.55)Facing.Yaw=Difference.Rotation().Yaw;
        }
        GuideActor->SetActorRotation(FMath::RInterpTo(GuideActor->GetActorRotation(),Facing,DeltaSeconds,2.3));
        // Translation drives strides. Turning towards a speaker must not leave
        // a planted walking foot behind while the capsule rotates in place.
        Motion->DesiredSpeed=Movement->IsMovingOnGround()?Velocity.Size2D():0;
        Motion->Speaking=Speaking;
        FVector EyeTarget=Player?Player->GetActorLocation()+FVector(0,0,55)-Body->GetComponentLocation():FVector(0,0,0);
        EyeTarget=Body->GetComponentTransform().InverseTransformVectorNoScale(EyeTarget);
        EyeTarget.Z-=165;
        Motion->DesiredLookYaw=Near?FMath::Clamp(FMath::Atan2(FVector::DotProduct(EyeTarget,Motion->Right),FVector::DotProduct(EyeTarget,Motion->Forward)),-.7,.7):.18*FMath::Sin(Elapsed*.41+I);
        Motion->DesiredLookPitch=Near?FMath::Clamp(FMath::Atan2(EyeTarget.Z,EyeTarget.Size2D()),-.22,.22):.03*FMath::Sin(Elapsed*.61+I);
        Body->SetMorphTarget(TEXT("Talk"),Speaking?.5f*FMath::Max(0.f,FMath::Sin(Elapsed*5.2f+I)):0.f);
        FVector Hand=Body->GetBoneLocation(TEXT("wrist_R"),EBoneSpaces::ComponentSpace);
        FQuat Head=Body->GetBoneQuaternion(TEXT("head"),EBoneSpaces::ComponentSpace);
        if(GuideHandBaseline[I].IsNearlyZero()){GuideHandBaseline[I]=Hand;GuideHeadBaseline[I]=Head;}
        else {GuideHandTravel[I]=FMath::Max(GuideHandTravel[I],FVector::Dist(Hand,GuideHandBaseline[I]));GuideHeadTravel[I]=FMath::Max(GuideHeadTravel[I],Head.AngularDistance(GuideHeadBaseline[I]));}
    }
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
