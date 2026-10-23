// Copyright (c) 2026 Juri Janovski. MIT.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstralGeometry.h"
#include "HALVETHAstralSanctum.generated.h"
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class AHALVETHCharacter;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/** A real, colliding addition to Portal Garden, assembled from closed solids. */
UCLASS()
class HALVETHREALMS_API AHALVETHAstralSanctum : public AActor {
    GENERATED_BODY()
public:
    AHALVETHAstralSanctum();
    void Build(uint32 Seed);
    virtual void Tick(float Dt) override;
    bool Interact(AHALVETHCharacter* Player);
    void CastFocus(AHALVETHCharacter* Player);
    bool IsAttuned() const {return Gate.aligned==3;}
    bool VerifyStructure() const;
    FVector Entrance() const {return GetActorLocation()+FVector(0,-1650,100);}
    FVector FocusPoint() const {return GetActorLocation()+FVector(0,900,500);}
    bool RunProbe(bool RequireTraversal=false);
private:
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Parts;
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Rings;
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Doors;
    UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Lamps;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Focus;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> FlightTrail;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Aperture;
    UPROPERTY() TObjectPtr<USceneCaptureComponent2D> PortalCamera;
    UPROPERTY() TObjectPtr<UTextureRenderTarget2D> PortalTarget;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FocusMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> RuneMaterial;
    Astral::Gate Gate;
    double Time=0,FocusRemaining=0,TeleportCooldown=0;
    int32 SolidTriangles=0;
    int32 CaptureStep=0;
    bool ProbeFinished=false;
    bool ProbeGroundPassed=false;
    FVector ProbeFlightStart;
    bool CaptureFailed=false;
    double LastFrameAt=0;
    TArray<double> CaptureFrameMilliseconds;
    TArray<FVector> TrailPoints;
    TArray<double> TrailTimes;
    double TrailSampleAt=0;
    double PortalSampleAt=0;
    int32 PortalFrames=0;
    void Capture();
    void UpdateTrail();
    void UpdateAperture();
    void UpdateGeometry(UProceduralMeshComponent* Component,const Astral::Mesh& Data,bool Solid);
    UProceduralMeshComponent* Mesh(const Astral::Mesh& Data,const FString& Name,
        FLinearColor Tint,float Metallic,float Roughness,float Glow,bool Solid);
    void Lamp(FVector At,FLinearColor Color,float Intensity,float Radius);
    void Sign(FVector At,const FString& Text,float Size);
    bool Transport(AHALVETHCharacter* Player,bool Return);
};
