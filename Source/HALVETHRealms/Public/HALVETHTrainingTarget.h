#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HALVETHTrainingTarget.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

/** A reusable, nonliving crystal for practising spell impact; not a combatant. */
UCLASS()
class HALVETHREALMS_API AHALVETHTrainingTarget : public AActor
{
    GENERATED_BODY()
public:
    AHALVETHTrainingTarget();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;
    UFUNCTION(BlueprintPure, Category="HALVETH|Training") float GetHealth() const { return Health; }
    UFUNCTION(BlueprintPure, Category="HALVETH|Training") int32 GetBreakCount() const { return BreakCount; }
    UFUNCTION(BlueprintPure, Category="HALVETH|Training") bool IsBroken() const { return RespawnRemaining > 0; }
    UFUNCTION(BlueprintCallable, Category="HALVETH|Training") void ResetTarget();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Crystal;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LowerCrystal;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Fragments;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY() TObjectPtr<UPointLightComponent> Glow;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
    FVector Home = FVector::ZeroVector;
    float Health = 100;
    float RespawnRemaining = 0;
    float HitFlash = 0;
    float Elapsed = 0;
    int32 BreakCount = 0;

    void UpdateAppearance();
};
