#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HALVETHKnowledgeComponent.generated.h"

struct FHALVETHKnowledgeRuntime;
struct FHALVETHKnowledgeRuntimeDeleter
{
    void operator()(FHALVETHKnowledgeRuntime* Pointer) const;
};

/** Authored books, learned recipes and local persistent constructions. */
UCLASS(ClassGroup=(HALVETH), meta=(BlueprintSpawnableComponent))
class HALVETHREALMS_API UHALVETHKnowledgeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHALVETHKnowledgeComponent();
    virtual ~UHALVETHKnowledgeComponent() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    void InitializeWorld(uint32 Seed, int32 Realm);
    void OnRealmChanged(int32 Realm);
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") void ToggleReading();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") void OpenBook(int32 Index);
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") void NextBook();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") void NextPage();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") void PreviousPage();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") bool RecallPage(int32 Choice);
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") void CycleRecipe();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") bool CraftSelected();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") bool PackDraught();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") void CycleStructure();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") bool BuildSelected();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") void CycleQuest();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") bool ChooseQuest(int32 Choice);
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") bool GatherNearby();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") bool SaveProgress();
    UFUNCTION(BlueprintCallable, Category="HALVETH|Knowledge") bool ReloadProgress();
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") TArray<FString> GetPanelLines() const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") TArray<FString> GetBookLines() const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") TArray<FString> GetWorkshopLines(int32 Panel = 0) const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") FString GetConstructionHint() const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") bool IsReading() const { return bReading; }
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") FString GetStatusText() const { return StatusText; }
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") float GetSpellCostMultiplier() const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") float GetMaxManaBonus() const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") int32 GetBuiltCount() const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") int32 GetVisibleBuiltCount() const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") int32 GetKnowledgeRank() const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") int32 GetMaterialCount(int32 Index) const;
    UFUNCTION(BlueprintPure, Category="HALVETH|Knowledge") int32 GetActiveReadingMilliseconds() const;

protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    TUniquePtr<FHALVETHKnowledgeRuntime, FHALVETHKnowledgeRuntimeDeleter> Runtime;
    UPROPERTY() TArray<TObjectPtr<AActor>> BuiltActors;
    uint32 WorldSeed = 127;
    int32 CurrentRealm = 0;
    int32 ActiveBook = 0;
    int32 ActivePage = 0;
    int32 SelectedRecipe = 0;
    int32 SelectedStructure = 0;
    int32 SelectedQuest = 0;
    float AutosaveElapsed = 0;
    bool bInitialized = false;
    bool bReading = false;
    bool bDirty = false;
    bool bSmokeSlot = false;
    bool bPreserveInvalidSave = false;
    FString StatusText = TEXT("Open the library to discover original books and practical recipes.");

    FString SavePath() const;
    void Feedback(const FString& Text);
    void RefreshBuiltActors();
    AActor* SpawnStructure(int32 Kind, FVector Location, float Yaw);
};
