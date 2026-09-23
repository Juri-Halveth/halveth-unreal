#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HALVETHRealmWorld.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class USkyAtmosphereComponent;
class UPostProcessComponent;

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
    static FString RealmName(int32 Realm);
    static FString RealmDescription(int32 Realm);

private:
    UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> CylinderMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> ConeMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> SurfaceMaterial;
    UPROPERTY() TObjectPtr<UDirectionalLightComponent> Sun;
    UPROPERTY() TObjectPtr<USkyLightComponent> Sky;
    UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> Fog;
    UPROPERTY() TObjectPtr<USkyAtmosphereComponent> Atmosphere;
    UPROPERTY() TObjectPtr<UPostProcessComponent> PostProcess;
    UPROPERTY() TArray<TObjectPtr<UActorComponent>> Generated;
    UPROPERTY() TArray<FHALVETHPortal> Portals;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Fireflies;
    UPROPERTY() TArray<FVector> FireflyOrigins;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LoveOrb;
    UPROPERTY() TObjectPtr<UPointLightComponent> LoveLight;
    UPROPERTY() TObjectPtr<AActor> TrainingTarget;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> GuideHeads;
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
    void Readable(FVector Position, int32 Book);
};
