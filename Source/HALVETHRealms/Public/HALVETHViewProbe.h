// MIT. Isolated native camera regression and rendered night review.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HALVETHViewProbe.generated.h"

UCLASS()
class HALVETHREALMS_API AHALVETHViewProbe : public AActor {
    GENERATED_BODY()
public:
    AHALVETHViewProbe();
    virtual void Tick(float DeltaSeconds) override;
private:
    int32 Frame=0,Phase=0,Zenith=0,Nadir=0,CaptureStep=0;
    double Elapsed=0,Arc=0,PitchArc=0,MaxStep=0,MinBodyUp=1;
    bool WasUp=false,WasDown=false,HavePrevious=false,LegacyRejected=false,Finished=false;
    FQuat Previous=FQuat::Identity;
    void Probe();
    void Capture();
    void Finish(bool Passed,const TCHAR* Detail);
};
