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
    ScheduleOrigin=FMath::Fmod(Realm->GetSkyClockUnix(),86400.);
    for(auto& R:Residents){R.lastUpdate=ScheduleOrigin;R.task=HalvethLife::routine(R,ScheduleOrigin);R.position=HalvethLife::destination(R,ScheduleOrigin);}
    CivicBeam->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_AstralSurface.M_AstralSurface"))){auto* Material=UMaterialInstanceDynamic::Create(Base,CivicBeam);Material->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.1f,.65f,1));Material->SetScalarParameterValue(TEXT("Glow"),220);CivicBeam->SetMaterial(0,Material);}
    CaptureMode=FParse::Param(FCommandLine::Get(),TEXT("HalvethLivingCapture"));Probe=CaptureMode||FParse::Param(FCommandLine::Get(),TEXT("HalvethLivingProbe"));
    if(auto* Budget=IAnimationBudgetAllocator::Get(GetWorld()))Budget->SetEnabled(true);
    if(auto* Setting=IConsoleManager::Get().FindConsoleVariable(TEXT("a.Budget.BudgetMs")))Setting->Set(3.f,ECVF_SetByCode);
    UE_LOG(LogTemp,Display,TEXT("LIVING_WORLD_READY records=4096 realms=4 physical_budget=%d animation_budget_ms=3 behaviour=authored_local no_network=true"),BodyBudget);
}
void AHALVETHLivingWorld::EndPlay(const EEndPlayReason::Type Reason){
    PrepareRealmChange();Super::EndPlay(Reason);
}
void AHALVETHLivingWorld::PrepareRealmChange(){
    for(auto& B:Bodies)if(B.Actor){Residents[B.Id].position={B.Actor->GetActorLocation().X/100,B.Actor->GetActorLocation().Y/100};B.Actor->Destroy();}
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
void AHALVETHLivingWorld::RefreshBodies(){
    if(!Realm)return;const int32 R=Realm->GetCurrentRealm();
    if(LastRealm!=R){PrepareRealmChange();LastRealm=R;}
    auto* P=UGameplayStatics::GetPlayerPawn(this,0);if(!P)return;
    struct Choice{int32 Id;double Rank;};TArray<Choice> Choices;
    for(auto& Resident:Residents)if(Resident.realm==R){
        const FVector At(Resident.position.x*100,Resident.position.y*100,0);
        double D=FVector::DistSquared2D(At,P->GetActorLocation());
        const bool Existing=Bodies.ContainsByPredicate([&](const FHALVETHResidentBody& B){return B.Id==int32(Resident.id);});
        // Do not materialize someone inside the player or on the narrow
        // arrival stair. Existing walkers may use it and yield normally.
        if(!Existing&&!Resident.guard&&(D<200.*200.||(R==0&&FMath::Abs(At.X-2500)<180&&At.Y>-1530&&At.Y<-820)))continue;
        if(Resident.guard)D=-1000000.+double(Resident.id);
        else if(Existing)D*=.55;
        if(Resident.guard||D<5000.*5000.)Choices.Add({int32(Resident.id),D});
    }
    Choices.Sort([](const Choice& A,const Choice& B){return A.Rank==B.Rank?A.Id<B.Id:A.Rank<B.Rank;});
    if(Choices.Num()>BodyBudget)Choices.SetNum(BodyBudget);
    for(int32 I=Bodies.Num()-1;I>=0;I--)if(!Choices.ContainsByPredicate([&](const Choice& C){return C.Id==Bodies[I].Id;})){
        auto& B=Bodies[I];if(B.Actor){Residents[B.Id].position={B.Actor->GetActorLocation().X/100,B.Actor->GetActorLocation().Y/100};B.Actor->Destroy();}Bodies.RemoveAt(I);
    }
    for(const auto C:Choices){
        if(Bodies.ContainsByPredicate([&](const FHALVETHResidentBody& B){return B.Id==C.Id;}))continue;
        auto& Resident=Residents[C.Id];FVector At(Resident.position.x*100,Resident.position.y*100,0);
        if(Resident.guard)At=FVector(2500+(C.Id<4?-300:300),-450,0);
        if(!Supported(At))continue;
        FActorSpawnParameters Params;Params.Owner=this;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
        auto* Actor=GetWorld()->SpawnActor<AHALVETHCitizen>(At,FRotator(0,-90,0),Params);if(!Actor)continue;
        FHALVETHResidentBody B;B.Id=C.Id;B.Actor=Actor;B.Origin=Actor->GetActorLocation();B.Goal=B.Origin;
        const TCHAR* AssetNames[]={TEXT("Scarlet"),TEXT("Lucinet"),TEXT("Rachel")};const FString Asset=AssetNames[Resident.guard?1:Resident.seed%3];
        auto* Mesh=Actor->GetMesh();Mesh->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/Characters/")+Asset+TEXT(".")+Asset)));
        Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);Mesh->SetAnimInstanceClass(UHALVETHGuideAnimInstance::StaticClass());
        Mesh->SetRelativeScale3D(FVector(1.+(int32(Resident.seed%13)-6)*.008));
        Mesh->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
        if(auto* Motion=Cast<UHALVETHGuideAnimInstance>(Mesh->GetAnimInstance())){
            Motion->BaseIdle=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/Characters/")+Asset+TEXT("Idle.")+Asset+TEXT("Idle")));
            Motion->Identity=int32(Resident.seed%3)+1;Motion->CruiseSpeed=110;
            Motion->Time=double(Resident.seed%12000)/1000.;Motion->GaitPhase=double((Resident.seed>>12)%1000)/1000.;
            Motion->ResponseSeconds=.14+double((Resident.seed>>16)%15)/100.;
        }
        auto* BudgetMesh=CastChecked<USkeletalMeshComponentBudgeted>(Mesh);BudgetMesh->SetAutoCalculateSignificance(false);
        B.Label=NewObject<UTextRenderComponent>(Actor);Actor->AddInstanceComponent(B.Label);B.Label->SetupAttachment(Actor->GetRootComponent());B.Label->SetRelativeLocation(FVector(0,0,118));
        B.Label->SetText(FText::FromString(ResidentName(C.Id)));B.Label->SetWorldSize(13);B.Label->SetHorizontalAlignment(EHTA_Center);B.Label->SetTextRenderColor(Resident.guard?FColor(130,200,255):FColor(225,206,162));B.Label->SetVisibility(false);B.Label->RegisterComponent();
        Bodies.Add(B);
    }
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
        const FVector At=B.Actor->GetActorLocation();B.Travel=FMath::Max(B.Travel,FVector::Dist2D(At,B.Origin));Resident.position={At.X/100,At.Y/100};Resident.lastUpdate=ScheduleOrigin+Time;
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
        CastChecked<USkeletalMeshComponentBudgeted>(Mesh)->SetComponentSignificance(float(1/(1+Distance/1500)),Distance<600,false,true,true);
        const bool Seen=P&&Distance<700&&Perceives(B.Actor,P);
        if(auto* Motion=Cast<UHALVETHGuideAnimInstance>(Mesh->GetAnimInstance())){
            Motion->DesiredSpeed=Move->IsMovingOnGround()?Move->Velocity.Size2D():0;Motion->CruiseSpeed=Move->MaxWalkSpeed;Motion->Speaking=Time<B.TalkUntil;
            Motion->BodyLoad=FMath::Clamp(double(Motion->DesiredSpeed)/390.,0.,1.);
            Motion->BodyBreathPhase=Time*(1.35+double(Resident.seed%31)/100.)+Resident.seed%19;
            Motion->BodyHeartPhase=Time*6.+Resident.seed%23;
            if(P&&Seen){const FVector Local=Mesh->GetComponentTransform().InverseTransformVectorNoScale(P->GetActorLocation()+FVector(0,0,45)-Mesh->GetComponentLocation());Motion->DesiredLookYaw=FMath::Clamp(FMath::Atan2(FVector::DotProduct(Local,Motion->Right),FVector::DotProduct(Local,Motion->Forward)),-.65,.65);}
            else Motion->DesiredLookYaw=0;
        }
        if(B.Label){B.Label->SetVisibility(P&&Distance<450);if(P)B.Label->SetWorldRotation((P->GetActorLocation()-B.Label->GetComponentLocation()).Rotation());}
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
