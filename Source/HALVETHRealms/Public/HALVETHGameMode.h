#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HALVETHGameMode.generated.h"

class AHALVETHRealmWorld;

UCLASS(Config=Game)
class HALVETHREALMS_API AHALVETHGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AHALVETHGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(Config, EditAnywhere, Category="HALVETH") int32 WorldSeed = 127;
    AHALVETHRealmWorld* GetRealmWorld() const { return RealmWorld; }
    bool Travel(int32 Destination);
    void Interact(APawn* Pawn);
    void CastLove();
    void ApplyQuality(int32 Quality, bool Persist = true);
    int32 GetQuality() const { return QualityIndex; }
    FString GetMessage() const { return Message; }
    float GetMessageRemaining() const { return MessageRemaining; }
    void Notify(const FString& Text);
private:
    UPROPERTY() TObjectPtr<AHALVETHRealmWorld> RealmWorld;
    int32 QualityIndex = 1;
    float MessageRemaining = 0;
    FString Message;
    bool bSmokeTest = false;
    bool bSmokeFailed = false;
    int32 SmokeStep = 0;
    int32 SmokeReadingMilliseconds = 0;
    float SmokeElapsed = 0;
    float TravelCooldown = 0;
    void RunSmokeStep();
    void FinishSmoke(bool Success, const FString& Detail);
    bool bVisualTest = false;
    float VisualElapsed = 0;
    int32 VisualStep = 0;
    bool bVisualFailed = false;
    void RunVisualStep();
    bool bMotionCapture=false;
    double MotionElapsed=0,MotionNextFrame=0;
    int32 MotionFrame=0,MotionStage=-1;
    FString MotionTiming;
    void RunMotionCapture(float DeltaSeconds);
    double MotionDropStartZ=0,MotionDropMinVz=0,MotionDropLandingError=0;
    bool MotionDropFalling=false,MotionDropLanded=false;
};
