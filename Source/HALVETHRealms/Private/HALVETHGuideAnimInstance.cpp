// SPDX-License-Identifier: MIT
#include "HALVETHGuideAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/BoneReference.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BonePose.h"

namespace {
HalvethMotion::V M(FVector V){return{V.X,V.Y,V.Z};}
FVector U(HalvethMotion::V V){return{V.x,V.y,V.z};}
struct FGuideProxy : FAnimInstanceProxy {
    explicit FGuideProxy(UAnimInstance* I):FAnimInstanceProxy(I){}
    UAnimSequence* Idle=nullptr;
    double Time=0,Phase=0;
    float Walk=0,Yaw=0,Pitch=0,Gesture=0;
    double HandForward[2]={3,3},WeightShift=0;
    FVector Forward,Right,Feet[2],Normals[2];float Ground[2]={0,0};
    virtual void PostEvaluate(UAnimInstance* I) override {
        FAnimInstanceProxy::PostEvaluate(I);
        auto* A=CastChecked<UHALVETHGuideAnimInstance>(I);
        for(int S=0;S<2;S++)A->EvaluatedFootTargets[S]=Feet[S];
        A->PoseAuditReady=true;
    }
    virtual void PreUpdate(UAnimInstance* I,float Dt) override {
        FAnimInstanceProxy::PreUpdate(I,Dt);
        auto* A=CastChecked<UHALVETHGuideAnimInstance>(I);
        Idle=A->BaseIdle;Time=A->Time;Phase=A->GaitPhase;Walk=A->WalkWeight;
        Yaw=A->LookYaw;Pitch=A->LookPitch;Gesture=A->Gesture;
        HandForward[0]=A->HandForward[0];HandForward[1]=A->HandForward[1];WeightShift=A->WeightShift;
        Forward=A->Forward;Right=A->Right;
        for(int S=0;S<2;S++){Feet[S]=A->FootTargets[S];Normals[S]=A->GroundNormals[S];Ground[S]=A->GroundCorrections[S];}
    }
    virtual bool Evaluate(FPoseContext& Output) override {
        Output.ResetToRefPose();
        if(!Idle)return true;
        FAnimationPoseData Data(Output);
        Idle->GetAnimationPose(Data,FAnimExtractContext(FMath::Fmod(Time,Idle->GetPlayLength()),false,{},true));
        FCSPose<FCompactPose> CS;CS.InitPose(Output.Pose);
        const auto& Bones=Output.Pose.GetBoneContainer();
        auto Index=[&](FName Name){FBoneReference R(FName(*Name.ToString().Replace(TEXT("."),TEXT("_"))));R.Initialize(Bones);return R.IsValidToEvaluate(Bones)?R.GetCompactPoseIndex(Bones):FCompactPoseBoneIndex(INDEX_NONE);};
        auto Rotate=[&](FName Name,FVector Axis,double Angle){
            auto B=Index(Name);if(B==INDEX_NONE)return;
            FTransform T=CS.GetComponentSpaceTransform(B);
            T.SetRotation((FQuat(Axis.GetSafeNormal(),Angle)*T.GetRotation()).GetNormalized());
            TArray<FBoneTransform> Changes;Changes.Emplace(B,T);CS.SafeSetCSBoneTransforms(Changes);
        };
        auto Shift=[&](FName Name,FVector Delta){
            auto B=Index(Name);if(B==INDEX_NONE)return;
            FTransform T=CS.GetComponentSpaceTransform(B);T.AddToTranslation(Delta);
            TArray<FBoneTransform> Changes;Changes.Emplace(B,T);CS.SafeSetCSBoneTransforms(Changes);
        };
        // Support height comes from the authored leg lengths and traced feet.
        // A permanent crouch offset previously compressed both knees at rest.
        const double Breath=FMath::Sin(Time*1.45),Sway=FMath::Sin(Time*.71);
        // Restrained lateral weight transfer; leg support is solved AFTER it.
        Shift(TEXT("root"),Right*WeightShift);
        double PelvisHeight=TNumericLimits<double>::Max();
        for(int S=0;S<2;S++){
            const FString Side=S==0?TEXT("L"):TEXT("R");
            auto Hip=Index(FName(*(TEXT("upperleg01.")+Side)));
            auto Knee=Index(FName(*(TEXT("lowerleg01.")+Side)));
            auto Ankle=Index(FName(*(TEXT("foot.")+Side)));
            if(Hip==INDEX_NONE||Knee==INDEX_NONE||Ankle==INDEX_NONE)continue;
            const FVector A=CS.GetComponentSpaceTransform(Hip).GetLocation();
            const FVector B=CS.GetComponentSpaceTransform(Knee).GetLocation();
            const FVector C=CS.GetComponentSpaceTransform(Ankle).GetLocation();
            const double Height=HalvethMotion::SupportedHipHeight(M(A),M(Feet[S]),FVector::Distance(A,B),FVector::Distance(B,C),FMath::DegreesToRadians(6.+12.*Walk));
            PelvisHeight=FMath::Min(PelvisHeight,Height-A.Z);
        }
        if(PelvisHeight!=TNumericLimits<double>::Max())Shift(TEXT("root"),FVector(0,0,PelvisHeight));
        Rotate(TEXT("spine03"),Forward,.009*Sway+.018*Walk*FMath::Sin(Phase*2*PI));
        Rotate(TEXT("spine02"),Right,.008*Breath+.025*Walk);
        Rotate(TEXT("spine01"),FVector::UpVector,.045*Walk*FMath::Sin(Phase*2*PI));
        const FVector YawAxis=FVector::CrossProduct(Forward,Right).GetSafeNormal();
        const FVector PitchAxis=FVector::CrossProduct(Forward,FVector::UpVector).GetSafeNormal();
        Rotate(TEXT("neck03"),YawAxis,Yaw*.30);
        Rotate(TEXT("head"),YawAxis,Yaw*.62);
        Rotate(TEXT("head"),PitchAxis,Pitch*.90);
        for(FName Eye:{FName(TEXT("eye.L")),FName(TEXT("eye.R"))}){
            Rotate(Eye,YawAxis,Yaw*.08);
            Rotate(Eye,PitchAxis,Pitch*.10);
        }
        auto Limb=[&](const FString& Upper,const FString& Lower,const FString& End,const FString& Side,FVector Goal,FVector Pole){
            FName Names[]={FName(*(Upper+TEXT("01.")+Side)),FName(*(Upper+TEXT("02.")+Side)),FName(*(Lower+TEXT("01.")+Side)),FName(*(Lower+TEXT("02.")+Side)),FName(*(End+TEXT(".")+Side))};
            FCompactPoseBoneIndex Id[5];FTransform Old[5];
            for(int J=0;J<5;J++){Id[J]=Index(Names[J]);if(Id[J]==INDEX_NONE)return;Old[J]=CS.GetComponentSpaceTransform(Id[J]);}
            FVector A=Old[0].GetLocation(),B=Old[2].GetLocation(),C=Old[4].GetLocation();
            auto Solved=HalvethMotion::Solve(M(A),M(Goal),M(Pole),FVector::Distance(A,B),FVector::Distance(B,C));
            FVector K=U(Solved.knee),E=U(Solved.end);
            FQuat Q1=FQuat::FindBetweenNormals((B-A).GetSafeNormal(),(K-A).GetSafeNormal());
            FQuat Q2=FQuat::FindBetweenNormals((C-B).GetSafeNormal(),(E-K).GetSafeNormal());
            TArray<FBoneTransform> Changes;
            for(int J=0;J<5;J++){
                FTransform T=Old[J];
                if(J<2){T.SetLocation(A+Q1.RotateVector(Old[J].GetLocation()-A));T.SetRotation((Q1*Old[J].GetRotation()).GetNormalized());}
                else if(J<4){T.SetLocation(K+Q2.RotateVector(Old[J].GetLocation()-B));T.SetRotation((Q2*Old[J].GetRotation()).GetNormalized());}
                else {T.SetLocation(E);if(Upper==TEXT("upperarm"))T.SetRotation((Q2*Old[J].GetRotation()).GetNormalized());}
                Changes.Emplace(Id[J],T);
            }
            Changes.Sort([](const FBoneTransform& A,const FBoneTransform& B){return A.BoneIndex<B.BoneIndex;});
            CS.SafeSetCSBoneTransforms(Changes);
        };
        for(int S=0;S<2;S++){
            FString Side=S==0?TEXT("L"):TEXT("R");double Sign=S==0?-1:1;
            FVector Goal=Feet[S];
            Limb(TEXT("upperleg"),TEXT("lowerleg"),TEXT("foot"),Side,Goal,Forward);
            auto Foot=Index(FName(*(TEXT("foot.")+Side)));if(Foot!=INDEX_NONE){
                FTransform T=CS.GetComponentSpaceTransform(Foot);
                T.SetRotation((FQuat::FindBetweenNormals(FVector::UpVector,Normals[S])*T.GetRotation()).GetNormalized());
                TArray<FBoneTransform> Changes;Changes.Emplace(Foot,T);CS.SafeSetCSBoneTransforms(Changes);
            }
            auto Hand=Index(FName(*(TEXT("wrist.")+Side)));
            auto Shoulder=Index(FName(*(TEXT("upperarm01.")+Side)));
            auto Elbow=Index(FName(*(TEXT("lowerarm01.")+Side)));
            if(Hand!=INDEX_NONE&&Shoulder!=INDEX_NONE&&Elbow!=INDEX_NONE){
                const FVector A=CS.GetComponentSpaceTransform(Shoulder).GetLocation();
                const FVector B=CS.GetComponentSpaceTransform(Elbow).GetLocation();
                const FVector C=CS.GetComponentSpaceTransform(Hand).GetLocation();
                double TalkWave=Gesture*(.65+.35*FMath::Sin(Time*2.0+S));
                const auto Offset=HalvethMotion::RelaxedHandOffset(FVector::Distance(A,B),FVector::Distance(B,C),HandForward[S],Sign*(3+5*TalkWave),TalkWave);
                // Normalize again after the rig-basis mapping: inferred forward
                // and right need not be perfectly orthogonal. Keep the reach.
                const FVector Direction=Forward*Offset.x+Right*Offset.y+FVector::UpVector*Offset.z;
                const FVector GoalHand=A+Direction.GetSafeNormal()*Offset.length();
                Limb(TEXT("upperarm"),TEXT("lowerarm"),TEXT("wrist"),Side,GoalHand,Right*(Sign*.7)-FVector::UpVector+Forward*.2);
                Rotate(FName(*(TEXT("wrist.")+Side)),Forward,Sign*(.07+.13*TalkWave));
            }
            for(int Finger=1;Finger<=5;Finger++)for(int Segment=1;Segment<=3;Segment++)
                Rotate(FName(*FString::Printf(TEXT("finger%d-%d.%s"),Finger,Segment,*Side)),Right,Sign*(.06+.05*FMath::Sin(Time*1.8+Finger*.6+Segment))*(1-.6*Gesture));
        }
        FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(CS),Output.Pose);
        return true;
    }
};
}

