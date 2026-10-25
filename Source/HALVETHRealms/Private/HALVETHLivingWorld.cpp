#include "HALVETHLivingWorld.h"
#include "HALVETHCitizen.h"
#include "HALVETHCharacter.h"
#include "HALVETHRealmWorld.h"
#include "HALVETHAstralSanctum.h"
#include "HALVETHGameMode.h"
#include "HALVETHGuideAnimInstance.h"
#include "HALVETHAdventureComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "SceneView.h"
#include "ConvexVolume.h"
#include "HALVETHTissueComponent.h"
#include "HumanAppearance.h"
#include "HumanAppearanceRuntime.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "IAnimationBudgetAllocator.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HighResScreenshot.h"

AHALVETHLivingWorld::AHALVETHLivingWorld(){
    PrimaryActorTick.bCanEverTick=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("LivingWorldRoot")));
    CivicBeam=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WarningRay"));CivicBeam->SetupAttachment(RootComponent);CivicBeam->SetCollisionEnabled(ECollisionEnabled::NoCollision);CivicBeam->SetCastShadow(false);CivicBeam->SetVisibility(false);
}
void AHALVETHLivingWorld::Initialize(AHALVETHRealmWorld* World,uint32 Seed){
    Realm=World;Residents=HalvethLife::population(Seed,4096);
    InitialPopulationPending=true;ContinuityFailures=0;
    ScheduleOrigin=FMath::Fmod(Realm->GetSkyClockUnix(),86400.);
    for(auto& R:Residents){R.lastUpdate=ScheduleOrigin;R.task=HalvethLife::routine(R,ScheduleOrigin);R.position=HalvethLife::destination(R,ScheduleOrigin);}
    CivicBeam->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_AstralSurface.M_AstralSurface"))){auto* Material=UMaterialInstanceDynamic::Create(Base,CivicBeam);Material->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.1f,.65f,1));Material->SetScalarParameterValue(TEXT("Glow"),220);CivicBeam->SetMaterial(0,Material);}
    CaptureMode=FParse::Param(FCommandLine::Get(),TEXT("HalvethLivingCapture"));Probe=CaptureMode||FParse::Param(FCommandLine::Get(),TEXT("HalvethLivingProbe"));
    if(auto* Budget=IAnimationBudgetAllocator::Get(GetWorld()))Budget->SetEnabled(true);
    if(auto* Setting=IConsoleManager::Get().FindConsoleVariable(TEXT("a.Budget.BudgetMs")))Setting->Set(3.f,ECVF_SetByCode);
    UE_LOG(LogTemp,Display,TEXT("LIVING_WORLD_READY records=4096 realms=4 physical_budget=%d animation_budget_ms=3 behaviour=authored_local no_network=true"),BodyBudget);
    UE_LOG(LogTemp,Display,TEXT("LIVING_CONTINUITY_POLICY entry_m=50 exit_m=65 unseen_cap_m=100 unseen_grace_s=2 rematerialize_delay_s=3 selection=retain_then_admit visibility=expanded_actual_camera_frustum occlusion_claim=NONE boot_exemption=initial_population_only"));
}
void AHALVETHLivingWorld::EndPlay(const EEndPlayReason::Type Reason){
    PrepareRealmChange();Super::EndPlay(Reason);
}
void AHALVETHLivingWorld::PrepareRealmChange(){
    // Realm unload is an explicit scene transition, not ordinary budget eviction.
    for(auto& B:Bodies)if(B.Actor){SavePhysicalState(B,true);B.Actor->Destroy();}
    if(!Bodies.IsEmpty())UE_LOG(LogTemp,Display,TEXT("LIVING_CONTINUITY_TRANSITION reason=realm_unload bodies=%d model_now=%.6f fixture=%d fixture_step=%d"),Bodies.Num(),ScheduleOrigin+Time,Probe||CaptureMode,ProbeStep);
    Bodies.Empty();Notice={};LastRealm=INDEX_NONE;ActiveGuard=INDEX_NONE;
    if(CivicBeam)CivicBeam->SetVisibility(false);BeamUntil=0;PopulationAt=Time;
}
FString AHALVETHLivingWorld::ResidentName(int32 Id) const{
    if(Residents[Id].guard)return Id<4?TEXT("Wache Neri"):TEXT("Wache Tal");
    const TCHAR* Names[]={TEXT("Ari"),TEXT("Mika"),TEXT("Elya"),TEXT("Noa"),TEXT("Rin"),TEXT("Lio"),TEXT("Niva"),TEXT("Aven"),TEXT("Sora"),TEXT("Jona"),TEXT("Kian"),TEXT("Miro")};
    return FString::Printf(TEXT("%s %04d"),Names[(Residents[Id].seed>>16)%UE_ARRAY_COUNT(Names)],Id);
}
FString AHALVETHLivingWorld::TaskName(HalvethLife::Task Task) const{
    switch(Task){case HalvethLife::Task::Rest:return TEXT("ruht sich aus");case HalvethLife::Task::Work:return TEXT("auf dem Weg zur Arbeit");case HalvethLife::Task::Market:return TEXT("zum Markt unterwegs");case HalvethLife::Task::Study:return TEXT("sucht einen Leseplatz");case HalvethLife::Task::Patrol:return TEXT("auf Rundgang");default:return TEXT("geht spazieren");}
}
bool AHALVETHLivingWorld::Supported(FVector& Location,const AActor* Ignore) const{
    // Architecture uses closed convex collision, while terrain supplies its
    // triangle collision. A complex-only query skips the architectural floor.
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ResidentFloor),false);if(Ignore)Q.AddIgnoredActor(Ignore);
    for(const auto& B:Bodies)if(B.Actor)Q.AddIgnoredActor(B.Actor);
    if(auto* P=UGameplayStatics::GetPlayerPawn(this,0))Q.AddIgnoredActor(P);
    FHitResult Hit;if(!GetWorld()->LineTraceSingleByObjectType(Hit,Location+FVector(0,0,600),Location-FVector(0,0,1800),FCollisionObjectQueryParams(ECC_WorldStatic),Q)||Hit.ImpactNormal.Z<.7)return false;
    Location.Z=Hit.ImpactPoint.Z+94;return true;
}
bool AHALVETHLivingWorld::SavePhysicalState(const FHALVETHResidentBody& B,bool Retiring){
    if(!B.Actor)return false;
    auto& Resident=Residents[B.Id];const FVector At=B.Actor->GetActorLocation();
    auto Pose=Resident.physicalPose;Pose.heightMetres=At.Z/100.;Pose.yawDegrees=B.Actor->GetActorRotation().Yaw;
    if(auto* Motion=Cast<UHALVETHGuideAnimInstance>(B.Actor->GetMesh()->GetAnimInstance())){
        Pose.idleSeconds=Motion->Time;Pose.gaitPhase=Motion->GaitPhase;
    }
    const double Now=ScheduleOrigin+Time;
    const bool Saved=Retiring?HalvethLife::retirePhysical(Resident,{At.X/100.,At.Y/100.},Now,Pose)
        :HalvethLife::synchronizePhysical(Resident,{At.X/100.,At.Y/100.},Now,Pose);
    if(!Saved){ContinuityFailures++;UE_LOG(LogTemp,Error,TEXT("LIVING_CONTINUITY_HANDOFF id=%d result=FAIL model_now=%.6f last_update=%.6f"),B.Id,Now,Resident.lastUpdate);return false;}
    if(Retiring)UE_LOG(LogTemp,Display,TEXT("LIVING_CONTINUITY_HANDOFF id=%d result=STORED model_now=%.6f last_update=%.6f clock_error_s=%.9f x_cm=%.6f y_cm=%.6f z_cm=%.6f yaw_deg=%.6f idle_s=%.6f gait_phase=%.9f fixture=%d fixture_step=%d"),B.Id,Now,Resident.lastUpdate,FMath::Abs(Now-Resident.lastUpdate),At.X,At.Y,At.Z,Pose.yawDegrees,Pose.idleSeconds,Pose.gaitPhase,Probe||CaptureMode,ProbeStep);
    return true;
}
void AHALVETHLivingWorld::RefreshBodies(){
    if(!Realm)return;const int32 R=Realm->GetCurrentRealm();
    if(LastRealm!=R){PrepareRealmChange();LastRealm=R;}
    auto* P=UGameplayStatics::GetPlayerPawn(this,0);if(!P)return;
    const double Now=ScheduleOrigin+Time;const bool Boot=InitialPopulationPending;
    HalvethContinuity::Policy Policy;Policy.bodyBudget=static_cast<std::size_t>(BodyBudget);
    // Use the real local-player projection, including viewport/FOV constraints,
    // not guard perception, pawn forward, a scene-capture camera or render timestamps.
    FConvexVolume View;FSceneViewProjectionData Projection;bool HasView=false;
    if(auto* PC=GetWorld()->GetFirstPlayerController())if(auto* Local=PC->GetLocalPlayer())
        if(Local->ViewportClient&&Local->ViewportClient->Viewport
           &&Local->GetProjectionData(Local->ViewportClient->Viewport,Projection)){
            const FMatrix Matrix=Projection.ComputeViewProjectionMatrix();
            if(!Matrix.ContainsNaN()&&Projection.GetConstrainedViewRect().Width()>0&&Projection.GetConstrainedViewRect().Height()>0){
                GetViewFrustumBounds(View,Matrix,false,false);HasView=View.Planes.Num()>=4;
            }
        }
    auto Visibility=[&](FVector At,double Radius){
        if(!HasView||At.ContainsNaN()||!FMath::IsFinite(Radius))return HalvethContinuity::Visibility::Unknown;
        return View.IntersectSphere(At,float(Radius+200.))?HalvethContinuity::Visibility::InView:HalvethContinuity::Visibility::OutsideView;
    };
    int32 Retained=0,Retired=0,Spawned=0,ViewProtected=0,DeferredView=0,DeferredBudget=0;
    // Retirement is independent of candidate rank. A full budget blocks admission;
    // it never evicts an existing visible or nearby body to make another slot.
    for(int32 I=Bodies.Num()-1;I>=0;I--){
        auto& B=Bodies[I];if(!B.Actor){Bodies.RemoveAt(I);continue;}
        FVector Centre,Extent;B.Actor->GetActorBounds(false,Centre,Extent);
        const auto Seen=Visibility(Centre,FMath::Max(250.,Extent.Size()));
        const double Distance=FVector::Dist2D(B.Actor->GetActorLocation(),P->GetActorLocation())/100.;
        const bool Pinned=B.Id==ActiveGuard&&Notice.stage!=HalvethLife::Stage::Quiet;
        if(!HalvethContinuity::retire(Distance,Now,Seen,Pinned,B.OutsideViewSince,Policy)){
            Retained++;ViewProtected+=Seen!=HalvethContinuity::Visibility::OutsideView;continue;
        }
        if(!SavePhysicalState(B,true)){Retained++;continue;}
        UE_LOG(LogTemp,Display,TEXT("LIVING_CONTINUITY_RETIRE id=%d reason=outside_view_and_exit distance2d_m=%.3f unseen_s=%.3f model_now=%.6f fixture=%d fixture_step=%d"),B.Id,Distance,Now-B.OutsideViewSince,Now,Probe||CaptureMode,ProbeStep);
        B.Actor->Destroy();Bodies.RemoveAt(I);Retired++;
    }
    auto Position=[](const HalvethLife::Resident& Resident){
        // The gate is only the initial guard placement. Return visits use the
        // saved/virtually advanced coordinates and saved height, never this reset.
        const auto At=HalvethLife::materializationPosition(Resident);
        return FVector(At.x*100,At.y*100,Resident.physicalPose.heightMetres*100);
    };
    struct Choice{int32 Id;double Rank;};TArray<Choice> Choices;
    for(const auto& Resident:Residents)if(Resident.realm==R){
        if(Bodies.ContainsByPredicate([&](const FHALVETHResidentBody& B){return B.Id==int32(Resident.id);}))continue;
        const double D=FVector::DistSquared2D(Position(Resident),P->GetActorLocation());
        if(D<=FMath::Square(Policy.enterMetres*100))Choices.Add({int32(Resident.id),Resident.guard?-1000000.+double(Resident.id):D});
    }
    Choices.Sort([](const Choice& A,const Choice& B){return A.Rank==B.Rank?A.Id<B.Id:A.Rank<B.Rank;});
    for(int32 I=0;I<Choices.Num();I++){
        if(Bodies.Num()>=BodyBudget){DeferredBudget=Choices.Num()-I;break;}
        auto& Resident=Residents[Choices[I].Id];
        // Finish only this admitted candidate's virtual interval before computing
        // its spawn position and camera visibility. The remaining cohort stays live.
        if(!HalvethLife::advance(Resident,Now))continue;
        FVector At=Position(Resident);double Distance=FVector::Dist2D(At,P->GetActorLocation())/100.;
        auto Seen=Visibility(At,250.);
        if(!HalvethContinuity::materialize(static_cast<std::size_t>(Bodies.Num()),Distance,Now,Seen,Boot,Resident.retiredAt,Policy)){
            DeferredView+=!Boot&&Seen!=HalvethContinuity::Visibility::OutsideView;continue;
        }
        if(!Resident.guard&&(Distance<2.||(R==0&&FMath::Abs(At.X-2500)<180&&At.Y>-1530&&At.Y<-820)))continue;
        if(!Supported(At))continue;
        const auto Appearance=HalvethAppearance::traits(Resident.seed);
        const FString Asset=UTF8_TO_TCHAR(HalvethAppearance::Models[Appearance.model]);
        auto* BodyMesh=LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/Characters/")+Asset+TEXT(".")+Asset));if(!BodyMesh)continue;
        const double Scale=Appearance.heightScale;
        const auto Bounds=BodyMesh->GetBounds();
        const double Radius=FMath::Max(250.,(Bounds.Origin.Size()+Bounds.SphereRadius)*Scale+94.15);
        Seen=Visibility(At,Radius);Distance=FVector::Dist2D(At,P->GetActorLocation())/100.;
        if(!HalvethContinuity::materialize(static_cast<std::size_t>(Bodies.Num()),Distance,Now,Seen,Boot,Resident.retiredAt,Policy)){DeferredView++;continue;}
        auto Pose=Resident.wasPhysical?HalvethContinuity::resume(Resident.physicalPose,Resident.retiredAt,Now):HalvethContinuity::Pose{};
        if(!Resident.wasPhysical){Pose.idleSeconds=double(Resident.seed%12000)/1000.;Pose.gaitPhase=double((Resident.seed>>12)%1000)/1000.;}
        FActorSpawnParameters Params;Params.Owner=this;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
        auto* Actor=GetWorld()->SpawnActor<AHALVETHCitizen>(At,FRotator(0,Pose.yawDegrees,0),Params);if(!Actor)continue;
        FHALVETHResidentBody B;B.Id=Choices[I].Id;B.Actor=Actor;B.Origin=Actor->GetActorLocation();B.Goal=B.Origin;
        auto* Mesh=Actor->GetMesh();Mesh->SetSkeletalMeshAsset(BodyMesh);
        HalvethAppearanceRuntime::Apply(Mesh,Resident.seed);
        Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);Mesh->SetAnimInstanceClass(UHALVETHGuideAnimInstance::StaticClass());
        Mesh->SetRelativeScale3D(FVector(Scale));Mesh->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
        double GaitError=-1,IdleError=-1;
        if(auto* Motion=Cast<UHALVETHGuideAnimInstance>(Mesh->GetAnimInstance())){
            Motion->BaseIdle=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/Characters/")+Asset+TEXT("Idle.")+Asset+TEXT("Idle")));
            Motion->Identity=int32(Appearance.model)+1;Motion->CruiseSpeed=110;
            Motion->Time=Pose.idleSeconds;Motion->GaitPhase=Pose.gaitPhase;
            Motion->ResponseSeconds=.14+double((Resident.seed>>16)%15)/100.;
            GaitError=FMath::Abs(Motion->GaitPhase-Pose.gaitPhase);IdleError=FMath::Abs(Motion->Time-Pose.idleSeconds);
        }
        auto* BudgetMesh=CastChecked<USkeletalMeshComponentBudgeted>(Mesh);BudgetMesh->SetAutoCalculateSignificance(false);
        B.Label=NewObject<UTextRenderComponent>(Actor);Actor->AddInstanceComponent(B.Label);B.Label->SetupAttachment(Actor->GetRootComponent());B.Label->SetRelativeLocation(FVector(0,0,118));
        B.Label->SetText(FText::FromString(ResidentName(B.Id)));B.Label->SetWorldSize(13);B.Label->SetHorizontalAlignment(EHTA_Center);B.Label->SetTextRenderColor(Resident.guard?FColor(130,200,255):FColor(225,206,162));B.Label->SetVisibility(false);B.Label->RegisterComponent();
        const bool Rematerialized=Resident.wasPhysical;
        if(!SavePhysicalState(B,false)){Actor->Destroy();continue;}
        Bodies.Add(B);Spawned++;
        const double PositionError=FVector::Dist(Actor->GetActorLocation(),At);
        const double YawError=FMath::Abs(FMath::FindDeltaAngleDegrees(Actor->GetActorRotation().Yaw,Pose.yawDegrees));
        if(!FMath::IsFinite(PositionError)||!FMath::IsFinite(YawError)||!FMath::IsFinite(GaitError)||!FMath::IsFinite(IdleError)
           ||PositionError>.01||YawError>.001||GaitError<0||GaitError>1e-9||IdleError<0||IdleError>1e-9)ContinuityFailures++;
        UE_LOG(LogTemp,Display,TEXT("LIVING_CONTINUITY_SPAWN id=%d reason=%s rematerialized=%d guard=%d physical=%d model_now=%.6f last_update=%.6f x_cm=%.6f y_cm=%.6f z_cm=%.6f yaw_deg=%.6f idle_s=%.6f gait_phase=%.9f position_error_cm=%.9f yaw_error_deg=%.9f gait_error=%.9f idle_error_s=%.9f camera_known=%d fixture=%d fixture_step=%d"),B.Id,Boot?TEXT("initial_population"):TEXT("outside_view"),Rematerialized,Resident.guard,Bodies.Num(),Now,Resident.lastUpdate,At.X,At.Y,At.Z,Pose.yawDegrees,Pose.idleSeconds,Pose.gaitPhase,PositionError,YawError,GaitError,IdleError,HasView,Probe||CaptureMode,ProbeStep);
    }
    InitialPopulationPending=false;
    if(Bodies.Num()>BodyBudget){ContinuityFailures++;ensureMsgf(false,TEXT("Resident continuity exceeded its physical budget."));}
    UE_LOG(LogTemp,Display,TEXT("LIVING_CONTINUITY time=%.3f realm=%d boot=%d camera_known=%d physical=%d budget=%d retained=%d protected_view=%d retired=%d spawned=%d deferred_view=%d deferred_budget=%d failures=%u fixture=%d fixture_step=%d"),Time,R,Boot,HasView,Bodies.Num(),BodyBudget,Retained,ViewProtected,Retired,Spawned,DeferredView,DeferredBudget,ContinuityFailures,Probe||CaptureMode,ProbeStep);
}
FHALVETHResidentBody* AHALVETHLivingWorld::Guard(){
    if(ActiveGuard!=INDEX_NONE)for(auto& B:Bodies)if(B.Id==ActiveGuard)return &B;
    for(auto& B:Bodies)if(Residents[B.Id].guard)return &B;return nullptr;
}
bool AHALVETHLivingWorld::Perceives(const AHALVETHCitizen* Observer,const AHALVETHCharacter* Player) const{
    if(!Observer||!Player)return false;
    const FVector From=Observer->GetActorLocation()+FVector(0,0,65),To=Player->GetActorLocation()+FVector(0,0,45),Delta=To-From;
    if(Delta.SizeSquared()>FMath::Square(2200.)||FVector::DotProduct(Observer->GetActorForwardVector(),Delta.GetSafeNormal())<.15)return false;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ResidentSight),false,Observer);FHitResult H;
    const bool Hit=GetWorld()->LineTraceSingleByChannel(H,From,To,ECC_Visibility,Q);
    return !Hit||H.GetActor()==Player;
}
void AHALVETHLivingWorld::Steer(FHALVETHResidentBody& B,FVector Goal,float Dt){
    if(!B.Actor)return;const FVector At=B.Actor->GetActorLocation();FVector D=Goal-At;D.Z=0;B.Steering=FVector::ZeroVector;
    if(D.Size2D()<65||Time<B.TalkUntil)return;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ResidentStep),false,B.Actor);FHitResult Hit;
    for(double Turn:{0.,50.,-50.,100.,-100.}){
        FVector Candidate=D.GetSafeNormal().RotateAngleAxis(Turn,FVector::UpVector);FVector Foot=At+Candidate*100;
        if(!Supported(Foot,B.Actor)||FMath::Abs(Foot.Z-At.Z)>50)continue;
        if(GetWorld()->SweepSingleByChannel(Hit,At,At+Candidate*100,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(28,87),Q))continue;
        B.Steering=Candidate;B.Blocked=0;break;
    }
    if(B.Steering.IsNearlyZero()){B.Blocked+=Dt;if(B.Blocked>3){B.Goal=At+D.RotateAngleAxis(100,FVector::UpVector).GetSafeNormal()*200;B.Blocked=0;}}
}
void AHALVETHLivingWorld::AdvanceBodies(float Dt){
    auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    for(auto& B:Bodies){
        if(!B.Actor)continue;auto& Resident=Residents[B.Id];auto* Mesh=B.Actor->GetMesh();auto* Move=B.Actor->GetCharacterMovement();
        const FVector At=B.Actor->GetActorLocation();B.Travel=FMath::Max(B.Travel,FVector::Dist2D(At,B.Origin));SavePhysicalState(B,false);
        const auto Task=HalvethLife::routine(Resident,ScheduleOrigin+Time);if(Task!=Resident.task){Resident.task=Task;Resident.decisions++;}
        Move->MaxWalkSpeed=Resident.guard&&Notice.stage==HalvethLife::Stage::Pursuit?390:float(85+Resident.seed%50);
        const auto Desired=HalvethLife::destination(Resident,ScheduleOrigin+Time+Resident.id*.137);
        if(Time>B.TalkUntil&&FVector::Dist2D(At,B.Goal)<100)B.Goal=FVector(Desired.x*100,Desired.y*100,At.Z);
        if(Resident.guard&&B.Id==ActiveGuard&&Notice.stage==HalvethLife::Stage::Pursuit&&P&&Perceives(B.Actor,P))B.Goal=P->GetActorLocation();
        const double Distance=P?FVector::Dist(At,P->GetActorLocation()):99999;
        const bool Yield=P&&!P->IsAstralFlying()&&Distance<240&&Notice.stage!=HalvethLife::Stage::Pursuit;
        if(Yield){const FVector Right=P->GetActorRightVector();const double Side=FVector::DotProduct(At-P->GetActorLocation(),Right)>=0?1.:-1.;B.Goal=At+Right*(Side*260);Move->MaxWalkSpeed=200;}
        Steer(B,B.Goal,Dt);
        if(Resident.task==HalvethLife::Task::Rest&&!Yield)B.Steering=FVector::ZeroVector;
        const auto* Tissue=B.Actor->FindComponentByClass<UHALVETHTissueComponent>();const bool FullPose=Tissue&&Tissue->NeedsFullPose();
        CastChecked<USkeletalMeshComponentBudgeted>(Mesh)->SetComponentSignificance(FullPose?10.f:float(1/(1+Distance/1500)),FullPose||Distance<600,FullPose,!FullPose,!FullPose);
        const bool Seen=P&&Distance<700&&Perceives(B.Actor,P);
        if(auto* Motion=Cast<UHALVETHGuideAnimInstance>(Mesh->GetAnimInstance())){
            Motion->DesiredSpeed=Move->IsMovingOnGround()?Move->Velocity.Size2D():0;Motion->CruiseSpeed=Move->MaxWalkSpeed;Motion->Speaking=Time<B.TalkUntil;
            Motion->BodyLoad=FMath::Clamp(double(Motion->DesiredSpeed)/390.,0.,1.);
            Motion->BodyBreathPhase=Time*(1.35+double(Resident.seed%31)/100.)+Resident.seed%19;
            Motion->BodyHeartPhase=Time*6.+Resident.seed%23;
            FVector View=P?P->GetPawnViewLocation():FVector::ZeroVector;FRotator ViewRotation;
            if(auto* Controller=GetWorld()->GetFirstPlayerController())Controller->GetPlayerViewPoint(View,ViewRotation);
            const FVector Face=Mesh->GetComponentTransform().TransformPosition(Motion->RefGazeOrigin);
            Motion->ObserveLookTarget(View,P&&Seen,FVector::DotProduct((Face-View).GetSafeNormal(),ViewRotation.Vector())>.94);
        }
        if(B.Label){
            FVector View=P?P->GetPawnViewLocation():FVector::ZeroVector;FRotator Rotation;
            if(auto* Controller=GetWorld()->GetFirstPlayerController())Controller->GetPlayerViewPoint(View,Rotation);
            const FVector To=B.Label->GetComponentLocation()-View;
            const bool Focus=P&&Distance<260&&FVector::DotProduct(To.GetSafeNormal(),Rotation.Vector())>.94;
            B.Label->SetText(FText::FromString(TEXT("Ansprechen [E]")));B.Label->SetWorldSize(7);
            B.Label->SetVisibility(Focus);if(Focus)B.Label->SetWorldRotation((-To).Rotation());
        }
    }
}
void AHALVETHLivingWorld::ApplyNotice(HalvethLife::Event Event,AHALVETHCharacter* P){
    auto* Mode=Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this));if(!Mode||!P)return;
    switch(Event){
        case HalvethLife::Event::Warn:Mode->Notify(TEXT("Wache: Langsamer neben den Leuten, bitte. Du hast eine Verwarnung."));break;
        case HalvethLife::Event::Chase:Mode->Notify(TEXT("Wache: Halt kurz an! Ich komme zu dir."));break;
        case HalvethLife::Event::Hold:{P->ApplyCivicHold(2.2f);HoldStarted=Time;Mode->Notify(TEXT("Kurz festgesetzt (2 Sekunden). Nur eine Verwarnung, kein Schaden oder Geldverlust."));
            if(auto* G=Guard();G&&G->Actor){const FVector A=G->Actor->GetActorLocation()+FVector(0,0,60),B=P->GetActorLocation()+FVector(0,0,20);CivicBeam->SetWorldLocation((A+B)*.5);CivicBeam->SetWorldRotation(FRotationMatrix::MakeFromZ(B-A).Rotator());CivicBeam->SetWorldScale3D(FVector(.018,.018,FVector::Dist(A,B)/100));BeamUntil=Time+.35;CivicBeam->SetVisibility(true);}break;}
        case HalvethLife::Event::Release:Mode->Notify(TEXT("Wache: Alles gut. Du bist wieder frei. Bitte mehr Abstand."));break;
        case HalvethLife::Event::Resolved:Mode->Notify(TEXT("Wache: Danke. Viel Spass beim Fliegen!"));break;
        case HalvethLife::Event::Lost:Mode->Notify(TEXT("Die Wache hat die Verfolgung beendet."));break;
        default:return;
    }
    UE_LOG(LogTemp,Display,TEXT("LIVING_NOTICE event=%d state=%d guard=%d warnings=%d holds=%d time=%.3f player_hold=%d health=%.3f damage_applied=0 fine_applied=0"),int32(Event),int32(Notice.stage),ActiveGuard,Notice.warnings,Notice.holds,Time,P->IsCivicHeld(),P->GetAdventure()?P->GetAdventure()->GetHealth():-1);
}
void AHALVETHLivingWorld::UpdateNotice(AHALVETHCharacter* P){
    if(!P)return;bool Hazard=false,Visible=false,Reach=false;
    if(P->IsAstralFlying()&&P->GetVelocity().Size()>850){
        for(const auto& B:Bodies)if(B.Actor&&!Residents[B.Id].guard&&FVector::Dist(B.Actor->GetActorLocation(),P->GetActorLocation())<420){Hazard=true;break;}
    }
    auto* Observer=Guard();
    if(Notice.stage==HalvethLife::Stage::Quiet){
        double Best=FMath::Square(2200.);ActiveGuard=INDEX_NONE;
        for(auto& B:Bodies)if(B.Actor&&Residents[B.Id].guard&&Perceives(B.Actor,P)){
            double D=FVector::DistSquared(B.Actor->GetActorLocation(),P->GetActorLocation());if(D<Best){Best=D;ActiveGuard=B.Id;Observer=&B;}
        }
    }
    if(Observer&&Observer->Actor){Visible=Perceives(Observer->Actor,P);Reach=Visible&&FVector::Dist(Observer->Actor->GetActorLocation(),P->GetActorLocation())<500;}
    ApplyNotice(Notice.observe({Time,Visible,Hazard,Reach}),P);
}
bool AHALVETHLivingWorld::Interact(AHALVETHCharacter* P){
    if(!P)return false;FHALVETHResidentBody* Nearest=nullptr;double Best=300*300;
    for(auto& B:Bodies)if(B.Actor){
        const double D=FVector::DistSquared(B.Actor->GetActorLocation(),P->GetActorLocation());
        FCollisionQueryParams Q(SCENE_QUERY_STAT(ResidentConversation),false,P);FHitResult Hit;
        const bool Blocked=GetWorld()->LineTraceSingleByChannel(Hit,P->GetActorLocation()+FVector(0,0,45),B.Actor->GetActorLocation()+FVector(0,0,45),ECC_Visibility,Q)&&Hit.GetActor()!=B.Actor;
        if(D<Best&&!Blocked){Best=D;Nearest=&B;}
    }
    if(!Nearest)return false;auto& R=Residents[Nearest->Id];Nearest->TalkUntil=Time+4;Nearest->Steering=FVector::ZeroVector;
    const TCHAR* Lines[]={TEXT("Hey! Schoen, dich zu sehen. Wohin fliegst du?"),TEXT("He, Luftpirat! Pass auf die Leute auf, dann ist alles gut."),TEXT("Ich brauche gerade etwas Ruhe. Danke."),TEXT("Wie lenkst du eigentlich in der Luft? Das will ich lernen!")};
    FString Text=R.guard?TEXT("Fliegen ist erlaubt, auch im Archiv. Neben Leuten bitte langsam. Wir verwarnen, wenn es gefaehrlich wird."):FString(Lines[int(R.temper)]);
    if(auto* Mode=Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this)))Mode->Notify(ResidentName(Nearest->Id)+TEXT(": ")+Text+TEXT(" Â· ")+TaskName(R.task));
    return true;
}
FString AHALVETHLivingWorld::GetStatus() const{
    return FString::Printf(TEXT("J frei fliegen  Â·  %d Menschen in der Naehe  Â·  E sprechen"),Bodies.Num());
}
void AHALVETHLivingWorld::Tick(float Dt){
    Super::Tick(Dt);if(!Realm||Residents.empty()||!FMath::IsFinite(Dt)||Dt<=0)return;Time+=Dt;
    if(Time>=PopulationAt){PopulationAt=Time+1;const double Start=FPlatformTime::Seconds();
        // Only the remote cohort is advanced here. Physical residents retain their actual position.
        for(uint32 Count=0;Count<512;Count++){
            const auto I=BackgroundCursor++%uint32(Residents.size());
            if(!Bodies.ContainsByPredicate([&](const FHALVETHResidentBody& B){return B.Id==int32(I);}))HalvethLife::advance(Residents[I],ScheduleOrigin+Time);
        }
        RefreshBodies();MeasuredBackgroundMs=(FPlatformTime::Seconds()-Start)*1000;
    }
    if(Time>=BrainAt){BrainAt=Time+.2;const double Start=FPlatformTime::Seconds();AdvanceBodies(.2f);MeasuredActorMs=(FPlatformTime::Seconds()-Start)*1000;
        if(!Probe||(ProbeStep>=4&&ProbeStep<=6))UpdateNotice(Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0)));
    }
    for(auto& B:Bodies)if(B.Actor&&!B.Steering.IsNearlyZero())B.Actor->AddMovementInput(B.Steering,1,true);
    if(CivicBeam&&Time>=BeamUntil)CivicBeam->SetVisibility(false);
    if(Probe&&!ProbeFinished)RunProbe();if(CaptureMode)Capture();
}
