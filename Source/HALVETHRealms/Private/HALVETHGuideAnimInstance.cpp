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
    FVector Forward,Right,Feet[2],Normals[2];float Ground[2]={0,0};
    virtual void PreUpdate(UAnimInstance* I,float Dt) override {
        FAnimInstanceProxy::PreUpdate(I,Dt);
        auto* A=CastChecked<UHALVETHGuideAnimInstance>(I);
        Idle=A->BaseIdle;Time=A->Time;Phase=A->GaitPhase;Walk=A->WalkWeight;
        Yaw=A->LookYaw;Pitch=A->LookPitch;Gesture=A->Gesture;
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
        // Weight transfer is supported by a two-leg solve rather than floating
        // the whole rigid model. Head and eyes keep independent response times.
        const double Breath=FMath::Sin(Time*1.45),Sway=FMath::Sin(Time*.71);
        Shift(TEXT("root"),Right*(1.45*Sway)+FVector(0,0,-2.8-.55*Walk*FMath::Sin(Phase*4*PI)));
        Rotate(TEXT("spine03"),Forward,.024*Sway);
        Rotate(TEXT("spine02"),Right,.016*Breath);
        Rotate(TEXT("spine01"),FVector::UpVector,.025*FMath::Sin(Time*.57));
        const FVector YawAxis=FVector::CrossProduct(Forward,Right).GetSafeNormal();
        const FVector PitchAxis=FVector::CrossProduct(Forward,FVector::UpVector).GetSafeNormal();
        Rotate(TEXT("neck03"),YawAxis,Yaw*.35);
        Rotate(TEXT("head"),YawAxis,Yaw*.65+.045*FMath::Sin(Time*.83));
        Rotate(TEXT("head"),PitchAxis,Pitch+.035*FMath::Sin(Time*1.11));
        for(FName Eye:{FName(TEXT("eye.L")),FName(TEXT("eye.R"))}){
            Rotate(Eye,YawAxis,.08*FMath::Sin(Time*2.13)+Yaw*.12);
            Rotate(Eye,PitchAxis,.035*FMath::Sin(Time*1.31)+Pitch*.18);
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
            auto Step=HalvethMotion::Footstep(Phase+S*.5);
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
                const double Length=FVector::Distance(A,B)+FVector::Distance(B,C);
                double TalkWave=Gesture*(.65+.35*FMath::Sin(Time*2.0+S));
                // Hands rest beside the hips. The shoulder-relative target
                // avoids inheriting the elevated elbows of the source A-pose.
                FVector GoalHand=A-FVector::UpVector*(Length*(.91-.20*TalkWave))
                    +Forward*(7+16*TalkWave+9*Walk*FMath::Sin(Phase*2*PI+S*PI))
                    +Right*(Sign*(5+5*TalkWave))+FVector(0,0,1.2*FMath::Sin(Time*1.07+S));
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
    FootTargets[0]=RefFeet[0];FootTargets[1]=RefFeet[1];
    Forward=(Pos(TEXT("toe3-1.L"))-RefFeet[0]);Forward.Z=0;Forward.Normalize();
    Right=RefFeet[1]-RefFeet[0];Right.Z=0;Right.Normalize();
    UE_LOG(LogTemp,Display,TEXT("GARDEN_MOTION_RIG left_foot=%s right_foot=%s forward=%s right=%s"),*RefFeet[0].ToString(),*RefFeet[1].ToString(),*Forward.ToString(),*Right.ToString());
    static bool Printed=false;if(!Printed){Printed=true;FString Names;for(int I=0;I<Local.Num();I++)Names+=Ref.GetBoneName(I).ToString()+TEXT(",");UE_LOG(LogTemp,Display,TEXT("GARDEN_MOTION_BONES %s"),*Names);}
}
void UHALVETHGuideAnimInstance::NativeUpdateAnimation(float Dt){
    Super::NativeUpdateAnimation(Dt);if(!FMath::IsFinite(Dt)||Dt<=0)return;
    Time+=Dt;
    WalkSpring.update(FMath::Clamp(DesiredSpeed/42.f,0.f,1.f),Dt,7);WalkWeight=WalkSpring.position;
    GaitPhase+=.6*DesiredSpeed/(48.*FMath::Max(.08f,WalkWeight))*Dt;
    YawSpring.update(DesiredLookYaw,Dt,5);LookYaw=YawSpring.position;
    PitchSpring.update(DesiredLookPitch,Dt,6);LookPitch=PitchSpring.position;
    GestureSpring.update(Speaking?1:0,Dt,5);Gesture=GestureSpring.position;
    auto* Body=GetSkelMeshComponent();if(!Body)return;
    auto* Pawn=Cast<ACharacter>(Body->GetOwner());const bool Supported=!Pawn||Pawn->GetCharacterMovement()->IsMovingOnGround();
    for(int S=0;S<2;S++){
        auto Step=HalvethMotion::Footstep(GaitPhase+S*.5);
        if(!Supported){FootTargets[S]=RefFeet[S];Planted[S]=false;GroundNormals[S]=FVector::UpVector;continue;}
        FVector Local=RefFeet[S]+Forward*(Step.forward*WalkWeight);
        FVector Contact=Body->GetComponentTransform().TransformPosition(Local);
        const bool WantPlant=Step.stance&&WalkWeight>.08f;
        if(WantPlant&&Planted[S]){Contact.X=PlantWorld[S].X;Contact.Y=PlantWorld[S].Y;}
        FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(GuideFeet),true);Query.AddIgnoredActor(Body->GetOwner());
        Query.AddIgnoredActor(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
        if(GetWorld()->LineTraceSingleByChannel(Hit,Contact+FVector(0,0,80),Contact-FVector(0,0,120),ECC_Visibility,Query)){
            double Ground=Body->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint).Z;
            GroundCorrections[S]=FMath::FInterpTo(GroundCorrections[S],Ground,Dt,15);
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
    double Flutter=.45*FMath::Sin(Time*1.8+Identity)+.22*FMath::Sin(Time*3.13)+.35*WalkWeight;
    ClothSpring.update(Flutter,Dt,7);
    Body->SetMorphTarget(TEXT("ClothLeft"),FMath::Max(0.,ClothSpring.position));
    Body->SetMorphTarget(TEXT("ClothRight"),FMath::Max(0.,-ClothSpring.position));
    Body->SetMorphTarget(TEXT("HairLeft"),FMath::Max(0.,FMath::Sin(Time*1.39+Identity))*.55);
    Body->SetMorphTarget(TEXT("HairRight"),FMath::Max(0.,-FMath::Sin(Time*1.39+Identity))*.55);
    Body->SetMorphTarget(TEXT("Breath"),.5+.5*FMath::Sin(Time*1.45));
    Body->SetMorphTarget(TEXT("Smile"),.35*Gesture);
}
FAnimInstanceProxy* UHALVETHGuideAnimInstance::CreateAnimInstanceProxy(){return new FGuideProxy(this);}
void UHALVETHGuideAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* P){delete P;}
