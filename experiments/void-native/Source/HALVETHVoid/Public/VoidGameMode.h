#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VoidSoul.h"
#include "VoidGameMode.generated.h"
UCLASS()
class HALVETHVOID_API AVoidGameMode : public AGameModeBase {
    GENERATED_BODY()
public:
    AVoidGameMode();
    virtual void StartPlay() override;
    virtual void Tick(float Delta) override;
    FVoidSoul Soul;
private:
    UPROPERTY() TObjectPtr<class AVoidWorld> WorldBuilder;
    bool bTest=false, bVisual=false;
    double TestTime=0;
    int32 TestStage=0, SupportPass=0, SupportFail=0;
    FVector WalkOrigin;
    TArray<FString> Failures;
    void TestStep();
};
