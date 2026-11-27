#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VoidCharacter.generated.h"
UCLASS()
class HALVETHVOID_API AVoidCharacter : public ACharacter {
    GENERATED_BODY()
public:
    AVoidCharacter();
    UPROPERTY() TObjectPtr<class UCameraComponent> Camera;
    bool bInventory = false;
    void Inventory();
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
private:
    void Forward(float V); void Right(float V); void LookX(float V); void LookY(float V);
    void Sprint(); void Walk(); void Quit();
};