void UHALVETHGuideAnimInstance::NativeInitializeAnimation(){
    Super::NativeInitializeAnimation();auto* Body=GetSkelMeshComponent();if(!Body||!Body->GetSkeletalMeshAsset())return;
    const auto& Ref=Body->GetSkeletalMeshAsset()->GetRefSkeleton();const auto& Local=Ref.GetRefBonePose();TArray<FTransform> World;World.SetNum(Local.Num());
    for(int I=0;I<Local.Num();I++){int Parent=Ref.GetParentIndex(I);World[I]=Parent>=0?Local[I]*World[Parent]:Local[I];}
    auto Pos=[&](FName N){int I=Ref.FindBoneIndex(FName(*N.ToString().Replace(TEXT("."),TEXT("_"))));return I==INDEX_NONE?FVector::ZeroVector:World[I].GetLocation();};
    RefFeet[0]=Pos(TEXT("foot.L"));RefFeet[1]=Pos(TEXT("foot.R"));
    Forward=(Pos(TEXT("toe3-1.L"))-RefFeet[0]);Forward.Z=0;Forward.Normalize();
    Right=RefFeet[1]-RefFeet[0];Right.Z=0;Right.Normalize();
    // The authored A-pose spreads the feet for rigging. Preserve its coordinate
    // axes, then place stance targets nearer the centre of support; knees and
    // hips follow through the existing limb solve instead of scaling anatomy.
    const FVector Centre=(RefFeet[0]+RefFeet[1])*.5;
    for(int S=0;S<2;S++){
        RefFeet[S].X=Centre.X+(RefFeet[S].X-Centre.X)*.55;
        RefFeet[S].Y=Centre.Y+(RefFeet[S].Y-Centre.Y)*.55;
        FootTargets[S]=RefFeet[S];
        HandSprings[S].position=3;HandSprings[S].velocity=0;HandForward[S]=3;
        Planted[S]=false;
    }
    WeightShiftSpring=HalvethMotion::Spring{};WeightShift=0;
    UE_LOG(LogTemp,Display,TEXT("GARDEN_MOTION_RIG left_foot=%s right_foot=%s forward=%s right=%s"),*RefFeet[0].ToString(),*RefFeet[1].ToString(),*Forward.ToString(),*Right.ToString());
    static bool Printed=false;if(!Printed){Printed=true;FString Names;for(int I=0;I<Local.Num();I++)Names+=Ref.GetBoneName(I).ToString()+TEXT(",");UE_LOG(LogTemp,Display,TEXT("GARDEN_MOTION_BONES %s"),*Names);}
}
void UHALVETHGuideAnimInstance::NativeUpdateAnimation(float Dt){
    Super::NativeUpdateAnimation(Dt);if(!FMath::IsFinite(Dt)||Dt<=0)return;
    auto* Body=GetSkelMeshComponent();if(!Body)return;
    if(FreezeAppearancePose){
        Time=0;GaitPhase=0;WalkWeight=0;LookYaw=0;LookPitch=0;Gesture=0;WeightShift=0;
        for(int32 Side=0;Side<2;Side++){HandForward[Side]=3;FootTargets[Side]=RefFeet[Side];GroundNormals[Side]=FVector::UpVector;}
        Body->SetMorphTarget(TEXT("Blink"),0);Body->SetMorphTarget(TEXT("Talk"),0);
        Body->SetMorphTarget(TEXT("Breath"),.25f);Body->SetMorphTarget(TEXT("Smile"),0);return;
    }
    auto* Pawn=Cast<ACharacter>(Body->GetOwner());
    auto* Movement=Pawn?Pawn->GetCharacterMovement():nullptr;
    // Both residents and guides are ACharacters. Read actual grounded speed
    // each animation update; non-character guide owners retain their input.
    if(Movement)DesiredSpeed=Movement->IsMovingOnGround()?Movement->Velocity.Size2D():0;
    const bool Supported=!Movement||Movement->IsMovingOnGround();
    Time+=Dt;
    WalkSpring.update(FMath::Clamp(DesiredSpeed/FMath::Max(1.,CruiseSpeed),0.,1.),Dt,7);WalkWeight=WalkSpring.position;
    GaitPhase+=.6*DesiredSpeed/(48.*FMath::Max(.08f,WalkWeight))*Dt;
    YawSpring.update(DesiredLookYaw,Dt,1.1/FMath::Max(.12,ResponseSeconds));LookYaw=YawSpring.position;
    PitchSpring.update(DesiredLookPitch,Dt,1.3/FMath::Max(.12,ResponseSeconds));LookPitch=PitchSpring.position;
    GestureSpring.update(Speaking?1:0,Dt,5);Gesture=GestureSpring.position;
    WeightShiftSpring.update(.3*FMath::Sin(Time*.71)+.85*WalkWeight*FMath::Sin(GaitPhase*2*PI),Dt,9);
    WeightShift=WeightShiftSpring.position;
    for(int S=0;S<2;S++){
        const double TalkWave=Gesture*(.65+.35*FMath::Sin(Time*2.+S));
        HandSprings[S].update(3.+20.*TalkWave+10.*WalkWeight*FMath::Sin(GaitPhase*2*PI+S*PI),Dt,11);
        HandForward[S]=HandSprings[S].position;
    }
    for(int S=0;S<2;S++){
        auto Step=HalvethMotion::Footstep(GaitPhase+S*.5);
        if(!Supported){FootTargets[S]=RefFeet[S];Planted[S]=false;GroundNormals[S]=FVector::UpVector;continue;}
        FVector Local=RefFeet[S]+Forward*(Step.forward*WalkWeight);
        FVector Contact=Body->GetComponentTransform().TransformPosition(Local);
        const bool WantPlant=Step.stance&&WalkWeight>.08f;
        if(WantPlant&&Planted[S]){Contact.X=PlantWorld[S].X;Contact.Y=PlantWorld[S].Y;}
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(GuideFeet),false);Query.AddIgnoredActor(Body->GetOwner());
        Query.AddIgnoredActor(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
        if(GetWorld()->LineTraceSingleByChannel(Hit,Contact+FVector(0,0,80),Contact-FVector(0,0,120),ECC_Visibility,Query)){
            double Ground=Body->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint).Z;
            GroundCorrections[S]=FMath::FInterpTo(GroundCorrections[S],Ground,Dt,15);
            // The new full, barefoot source binds its sole to local Z=0.
            // Do not retain the removed boot's 3.204499 cm negative sole offset.
            Contact.Z=Hit.ImpactPoint.Z+RefFeet[S].Z+Step.lift*WalkWeight;
            GroundNormals[S]=Body->GetComponentTransform().InverseTransformVectorNoScale(Hit.ImpactNormal).GetSafeNormal();
            if(WantPlant&&!Planted[S])PlantWorld[S]=Contact;
            if(WantPlant&&Planted[S]){PlantDriftMax=FMath::Max(PlantDriftMax,FVector::Dist2D(Contact,PlantWorld[S]));PlantSamples++;}
            FootTargets[S]=Body->GetComponentTransform().InverseTransformPosition(Contact);
        }else{
            FootTargets[S]=Local+FVector(0,0,Step.lift*WalkWeight);
            GroundNormals[S]=FVector::UpVector;
        }
        Planted[S]=WantPlant;
    }
    // Self-timed, phase-offset blinks also reach ordinary residents, whose
    // faces previously stayed fixed unless the guide dialogue controller ran.
    const double BlinkTime=FMath::Fmod(Time+Identity*.731,3.1+Identity*.173);
    const double Blink=FMath::Max(0.,1.-FMath::Abs(BlinkTime-.12)/.12);
    Body->SetMorphTarget(TEXT("Blink"),Blink*Blink*(3.-2.*Blink));
    Body->SetMorphTarget(TEXT("Talk"),Gesture*(.18+.22*FMath::Sin(Time*8.4+Identity)));
    Body->SetMorphTarget(TEXT("Breath"),(.4+BodyLoad)*(.5+.5*FMath::Sin(BodyBreathPhase))+.015*(.5+.5*FMath::Sin(BodyHeartPhase)));
    Body->SetMorphTarget(TEXT("Smile"),.35*Gesture);
}
void UHALVETHGuideAnimInstance::LogPoseAudit(const TCHAR* Context) const {
    const auto* Body=GetSkelMeshComponent();if(!Body||!PoseAuditReady)return;
    for(int S=0;S<2;S++){
        const FString Side=S==0?TEXT("L"):TEXT("R");
        const FVector A=Body->GetBoneLocation(FName(*(TEXT("upperleg01_")+Side)),EBoneSpaces::ComponentSpace);
        const FVector B=Body->GetBoneLocation(FName(*(TEXT("lowerleg01_")+Side)),EBoneSpaces::ComponentSpace);
        const FVector C=Body->GetBoneLocation(FName(*(TEXT("foot_")+Side)),EBoneSpaces::ComponentSpace);
        const double Bend=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct((B-A).GetSafeNormal(),(C-B).GetSafeNormal()),-1.,1.)));
        const FVector Shoulder=Body->GetBoneLocation(FName(*(TEXT("upperarm01_")+Side)),EBoneSpaces::ComponentSpace);
        const FVector Elbow=Body->GetBoneLocation(FName(*(TEXT("lowerarm01_")+Side)),EBoneSpaces::ComponentSpace);
        const FVector Hand=Body->GetBoneLocation(FName(*(TEXT("wrist_")+Side)),EBoneSpaces::ComponentSpace);
        const double ElbowBend=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct((Elbow-Shoulder).GetSafeNormal(),(Hand-Elbow).GetSafeNormal()),-1.,1.)));
        UE_LOG(LogTemp,Display,TEXT("GARDEN_ARM_SAMPLE context=%s identity=%d side=%s elbow_bend_deg=%.5f idle_loaded=%d"),Context,Identity,*Side,ElbowBend,BaseIdle!=nullptr);
        UE_LOG(LogTemp,Display,TEXT("GARDEN_POSTURE_SAMPLE context=%s identity=%d side=%s speed_cm_s=%.5f walk_weight=%.5f knee_bend_deg=%.5f ankle_target_error_cm=%.5f upper_length_cm=%.5f lower_length_cm=%.5f target_binding=last_completed_evaluation"),Context,Identity,*Side,DesiredSpeed,WalkWeight,Bend,FVector::Distance(C,EvaluatedFootTargets[S]),FVector::Distance(A,B),FVector::Distance(B,C));
    }
}
FAnimInstanceProxy* UHALVETHGuideAnimInstance::CreateAnimInstanceProxy(){return new FGuideProxy(this);}
void UHALVETHGuideAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* P){delete P;}
