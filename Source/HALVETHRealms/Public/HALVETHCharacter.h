#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HALVETHCharacter.generated.h"

class UCameraComponent;
class UHALVETHAdventureComponent;
class UHALVETHKnowledgeComponent;

UCLASS()
class HALVETHREALMS_API AHALVETHCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AHALVETHCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    void ResetToSpawn();
    bool IsHelpVisible() const { return bHelpVisible; }
    bool IsCreditsVisible() const { return bCreditsVisible; }
    int32 GetReaderPanel() const { return ReaderPanel; }
    void SetReaderPanel(int32 Panel) { ReaderPanel = FMath::Clamp(Panel, 0, 3); }
    UHALVETHAdventureComponent* GetAdventure() const { return Adventure; }
    UHALVETHKnowledgeComponent* GetKnowledge() const { return Knowledge; }
private:
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    UPROPERTY() TObjectPtr<UHALVETHAdventureComponent> Adventure;
    UPROPERTY() TObjectPtr<UHALVETHKnowledgeComponent> Knowledge;
    bool bHelpVisible = false;
    bool bCreditsVisible = false;
    int32 ReaderPanel = 0;
    void Forward(float Value);
    void Right(float Value);
    void Turn(float Value);
    void Look(float Value);
    void SprintOn();
    void SprintOff();
    void Interact();
    void Love();
    void CastSpell();
    void NextSpell();
    void NextItem();
    void UseItem();
    void Dodge();
    void ReturnHome();
    void QualityLow();
    void QualityBalanced();
    void QualityEpic();
    void ToggleHelp();
    void ToggleCredits();
    bool IsReading() const;
    void StartJump();
    void ToggleLibrary();
    void CycleReaderPanel();
    void NextBook();
    void NextPage();
    void PreviousPage();
    void CycleRecipe();
    void CraftSelected();
    void PackDraught();
    void CycleStructure();
    void BuildSelected();
    void CycleQuest();
    void QuestOne();
    void QuestTwo();
    void QuestThree();
    void SaveKnowledge();
    void LoadKnowledge();
    void Quit();
};
