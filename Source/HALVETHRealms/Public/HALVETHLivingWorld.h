// MIT. A finite, local population with authored routines and a non-criminal warning response.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LivingWorldModel.h"
#include "HALVETHLivingWorld.generated.h"
class AHALVETHCitizen;
class AHALVETHCharacter;
class AHALVETHRealmWorld;
class UTextRenderComponent;
class UStaticMeshComponent;
USTRUCT()
struct FHALVETHResidentBody {
    GENERATED_BODY()
    int32 Id=0;FVector Goal=FVector::ZeroVector,Origin=FVector::ZeroVector,Steering=FVector::ZeroVector;
    double Travel=0,TalkUntil=0,Blocked=0;
    double OutsideViewSince=-1;
    UPROPERTY() TObjectPtr<AHALVETHCitizen> Actor;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
};
UCLASS()
class HALVETHREALMS_API AHALVETHLivingWorld : public AActor {
    GENERATED_BODY()
public:
    AHALVETHLivingWorld();
    void Initialize(AHALVETHRealmWorld* World,uint32 Seed);
    void PrepareRealmChange();
    virtual void Tick(float Dt) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    bool Interact(AHALVETHCharacter* Player);
    FString GetStatus() const;
    int32 VisibleResidents() const {return Bodies.Num();}
    uint32 ContinuityErrors() const {return ContinuityFailures;}
    bool Perceives(const AHALVETHCitizen* Observer,const AHALVETHCharacter* Player) const;
private:
    UPROPERTY() TObjectPtr<AHALVETHRealmWorld> Realm;
    UPROPERTY() TArray<FHALVETHResidentBody> Bodies;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> WitnessWall;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> CivicBeam;
    std::vector<HalvethLife::Resident> Residents;
    HalvethLife::Notice Notice;
    int32 LastRealm=INDEX_NONE,BodyBudget=18,ActiveGuard=INDEX_NONE;
    double Time=0,ScheduleOrigin=0,BrainAt=0,PopulationAt=0,SpeakingUntil=0,SampleAt=0,BeamUntil=0;
    uint32 BackgroundCursor=0;
    uint32 ContinuityFailures=0;
    bool InitialPopulationPending=true;
    bool Probe=false,CaptureMode=false,ProbeFinished=false,ProbeOcclusion=false,ProbeFreeFlight=false,ProbeIndoor=false,ProbeSawPursuit=false;
    int32 ProbeStep=0,CaptureStep=0;
    double ProbeAt=0,HoldStarted=0,MeasuredActorMs=0,MeasuredBackgroundMs=0;
    float ProbeHealth=0;
    FVector ProbeFlightStart=FVector::ZeroVector,ProbeChaseStart=FVector::ZeroVector;
    TArray<double> FrameTimes;
    double LastFrame=0;
    void RefreshBodies();
    bool SavePhysicalState(const FHALVETHResidentBody& Body,bool Retiring);
    void AdvanceBodies(float Dt);
    void UpdateNotice(AHALVETHCharacter* Player);
    void ApplyNotice(HalvethLife::Event Event,AHALVETHCharacter* Player);
    void RunProbe();
    void Capture();
    void Shot(const TCHAR* Name);
    void FinishProbe(bool Passed,const TCHAR* Reason);
    FHALVETHResidentBody* Guard();
    bool Supported(FVector& Location,const AActor* Ignore=nullptr) const;
    void Steer(FHALVETHResidentBody& Body,FVector Goal,float Dt);
    FString ResidentName(int32 Id) const;
    FString TaskName(HalvethLife::Task Task) const;
};
