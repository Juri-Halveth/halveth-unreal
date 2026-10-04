#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GuideMotionMath.h"
#include "HALVETHGuideAnimInstance.generated.h"

class UAnimSequence;
UCLASS(Transient)
class HALVETHREALMS_API UHALVETHGuideAnimInstance : public UAnimInstance {
    GENERATED_BODY()
public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
    UPROPERTY() TObjectPtr<UAnimSequence> BaseIdle;
    float DesiredSpeed=0,DesiredLookYaw=0,DesiredLookPitch=0;
    bool Speaking=false;
    int32 Identity=1;
    double Time=0,GaitPhase=0;
    float WalkWeight=0,LookYaw=0,LookPitch=0,Gesture=0;
    FVector Forward=FVector::ForwardVector,Right=FVector::RightVector;
    FVector RefFeet[2];
    FVector FootTargets[2],PlantWorld[2],GroundNormals[2]={FVector::UpVector,FVector::UpVector};
    bool Planted[2]={false,false};
    double PlantDriftMax=0;
    int32 PlantSamples=0;
    float GroundCorrections[2]={0,0};
    HalvethMotion::Spring WalkSpring,YawSpring,PitchSpring,GestureSpring,ClothSpring;
};
