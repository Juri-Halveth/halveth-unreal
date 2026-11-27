#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GuideBodyModel.h"
#include "HALVETHRealmWorld.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class USkyAtmosphereComponent;
class UPostProcessComponent;
class USkeletalMeshComponent;
class AHALVETHGuideCharacter;
class UProceduralMeshComponent;
class UVolumetricCloudComponent;

struct FHALVETHBodyLineage {
    FString Digest,Genome,JournalPath;
    uint64 Generation=0;
    HalvethBody::Profile Profile;
    HalvethBody::State State;
};

USTRUCT()
struct FHALVETHPortal
{
    GENERATED_BODY()
    UPROPERTY() FVector Position = FVector::ZeroVector;
    UPROPERTY() int32 Destination = 0;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Veil;
    UPROPERTY() TObjectPtr<UPointLightComponent> Light;
};

UCLASS()
class HALVETHREALMS_API AHALVETHRealmWorld : public AActor
{
    GENERATED_BODY()
public:
    AHALVETHRealmWorld();
    virtual void Tick(float DeltaSeconds) override;
    void BuildRealm(int32 Realm, uint32 Seed);
    void BeginLovePulse();
    int32 FindPortal(const FVector& Position) const;
    int32 FindReadable(const FVector& Position) const;
    FVector GetReadablePosition(int32 Book) const;
    FVector GetPortalPosition(int32 Destination) const;
    int32 GetPortalCount() const { return Portals.Num(); }
    int32 GetCurrentRealm() const { return CurrentRealm; }
    float GetLoveStrength() const { return LoveRemaining / 4.0f; }
    bool VerifyLandscape();
    bool VerifyCharacters() const;
    bool VerifyGuidePatrol() const;
    FVector GetGuidePosition(int32 Identity) const;
    AHALVETHGuideCharacter* GetGuideCharacter(int32 Identity) const;
    bool VerifyGroundDynamics() const;
    bool VerifySkyDynamics() const;
    double GetSkyClockUnix() const { return SkyClockUnix; }
    int32 GetTerrainChecks() const { return TerrainChecks; }
    int32 GetTreeCount() const { return TreeCount; }
    int32 GetFernCount() const { return FernCount; }
    int32 GetRockCount() const { return RockCount; }
    static FString RealmName(int32 Realm);
    static FString RealmDescription(int32 Realm);

private:
    UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> CylinderMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> ConeMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> SurfaceMaterial;
    UPROPERTY() TObjectPtr<UDirectionalLightComponent> Sun;
    UPROPERTY() TObjectPtr<UDirectionalLightComponent> NightFill;
    UPROPERTY() TObjectPtr<USkyLightComponent> Sky;
    UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> Fog;
    UPROPERTY() TObjectPtr<USkyAtmosphereComponent> Atmosphere;
    UPROPERTY() TObjectPtr<UVolumetricCloudComponent> Clouds;
    UPROPERTY() TObjectPtr<UPostProcessComponent> PostProcess;
    UPROPERTY() TArray<TObjectPtr<UActorComponent>> Generated;
    UPROPERTY() TArray<FHALVETHPortal> Portals;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Fireflies;
    UPROPERTY() TArray<FVector> FireflyOrigins;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LoveOrb;
    UPROPERTY() TObjectPtr<UPointLightComponent> LoveLight;
    UPROPERTY() TObjectPtr<AActor> TrainingTarget;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> GuideHeads;
    UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> GuideBodies;
    UPROPERTY() TArray<TObjectPtr<AHALVETHGuideCharacter>> GuideActors;
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> SoilComponents;
    TArray<FVector> SoilCenters;
    TArray<TArray<FVector>> SoilVertices;
    TArray<TArray<FLinearColor>> SoilColors;
    TArray<double> SoilLoads;
    TArray<double> SoilDepths;
    float SoilUpdateTime=0;
    TArray<FVector> SunDirections;
    double SkyFirstUnix=0,SkyLastUnix=0,SkyClockUnix=0,SkyTimeScale=1;
    float SkyUpdateTime=0;
    double SkyMinHeight=1,SkyMaxHeight=-1;
    int32 SkyObservedSamples=0,SkyNightSamples=0,SkyDaySamples=0;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Stars;
    TArray<FVector> GuideOrigins;
    TArray<int32> GuideIdentities;
    TArray<FQuat> GuideBoneBaseline;
    TArray<bool> GuidePoseChanged;
    TArray<double> GuidePatrolTime;
    TArray<FRandomStream> GuideChoices;
    TArray<FVector> GuideGoals;
    TArray<double> GuideDecisionAt,GuideRestUntil,GuideBlockedTime;
    TArray<int32> GuideDecisionCount;
    TArray<HalvethBody::Profile> GuideProfiles;
    TArray<HalvethBody::State> GuideBodyStates;
    TMap<int32,FHALVETHBodyLineage> BodyLineages;
    FString BodyOrigin;
    TArray<double> GuideSpatialTravel;
    TArray<FVector> GuideHandBaseline;
    TArray<double> GuideHandTravel;
    TArray<FQuat> GuideHeadBaseline;
    TArray<double> GuideHeadTravel;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> TerrainComponent;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeTrunks;
    int32 TerrainChecks = 0;
    int32 TreeCount = 0;
    int32 FernCount = 0;
    int32 RockCount = 0;
    TMap<int32, FVector> ReadablePositions;
    int32 CurrentRealm = 0;
    float Elapsed = 0;
    float LoveRemaining = 0;
    FLinearColor Accent = FLinearColor(0.9f, 0.08f, 0.22f);

    UStaticMeshComponent* Shape(UStaticMesh* Mesh, FVector Position, FVector Scale,
        FLinearColor Color, float Glow = 0, bool Collision = true, FRotator Rotation = FRotator::ZeroRotator);
    UPointLightComponent* Light(FVector Position, FLinearColor Color, float Intensity, float Radius);
    void Portal(FVector Position, int32 Destination, FLinearColor Color);
    void Label(FVector Position, const FString& Text, FLinearColor Color, float Size = 28);
    void Guide(FVector Position, int32 Identity);
    void GrowBodyLineage(int32 Identity,const FString& Event);
    void Readable(FVector Position, int32 Book);
    void BuildLandscape(uint32 Seed);
    void BuildSoil(FVector Guide);
    void TickSoil(float Dt);
    void InitializeEarthSky();
    void TickEarthSky(float Dt);
};
