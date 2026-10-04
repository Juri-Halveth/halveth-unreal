#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HALVETHAdventureComponent.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class UDamageType;
class AController;

USTRUCT()
struct FHALVETHSparkFlight
{
    GENERATED_BODY()
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<UPointLightComponent> Light;
    FVector Position = FVector::ZeroVector;
    FVector Velocity = FVector::ZeroVector;
    float Remaining = 0;
    bool bImpact = false;
};

/** Local adventure state. Dialogues are authored text; no model or network calls. */
UCLASS(ClassGroup=(HALVETH), meta=(BlueprintSpawnableComponent))
class HALVETHREALMS_API UHALVETHAdventureComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHALVETHAdventureComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="HALVETH|Adventure") void CycleAbility();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Adventure") bool ActivateAbility();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Adventure") void CycleItem();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Adventure") bool UseItem();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Adventure") bool Dodge();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Adventure") bool InteractWithNearbyCharacter();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Adventure") float ReceiveDamage(float RawDamage);
    UFUNCTION(BlueprintCallable, Category="HALVETH|Adventure") void ClearTransientEffects();
    bool GrantConsumable(int32 Index, int32 Amount);
    bool RestoreConsumables(int32 HealthItems, int32 ManaItems, int32 StaminaItems);
    float GetMaxMana() const;
    float GetSelectedAbilityCost() const;
    bool IsSpeakingTo(FName Identity) const;

    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") float GetHealth() const { return Health; }
    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") float GetMana() const { return Mana; }
    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") float GetStamina() const { return Stamina; }
    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") bool IsShieldActive() const { return ShieldRemaining > 0; }
    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") float GetShieldSeconds() const { return ShieldRemaining; }
    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") FString GetStatusText() const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") FString GetAbilityText() const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") FString GetInventoryText() const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") FString GetInteractionText() const { return InteractionText; }
    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") int32 GetSelectedAbility() const { return SelectedAbility; }
    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") int32 GetSelectedItem() const { return SelectedItem; }
    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") int32 GetItemCount(int32 Index) const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Adventure") int32 GetSparkImpactCount() const { return SparkImpacts; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Aura;
    UPROPERTY() TObjectPtr<UPointLightComponent> AuraLight;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> AuraMaterial;
    UPROPERTY() TArray<FHALVETHSparkFlight> Sparks;
    float Health = 100;
    float Mana = 100;
    float Stamina = 100;
    float AbilityCooldown = 0;
    float DodgeCooldown = 0;
    float ShieldRemaining = 0;
    float LoveRemaining = 0;
    float Elapsed = 0;
    int32 SelectedAbility = 0;
    int32 SelectedItem = 0;
    int32 SparkImpacts = 0;
    int32 ItemCounts[3] = {3, 2, 2};
    FString InteractionText = TEXT("E near a guide: authored stories. Choose a spell or an inventory item.");
    TMap<FName, int32> ConversationSteps;
    FName SpeakingIdentity = NAME_None;
    double SpeakingUntil = 0;

    UFUNCTION() void OnOwnerDamaged(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
        AController* InstigatedBy, AActor* DamageCauser);
    bool LaunchSpark();
    void UpdateSparks(float DeltaTime);
    void SetFeedback(const FString& Message);
    static FString AbilityName(int32 Index);
    static FString ItemName(int32 Index);
    static FName FindDialogueTag(const TArray<FName>& Tags);
    static FString AuthoredDialogue(FName Identity, int32 Step);
};
