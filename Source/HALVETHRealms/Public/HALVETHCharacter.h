#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HALVETHCharacter.generated.h"

class UCameraComponent;
class UHALVETHAdventureComponent;

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
    UHALVETHAdventureComponent* GetAdventure() const { return Adventure; }
private:
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    UPROPERTY() TObjectPtr<UHALVETHAdventureComponent> Adventure;
    bool bHelpVisible = true;
    bool bCreditsVisible = false;
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
    void Quit();
};
