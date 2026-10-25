#pragma once
// SPDX-License-Identifier: MIT
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GuideMotionMath.h"
#include "GazeMotion.h"
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
    void LogPoseAudit(const TCHAR* Context) const;
    void ObserveLookTarget(const FVector& WorldTarget,bool Eligible,bool ViewerLooksAtFace);
    UPROPERTY() TObjectPtr<UAnimSequence> BaseIdle;
    float DesiredSpeed=0,DesiredLookYaw=0,DesiredLookPitch=0;
    bool Speaking=false;
    bool FreezeAppearancePose=false;
    int32 Identity=1;
    double Time=0,GaitPhase=0;
    float WalkWeight=0,LookYaw=0,LookPitch=0,EyeYaw=0,EyePitch=0,Gesture=0;
    bool VisibleLookTarget=false,ViewerEyeContact=false;
    double NextSightCheck=0;
    FVector RefGazeOrigin=FVector::ZeroVector,LastSightEndpoint=FVector::ZeroVector;
    bool SightClear=false;
    HalvethGaze::State Gaze;
    double BodyBreathPhase=0,BodyHeartPhase=0,BodyLoad=0,ResponseSeconds=.18;
    double CruiseSpeed=84;
    FVector EvaluatedFootTargets[2];
    bool PoseAuditReady=false;
    FVector Forward=FVector::ForwardVector,Right=FVector::RightVector;
    FVector RefFeet[2];
    FVector FootTargets[2],PlantWorld[2],GroundNormals[2]={FVector::UpVector,FVector::UpVector};
    bool Planted[2]={false,false};
    double PlantDriftMax=0;
    int32 PlantSamples=0;
    float GroundCorrections[2]={0,0};
    double HandForward[2]={3,3},WeightShift=0;
    HalvethMotion::Spring WalkSpring,GestureSpring;
    HalvethMotion::Spring HandSprings[2],WeightShiftSpring;
};
