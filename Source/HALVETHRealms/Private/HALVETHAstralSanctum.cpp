// Copyright (c) 2026 Juri Janovski. MIT.
#include "HALVETHAstralSanctum.h"
#include "HALVETHCharacter.h"
#include "HALVETHAdventureComponent.h"
#include "HALVETHKnowledgeComponent.h"
#include "HALVETHGameMode.h"
#include "ProceduralMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/App.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "CollisionShape.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HighResScreenshot.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"

namespace {
using namespace Astral;
void Append(Mesh& To,const Mesh& From,FVector At,FRotator Rot=FRotator::ZeroRotator,FVector Scale=FVector(1)) {
    const int Base=int(To.p.size());FTransform T(Rot,At,Scale);
    for(V P:From.p){const FVector Q=T.TransformPosition(FVector(P.x,P.y,P.z));To.p.push_back({Q.X,Q.Y,Q.Z});}
    for(int I:From.t)To.t.push_back(Base+I);
    for(const auto& Part:From.convexParts){std::vector<V> Out;for(V P:Part){const FVector Q=T.TransformPosition(FVector(P.x,P.y,P.z));Out.push_back({Q.X,Q.Y,Q.Z});}To.convexParts.push_back(std::move(Out));}
}
void Block(Mesh& To,FVector At,FVector Half,double Bevel=3,FRotator Rot=FRotator::ZeroRotator){Append(To,block({Half.X,Half.Y,Half.Z},Bevel),At,Rot);}
void Circle(Mesh& To,FVector At,double Radius,double Tube=1.1,FRotator Rot=FRotator::ZeroRotator){Append(To,ring(Radius,Tube),At,Rot);}
}

AHALVETHAstralSanctum::AHALVETHAstralSanctum(){
    PrimaryActorTick.bCanEverTick=true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SanctumRoot")));
}

UProceduralMeshComponent* AHALVETHAstralSanctum::Mesh(const Astral::Mesh& Data,const FString& Name,FLinearColor Tint,float Metallic,float Roughness,float Glow,bool Solid){
    auto* C=NewObject<UProceduralMeshComponent>(this,*Name);AddInstanceComponent(C);C->SetupAttachment(RootComponent);
    C->SetMobility(EComponentMobility::Movable);C->bUseAsyncCooking=false;C->bUseComplexAsSimpleCollision=false;
    C->SetCollisionProfileName(Solid?TEXT("BlockAll"):TEXT("NoCollision"));C->SetGenerateOverlapEvents(false);
    C->CanCharacterStepUpOn=ECB_Yes;
    // The 45-degree bevel is slightly steeper than CharacterMovement's default
    // walkable cutoff. Permit the bevel, while keeping vertical risers blocking.
    C->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Increase,50.f));
    UpdateGeometry(C,Data,Solid);
    auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_AstralSurface.M_AstralSurface"));
    if(!Base)Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_HalvethSurface.M_HalvethSurface"));
    if(Base){auto* M=UMaterialInstanceDynamic::Create(Base,C);M->SetVectorParameterValue(TEXT("Tint"),Tint);M->SetScalarParameterValue(TEXT("Metallic"),Metallic);M->SetScalarParameterValue(TEXT("Roughness"),Roughness);M->SetScalarParameterValue(TEXT("Glow"),Glow*RadianceScale);C->SetMaterial(0,M);if(Glow>0){GlowMaterials.Add(M);GlowBase.Add(Glow);}}
    C->SetCastShadow(Glow<30);C->RegisterComponent();Parts.Add(C);if(Solid)SolidTriangles+=int32(Data.t.size()/3);return C;
}
void AHALVETHAstralSanctum::UpdateGeometry(UProceduralMeshComponent* C,const Astral::Mesh& Data,bool Solid){
    TArray<FVector> P,N;TArray<int32> T;TArray<FVector2D> UV;TArray<FLinearColor> Colors;TArray<FProcMeshTangent> Tangents;
    P.Reserve(Data.p.size());N.SetNumZeroed(Data.p.size());
    for(auto V:Data.p){P.Add(FVector(V.x,V.y,V.z));}
    for(size_t I=0;I<Data.t.size();I+=3){int A=Data.t[I],B=Data.t[I+1],D=Data.t[I+2];const FVector Normal=FVector::CrossProduct(P[B]-P[A],P[D]-P[A]).GetSafeNormal();N[A]+=Normal;N[B]+=Normal;N[D]+=Normal;
        // Portable model is outward CCW; UE's front-face index convention is clockwise.
        T.Append({A,D,B});}
    for(int I=0;I<P.Num();I++){
        N[I].Normalize();const FVector A=N[I].GetAbs();FVector Tangent;
        if(A.Z>=A.X&&A.Z>=A.Y){UV.Add(FVector2D(P[I].X,P[I].Y)/100);Tangent=FVector(1,0,0);}
        else if(A.X>=A.Y){UV.Add(FVector2D(P[I].Y,P[I].Z)/100);Tangent=FVector(0,1,0);}
        else{UV.Add(FVector2D(P[I].X,P[I].Z)/100);Tangent=FVector(1,0,0);}
        const double Variation=.91+.09*FMath::Sin(P[I].X*.012+P[I].Y*.007+P[I].Z*.015);Colors.Add(FLinearColor(Variation,Variation,Variation,1));Tangents.Add(FProcMeshTangent(Tangent,false));
    }
    C->CreateMeshSection_LinearColor(0,P,T,N,UV,Colors,Tangents,false);
    if(Solid){TArray<TArray<FVector>> Hulls;for(const auto& Part:Data.convexParts){TArray<FVector> Hull;for(auto V:Part)Hull.Add(FVector(V.x,V.y,V.z));Hulls.Add(MoveTemp(Hull));}C->SetCollisionConvexMeshes(Hulls);}
}
void AHALVETHAstralSanctum::Lamp(FVector At,FLinearColor Color,float Intensity,float Radius){
    auto* L=NewObject<UPointLightComponent>(this);AddInstanceComponent(L);L->SetupAttachment(RootComponent);L->SetRelativeLocation(At);
    L->SetMobility(EComponentMobility::Movable);
    L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(Intensity);L->SetLightColor(Color);L->SetAttenuationRadius(Radius);L->SetSourceRadius(25);L->SetCastShadows(false);L->RegisterComponent();Lamps.Add(L);LampBase.Add(Intensity);
}
void AHALVETHAstralSanctum::SetDaylight(double Day){
    const float Blend=float(FMath::Clamp(Day,0.,1.));RadianceScale=FMath::Lerp(.18f,1.f,Blend);
    for(int32 I=0;I<Lamps.Num();I++)Lamps[I]->SetIntensity(LampBase[I]*FMath::Lerp(.08f,1.f,Blend));
    for(int32 I=0;I<GlowMaterials.Num();I++)GlowMaterials[I]->SetScalarParameterValue(TEXT("Glow"),GlowBase[I]*RadianceScale);
}
void AHALVETHAstralSanctum::Sign(FVector At,const FString& Text,float Size){
    auto* C=NewObject<UTextRenderComponent>(this);AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetRelativeLocation(At);C->SetRelativeRotation(FRotator(0,-90,0));
    C->SetText(FText::FromString(Text));C->SetTextRenderColor(FColor(221,192,130));C->SetHorizontalAlignment(EHTA_Center);C->SetWorldSize(Size);C->RegisterComponent();
}

void AHALVETHAstralSanctum::Build(uint32 Seed){
    using namespace Astral;FRandomStream Random(int32(Seed)^142039);
    Astral::Mesh Stone,Gold,Dark,Runes,Books,BooksBlue,BooksGreen,BooksOchre,Pages;
    // The complete foundation reaches below the traced ground. Individual joints
    // sit on this volume; gaps are never the player's only collision support.
    Block(Stone,FVector(0,600,-220),FVector(1000,1400,380),18);
    Block(Stone,FVector(0,450,128),FVector(1050,1250,32),9);
    for(int S=0;S<8;S++){const double Top=20*(S+1);Block(Stone,FVector(0,-1450+S*90,(Top-200)*.5),FVector(540,48,(Top+200)*.5),4);}
    // Masonry pavers have volume, bevels and staggered joints.
    for(int Y=0;Y<22;Y++)for(int X=0;X<15;X++){
        const double XX=-966+X*138+(Y%2?33:0);if(FMath::Abs(XX)>980)continue;
        Block(Dark,FVector(XX,-750+Y*110,166+Random.FRandRange(-.3,.3)),FVector(66.5,52.5,6),2.3);
    }
    // Guard walls, pilasters, capitals, lantern plinths and planted margins.
    for(int Side:{-1,1}){
        Block(Stone,FVector(Side*1040,370,210),FVector(42,1300,50),6);
        Block(Gold,FVector(Side*1040,370,266),FVector(49,1300,6),2);
        for(int J=0;J<6;J++){
            const FVector P(Side*1040,-830+J*470,0);
            Block(Stone,P+FVector(0,0,240),FVector(72,72,90),6);
            Block(Gold,P+FVector(0,0,334),FVector(78,78,8),2);
            auto* Crystal=Mesh(crystal(30,125),FString::Printf(TEXT("PathCrystal%d_%d"),Side,J),FLinearColor(.32,.075,.72),.28,.17,240,false);Crystal->SetRelativeLocation(P+FVector(0,0,344));
            Lamp(P+FVector(0,0,400),FLinearColor(.57,.19,1),65000,650);
        }
    }
    // Four substantial piers and three nested archivolts. Every arch is a set
    // of closed voussoirs, with visible front/back thickness.
    for(int Side:{-1,1}){
        Block(Stone,FVector(Side*465,900,490),FVector(94,110,330),12);
        for(int J=0;J<6;J++)Block(Gold,FVector(Side*465,900,200+J*120),FVector(103,118,9),3);
        for(int Flute=0;Flute<5;Flute++)Block(Dark,FVector(Side*465-65+Flute*32,779,490),FVector(10,12,270),4);
        Block(Stone,FVector(Side*465,900,840),FVector(126,138,35),8);
    }
    for(int Layer=0;Layer<3;Layer++)for(int I=0;I<40;I++){
        const double A=Pi*I/40+.002,B=Pi*(I+1)/40-.002;
        Append(Stone,sector(365+Layer*55,415+Layer*55,A,B,90),FVector(0,840+Layer*95,840),FRotator(0,0,-90));
    }
    // Raised rear terrace is supported by the tower rather than a floating sheet.
    Block(Stone,FVector(0,2610,440),FVector(980,700,600),22);
    Block(Stone,FVector(0,2610,1080),FVector(1050,740,40),12);
    // Rear tower facade is masonry, not one featureless stretched cuboid.
    for(int Row=0;Row<9;Row++)for(int Col=0;Col<9;Col++){
        const double X=-870+Col*216+(Row%2?22:0);
        Block(Stone,FVector(X,1900,60+Row*115),FVector(104,22,54),5);
    }
    for(double Z:{140.,520.,1010.})Block(Gold,FVector(0,1872,Z),FVector(982,5,4),1.5);
    for(int Side:{-1,1})for(int J=0;J<3;J++){
        const FVector P(Side*920,2040+J*570,0);
        Block(Stone,P+FVector(0,0,960),FVector(90,95,1000),14);
        Block(Gold,P+FVector(0,0,1520),FVector(106,108,12),3);
        Block(Gold,P+FVector(0,0,1910),FVector(112,114,18),5);
        Append(Stone,crystal(70,340),P+FVector(0,0,1960));
    }
    for(int Side:{-1,1})for(int Row=0;Row<9;Row++){
        Block(Dark,FVector(Side*780,2680,1150+Row*43),FVector(40,420,5),2);
        for(int B=0;B<45;B++){
            const double Height=Random.FRandRange(23,36);FVector P(Side*780,2290+B*18,1156+Row*43+Height*.5);
            Astral::Mesh* Volume=(B+Row)%4==0?&BooksBlue:(B+Row)%4==1?&BooksGreen:(B+Row)%4==2?&BooksOchre:&Books;
            Block(*Volume,P,FVector(24,Random.FRandRange(4,7),Height*.5),1,FRotator(Random.FRandRange(-5,5),0,0));
            for(int Band:{-1,1})Block(Gold,P+FVector(-Side*24,0,Band*Height*.32),FVector(.65,4.2,.75),.25);
        }
    }
    // The archive has a backed wall, protective parapets and a real vaulted roof.
    Block(Stone,FVector(0,3340,1460),FVector(1020,50,340),10);
    for(int Side:{-1,1})Block(Stone,FVector(Side*1010,2640,1230),FVector(40,710,110),8);
    for(int I=0;I<64;I++)Append(Stone,sector(930,980,Pi*I/64,Pi*(I+1)/64,1390),FVector(0,2640,1510),FRotator(0,0,-90));
    for(double Y:{1960.,2630.,3320.})for(int I=0;I<48;I++)Append(Gold,sector(921,932,Pi*I/48+.001,Pi*(I+1)/48-.001,26),FVector(0,Y,1510),FRotator(0,0,-90));
    for(double Y:{2200.,2660.,3110.}){
        Circle(Gold,FVector(0,Y,1830),105,3);
        for(int I=0;I<8;I++){
            const FVector At(105*FMath::Cos(I*PI/4),Y+105*FMath::Sin(I*PI/4),1830);
            Append(Runes,crystal(7,35),At);Circle(Gold,At+FVector(0,0,5),9,.65);
        }
        Block(Gold,FVector(0,Y,2075),FVector(1.5,1.5,245),.5);
        Lamp(FVector(0,Y,1800),FLinearColor(1,.72,.48),600000,1150);
    }
    Block(Dark,FVector(0,2910,1250),FVector(180,85,12),4);
    for(int S:{-1,1})Block(Gold,FVector(S*135,2910,1179),FVector(8,65,63),3);
    for(int Side:{-1,1}){
        const FRotator Tilt(0,0,Side*9);
        Block(Books,FVector(Side*19,2890,1266),FVector(19,28,2),.8,Tilt);
        Block(Pages,FVector(Side*19,2890,1269),FVector(17.5,26,1),.6,Tilt);
        for(int Row=0;Row<11;Row++)Block(Dark,FVector(Side*19,2870+Row*3.5,1271),FVector(12,.25,.16),.08,Tilt);
    }
    // Closed toroidal inlays follow the stone surface, with a real tube profile.
    for(double R:{145.,235.,345.,490.})Circle(Gold,FVector(0,200,175),R,1.4);
    for(int I=0;I<36;I++){
        const double A=2*Pi*I/36;const FVector P(450*std::cos(A),200+450*std::sin(A),176);
        Block(Gold,P,FVector(2.5,I%3?11:23,1.8),1,FRotator(0,I*10,0));
    }
    for(double R:{160.,220.,275.,319.})Circle(Gold,FVector(0,890,560),R,2.1,FRotator(0,0,90));
    // Lens rings tilt in three independent planes. Their shared origin is stable.
    for(int I=0;I<3;I++){
        auto* Ring=Mesh(ring(193+I*40,2.8),FString::Printf(TEXT("OrbitalRing%d"),I),FLinearColor(.7,.27,1),.35,.24,720,false);
        Ring->SetRelativeLocation(FVector(0,875,560));Rings.Add(Ring);
    }
    // Door leaves have thickness and collision throughout their opening motion.
    for(int S:{-1,1}){
        auto* Door=Mesh(block({177,24,320},8),FString::Printf(TEXT("Door%d"),S),FLinearColor(.025,.04,.075),.5,.32,0,true);
        Door->SetRelativeLocation(FVector(S*181,900,490));Doors.Add(Door);
        Astral::Mesh Pattern;for(double R:{60.,110.,155.})Circle(Pattern,FVector(0,-26,0),R,1.1,FRotator(0,0,90));
        auto* Inlay=Mesh(Pattern,FString::Printf(TEXT("DoorInlay%d"),S),FLinearColor(.8,.49,.16),.85,.28,12,false);
        Inlay->AttachToComponent(Door,FAttachmentTransformRules::KeepRelativeTransform);Inlay->SetRelativeLocation(FVector::ZeroVector);
    }
    // Only the optical interface is planar. The view behind it is a second
    // rendering of this same three-dimensional world, not a background image.
    Astral::Mesh Window;Window.p.push_back({0,889,560});
    for(int I=0;I<80;I++){const double A=2*Pi*I/80;Window.p.push_back({312*std::cos(A),889,560+312*std::sin(A)});}
    for(int I=0;I<80;I++)Window.t.insert(Window.t.end(),{0,I+1,(I+1)%80+1});
    Aperture=Mesh(Window,TEXT("LiveArchiveAperture"),FLinearColor(.4,.1,1),0,1,0,false);Aperture->SetCastShadow(false);Aperture->SetVisibility(false);
    Circle(Runes,FVector(0,200,178),325,1.8);Circle(Runes,FVector(0,2490,1124),220,2.0);
    auto* RuneMesh=Mesh(Runes,TEXT("ActivationCircuit"),FLinearColor(.45,.16,1),.1,.35,150,false);RuneMaterial=Cast<UMaterialInstanceDynamic>(RuneMesh->GetMaterial(0));
    Mesh(Stone,TEXT("FoundationAndArchitecture"),FLinearColor(.25,.225,.20),.05,.62,0,true);
    Mesh(Dark,TEXT("ObsidianPaversAndShelves"),FLinearColor(.032,.043,.065),.22,.26,0,true);
    Mesh(Gold,TEXT("BrassInlays"),FLinearColor(.72,.43,.16),.85,.3,0,false);
    Mesh(Books,TEXT("ArchiveVolumes"),FLinearColor(.22,.065,.12),.05,.7,0,false);
    Mesh(BooksBlue,TEXT("ArchiveBlueVolumes"),FLinearColor(.035,.07,.13),.05,.6,0,false);
    Mesh(BooksGreen,TEXT("ArchiveGreenVolumes"),FLinearColor(.045,.1,.065),.05,.65,0,false);
    Mesh(BooksOchre,TEXT("ArchiveOchreVolumes"),FLinearColor(.25,.13,.035),.05,.6,0,false);
    Mesh(Pages,TEXT("OpenArchivePages"),FLinearColor(.65,.53,.32),0,.85,0,false);
    for(int S:{-1,1})Lamp(FVector(S*420,600,500),FLinearColor(1,.67,.38),450000,1050);
    Lamp(FVector(0,2600,1650),FLinearColor(1,.75,.50),350000,1400);
    Sign(FVector(0,-760,272),TEXT("ASTRALIS · DIE STERNENPFORTE"),28);
    Sign(FVector(0,490,276),TEXT("[E] Drei Ringe ausrichten"),19);
    Sign(FVector(0,2610,1270),TEXT("[E] Rückkehr  ·  [B] Bibliothek"),18);
    Sign(FVector(0,3040,1370),TEXT("[E] Archiv der Verbindungen lesen"),18);
    auto* Bounds=NewObject<UBoxComponent>(this,TEXT("ArchiveExposureBounds"));AddInstanceComponent(Bounds);Bounds->SetupAttachment(RootComponent);
    Bounds->SetRelativeLocation(FVector(0,2650,1550));Bounds->SetBoxExtent(FVector(1150,1050,900));Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);Bounds->RegisterComponent();
    auto* Exposure=NewObject<UPostProcessComponent>(this,TEXT("ArchiveEyeAdaptation"));AddInstanceComponent(Exposure);Exposure->SetupAttachment(Bounds);
    Exposure->bUnbound=false;Exposure->Priority=10;Exposure->BlendRadius=300;Exposure->BlendWeight=1;
    Exposure->Settings.bOverride_AutoExposureMethod=true;Exposure->Settings.AutoExposureMethod=AEM_Histogram;
    Exposure->Settings.bOverride_AutoExposureMinBrightness=true;Exposure->Settings.AutoExposureMinBrightness=8;
    Exposure->Settings.bOverride_AutoExposureMaxBrightness=true;Exposure->Settings.AutoExposureMaxBrightness=10;Exposure->RegisterComponent();
    UE_LOG(LogTemp,Display,TEXT("ASTRAL_BUILT seed=%u solid_triangles=%d lights=%d rings=%d reference_method=closed_solids"),Seed,SolidTriangles,Lamps.Num(),Rings.Num());
}

bool AHALVETHAstralSanctum::Transport(AHALVETHCharacter* P,bool Return){
    if(!P||TeleportCooldown>0)return false;
    const FVector Target=GetActorLocation()+(Return?FVector(0,230,270):FVector(0,2480,1220));
    FCollisionQueryParams Params(SCENE_QUERY_STAT(AstralArrival),false,P);
    const auto Shape=FCollisionShape::MakeCapsule(P->GetCapsuleComponent()->GetScaledCapsuleRadius(),P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    if(GetWorld()->OverlapBlockingTestByChannel(Target,FQuat::Identity,ECC_Pawn,Shape,Params)){
        UE_LOG(LogTemp,Warning,TEXT("ASTRAL_TRAVEL_BLOCKED destination=%s"),Return?TEXT("lower"):TEXT("archive"));return false;
    }
    if(P->IsAstralFlying())P->ToggleAstralFlight();
    FVector Velocity=P->GetCharacterMovement()->Velocity;Velocity.Z=FMath::Max(0.,Velocity.Z);
    P->SetActorLocation(Target,false,nullptr,ETeleportType::TeleportPhysics);P->GetCharacterMovement()->Velocity=Velocity;
    P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);TeleportCooldown=1.;
    if(auto* M=Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this)))M->Notify(Return?TEXT("Zurück an der Sternenpforte."):TEXT("Das Archiv ist offen. B öffnet deine Bücher."));
    UE_LOG(LogTemp,Display,TEXT("ASTRAL_TRAVEL destination=%s arrival_clear=true"),Return?TEXT("lower"):TEXT("archive"));return true;
}
bool AHALVETHAstralSanctum::Interact(AHALVETHCharacter* P){
    if(!P)return false;const FVector Local=P->GetActorLocation()-GetActorLocation();
    if(FVector::Dist2D(Local,FVector(0,2910,0))<190&&Local.Z>1000&&P->GetKnowledge()){
        P->GetCharacterMovement()->StopMovementImmediately();P->GetKnowledge()->OpenBook(0);return true;
    }
    if(FVector::Dist2D(Local,FVector(0,2480,0))<310&&Local.Z>1000)return Transport(P,true);
    if(FVector::Dist2D(Local,FVector(0,500,0))>480||Local.Z>500)return false;
    if(Gate.ready())return Transport(P,false);
    if(Gate.align()){
        if(auto* M=Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this)))M->Notify(FString::Printf(TEXT("Sternenpforte: %d / 3 Ringe ausgerichtet%s"),Gate.aligned,Gate.aligned==3?TEXT(". Die Pforte öffnet sich."):TEXT(".")));
        UE_LOG(LogTemp,Display,TEXT("ASTRAL_ALIGN aligned=%d"),Gate.aligned);
    }return true;
}
void AHALVETHAstralSanctum::CastFocus(AHALVETHCharacter* P){
    if(!P)return;
    if(!Focus){
        Astral::Mesh G;Circle(G,{},12,.16);Circle(G,{},9,.12,FRotator(35,0,0));
        for(int I=0;I<6;I++)Append(G,Astral::crystal(.6,2),FVector(10*FMath::Cos(I*PI/3),10*FMath::Sin(I*PI/3),0));
        Focus=Mesh(G,TEXT("CastBoundFocus"),FLinearColor(.6,.2,1),.1,.3,800,false);
        if(auto* Camera=P->FindComponentByClass<UCameraComponent>()){Focus->AttachToComponent(Camera,FAttachmentTransformRules::KeepRelativeTransform);Focus->SetRelativeLocation(FVector(48,26,-18));Focus->SetRelativeRotation(FRotator(0,90,20));}
        FocusMaterial=Cast<UMaterialInstanceDynamic>(Focus->GetMaterial(0));
    }
    FocusRemaining=.85;Focus->SetVisibility(true);
    if(FocusMaterial)FocusMaterial->SetVectorParameterValue(TEXT("Tint"),P->GetAdventure()->GetSelectedAbility()==1?FLinearColor(1,.36,.08):FLinearColor(.5,.13,1));
}
void AHALVETHAstralSanctum::UpdateTrail(){
    if(Time-TrailSampleAt<.05)return;TrailSampleAt=Time;
    while(TrailTimes.Num()&&Time-TrailTimes[0]>1.2){TrailTimes.RemoveAt(0);TrailPoints.RemoveAt(0);}
    auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    if(P&&P->IsAstralFlying()){
        const FVector At=P->GetActorLocation()-GetActorLocation()-FVector(0,0,20);
        // A teleport is a discontinuity, never an invented luminous flight path.
        if(TrailPoints.Num()&&FVector::Dist(TrailPoints.Last(),At)>300){TrailPoints.Reset();TrailTimes.Reset();}
        if(!TrailPoints.Num()||FVector::Dist(TrailPoints.Last(),At)>5){TrailPoints.Add(At);TrailTimes.Add(Time);}
    }
    if(TrailPoints.Num()<2){if(FlightTrail)FlightTrail->SetVisibility(false);return;}
    std::vector<Astral::V> Path;for(auto Pnt:TrailPoints)Path.push_back({Pnt.X,Pnt.Y,Pnt.Z});
    auto Geometry=Astral::strand(Path,2.7);
    if(!FlightTrail)FlightTrail=Mesh(Geometry,TEXT("ObservedFlightPath"),FLinearColor(.38,.09,1),.05,.4,1100,false);
    else UpdateGeometry(FlightTrail,Geometry,false);
    FlightTrail->SetVisibility(true);
}
void AHALVETHAstralSanctum::UpdateAperture(){
    if(!Aperture)return;
    auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    const FVector Local=P?P->GetActorLocation()-GetActorLocation():FVector(0,0,9999);
    const bool Visible=Gate.openness>.85&&Local.Y<880&&Local.Z<850&&Local.Size2D()<2200;
    Aperture->SetVisibility(Visible&&PortalTarget!=nullptr);
    if(!Visible||!FApp::CanEverRender()||Time-PortalSampleAt<1./12)return;
    auto* Camera=P->FindComponentByClass<UCameraComponent>();if(!Camera)return;
    if(!PortalCamera){
        PortalTarget=NewObject<UTextureRenderTarget2D>(this,TEXT("ArchiveViewTarget"));PortalTarget->RenderTargetFormat=RTF_RGBA16f;PortalTarget->Filter=TF_Bilinear;PortalTarget->ClearColor=FLinearColor::Black;PortalTarget->InitAutoFormat(960,540);PortalTarget->UpdateResourceImmediate(true);
        PortalCamera=NewObject<USceneCaptureComponent2D>(this,TEXT("ArchiveViewCamera"));AddInstanceComponent(PortalCamera);PortalCamera->SetupAttachment(RootComponent);
        PortalCamera->TextureTarget=PortalTarget;PortalCamera->bCaptureEveryFrame=false;PortalCamera->bCaptureOnMovement=false;PortalCamera->bAlwaysPersistRenderingState=true;
        PortalCamera->CaptureSource=SCS_SceneColorHDRNoAlpha;PortalCamera->MaxViewDistanceOverride=6000;PortalCamera->ShowFlags.SetDynamicShadows(false);PortalCamera->ShowFlags.SetVolumetricFog(false);PortalCamera->ShowFlags.SetLumenGlobalIllumination(false);PortalCamera->ShowFlags.SetLumenReflections(false);
        PortalCamera->HideComponent(Aperture);PortalCamera->RegisterComponent();
        auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_AstralAperture.M_AstralAperture"));
        if(!Base){UE_LOG(LogTemp,Error,TEXT("ASTRAL_APERTURE_MATERIAL_MISSING"));PortalCamera->TextureTarget=nullptr;return;}
        auto* Material=UMaterialInstanceDynamic::Create(Base,Aperture);Material->SetTextureParameterValue(TEXT("ArchiveView"),PortalTarget);Aperture->SetMaterial(0,Material);
    }
    if(!PortalCamera->TextureTarget){Aperture->SetVisibility(false);return;}
    int32 W=2560,H=1440;if(auto* PC=GetWorld()->GetFirstPlayerController())PC->GetViewportSize(W,H);
    const int32 Height=FMath::Clamp(FMath::RoundToInt(960.*FMath::Max(1,H)/FMath::Max(1,W)),270,960);
    if(PortalTarget->SizeY!=Height)PortalTarget->ResizeTarget(960,Height);
    // Both apertures face +Y. A translation preserves the observer-to-point
    // vectors; screen-space UVs preserve the main camera's crop and aspect.
    const FVector Eye=Camera->GetComponentLocation();const auto Linked=Astral::linkedObserver({Eye.X,Eye.Y,Eye.Z},{0,889,560},{0,1960,1500});
    PortalCamera->SetWorldLocation(FVector(Linked.x,Linked.y,Linked.z));PortalCamera->SetWorldRotation(Camera->GetComponentRotation());PortalCamera->FOVAngle=Camera->FieldOfView;
    PortalCamera->CaptureScene();PortalSampleAt=Time;PortalFrames++;Aperture->SetVisibility(true);
}
void AHALVETHAstralSanctum::Tick(float Dt){
    Super::Tick(Dt);if(!FMath::IsFinite(Dt)||Dt<=0)return;Time+=Dt;Gate.tick(Dt);TeleportCooldown=FMath::Max(0.,TeleportCooldown-Dt);
    for(int I=0;I<Rings.Num();I++){
        const double Aligned=Gate.aligned>I?1:0;const double Spin=Aligned?0:Time*(14+I*8);
        Rings[I]->SetRelativeRotation(FRotator(Aligned?0:28*FMath::Sin(Time*.4+I),Spin,90+I*21*(1-Aligned)));
    }
    for(int I=0;I<Doors.Num();I++)Doors[I]->SetRelativeLocation(FVector((I==0?-1:1)*(181+380*Gate.openness),900,490));
    if(RuneMaterial)RuneMaterial->SetScalarParameterValue(TEXT("Glow"),(150+Gate.openness*650)*RadianceScale);
    FocusRemaining=FMath::Max(0.,FocusRemaining-Dt);
    if(Focus){Focus->SetVisibility(FocusRemaining>0);Focus->AddLocalRotation(FRotator(0,0,Dt*100));if(FocusMaterial)FocusMaterial->SetScalarParameterValue(TEXT("Glow"),800*FMath::Min(1.,FocusRemaining*3)*RadianceScale);}
    UpdateTrail();
    UpdateAperture();
    if(FParse::Param(FCommandLine::Get(),TEXT("HalvethAstralProbe"))&&!ProbeFinished){
        if(Time>.5&&Time<5)if(auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0)))P->AddMovementInput(FVector(0,1,0),1);
        if(Time>=5&&!ProbeGroundPassed){
            ProbeGroundPassed=RunProbe(true);
            if(!ProbeGroundPassed){ProbeFinished=true;UE_LOG(LogTemp,Error,TEXT("ASTRAL_NATIVE_PROBE result=FAIL"));FPlatformMisc::RequestExitWithStatus(false,3);return;}
            auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));Gate.aligned=3;Gate.openness=1;P->ToggleAstralFlight();ProbeFlightStart=P->GetActorLocation();
        }
        if(ProbeGroundPassed&&Time<7)if(auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0)))P->AddMovementInput(FVector(0,1,.6).GetSafeNormal(),1);
        if(ProbeGroundPassed&&Time>=7){
            auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));const FVector Delta=P->GetActorLocation()-ProbeFlightStart;
            const bool Flew=P->IsAstralFlying()&&P->GetCharacterMovement()->IsFlying()&&Delta.Y>400&&Delta.Z>150&&TrailPoints.Num()>2;
            P->ToggleAstralFlight();const bool Falling=!P->IsAstralFlying()&&P->GetCharacterMovement()->IsFalling();
            UE_LOG(LogTemp,Display,TEXT("ASTRAL_FLIGHT_PROBE passed=%d forward_cm=%.3f rise_cm=%.3f path_samples=%d gravity_restored=%d"),Flew&&Falling,Delta.Y,Delta.Z,TrailPoints.Num(),Falling);
            ProbeFinished=true;UE_LOG(LogTemp,Display,TEXT("ASTRAL_NATIVE_PROBE result=%s"),Flew&&Falling?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,Flew&&Falling?0:3);
        }
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("HalvethAstralCapture")))Capture();
}
bool AHALVETHAstralSanctum::VerifyStructure() const{
    FCollisionQueryParams Q(SCENE_QUERY_STAT(AstralFloor),false);int Hits=0;
    for(double Y:{-1370.,-1080.,-820.,0.,600.,1500.,2480.,2900.}){
        FHitResult H;const FVector Top=GetActorLocation()+FVector(0,Y,2200);
        if(GetWorld()->LineTraceSingleByChannel(H,Top,Top-FVector(0,0,4000),ECC_Visibility,Q)&&H.GetActor()==this&&H.ImpactNormal.Z>.8)Hits++;
    }
    UE_LOG(LogTemp,Display,TEXT("ASTRAL_FLOOR_CHECK supported=%d expected=8 solid_triangles=%d"),Hits,SolidTriangles);
    return Hits==8&&SolidTriangles>1000&&Rings.Num()==3&&Doors.Num()==2;
}
bool AHALVETHAstralSanctum::RunProbe(bool RequireTraversal){
    auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));if(!P||!VerifyStructure())return false;
    const FVector Local=P->GetActorLocation()-GetActorLocation();
    const bool Traversed=Local.Y>0&&Local.Y<900&&Local.Z>250&&Local.Z<330&&P->GetCharacterMovement()->IsMovingOnGround();
    if(RequireTraversal){
        auto* Movement=P->GetCharacterMovement();
        UE_LOG(LogTemp,Display,TEXT("ASTRAL_TRAVERSAL passed=%d local_y=%.3f local_z=%.3f walking=%d max_step=%.3f floor_dist=%.3f floor_normal=%s"),Traversed,Local.Y,Local.Z,Movement->IsMovingOnGround(),Movement->MaxStepHeight,Movement->CurrentFloor.FloorDist,*Movement->CurrentFloor.HitResult.ImpactNormal.ToString());
        if(!Traversed){
            FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(AstralStepDebug),false,P);
            auto Shape=FCollisionShape::MakeCapsule(34,88);
            const FVector Here=P->GetActorLocation();
            for(double Up:{0.,45.}){
                const bool Blocked=GetWorld()->SweepSingleByChannel(Hit,Here+FVector(0,0,Up),Here+FVector(0,70,Up),FQuat::Identity,ECC_Pawn,Shape,Query);
                UE_LOG(LogTemp,Display,TEXT("ASTRAL_STEP_DIAGNOSTIC up=%.3f blocked=%d component=%s normal=%s impact_local_z=%.3f start_penetrating=%d component_step=%d actor_base=%d"),Up,Blocked,*GetNameSafe(Hit.GetComponent()),*Hit.ImpactNormal.ToString(),Hit.ImpactPoint.Z-GetActorLocation().Z,Hit.bStartPenetrating,Hit.GetComponent()?Hit.GetComponent()->CanCharacterStepUp(P):false,Hit.GetActor()?Hit.GetActor()->CanBeBaseForCharacter(P):false);
            }return false;
        }
    }
    Gate=Astral::Gate{};Gate.align();Gate.tick(.6);Gate.align();Gate.tick(.6);Gate.align();Gate.tick(2);
    bool Ok=Gate.ready()&&Transport(P,false);
    P->SetActorLocation(GetActorLocation()+FVector(0,2760,1220),false,nullptr,ETeleportType::TeleportPhysics);
    const bool Read=Interact(P)&&P->GetKnowledge()->IsReading();if(P->GetKnowledge()->IsReading())P->GetKnowledge()->ToggleReading();
    UE_LOG(LogTemp,Display,TEXT("ASTRAL_BOOK_PROBE opened=%d closed=%d"),Read,!P->GetKnowledge()->IsReading());
    Ok=Read&&!P->GetKnowledge()->IsReading()&&Ok;TeleportCooldown=0;Ok=Transport(P,true)&&Ok;Gate=Astral::Gate{};
    UE_LOG(LogTemp,Display,TEXT("ASTRAL_PORTAL_PROBE result=%s"),Ok?TEXT("PASS"):TEXT("FAIL"));return Ok;
}

void AHALVETHAstralSanctum::Capture(){
    const double Now=FPlatformTime::Seconds();if(LastFrameAt>0&&Time>6&&Time<33)CaptureFrameMilliseconds.Add((Now-LastFrameAt)*1000);LastFrameAt=Now;
    const double Times[]={5,10,12,16,18,19,20,24,26,30,31,31.2,33,36,36.3,38};
    if(CaptureStep==13)if(auto* Flying=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0)))Flying->AddMovementInput(FVector(0,1,.6).GetSafeNormal(),1,true);
    if(CaptureStep>=UE_ARRAY_COUNT(Times)||Time<Times[CaptureStep])return;
    auto* P=Cast<AHALVETHCharacter>(UGameplayStatics::GetPlayerPawn(this,0));auto* PC=GetWorld()->GetFirstPlayerController();if(!P||!PC)return;
    auto View=[this,P,PC](FVector At,FVector Target){P->GetCharacterMovement()->SetMovementMode(MOVE_Flying);P->SetActorLocation(GetActorLocation()+At,false,nullptr,ETeleportType::TeleportPhysics);PC->SetControlRotation((Target-At-FVector(0,0,66)).Rotation());};
    auto Shot=[this](const TCHAR* Name){IFileManager::Get().MakeDirectory(*(FPaths::ProjectDir()/TEXT("Preview")),true);FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/TEXT("Preview")/(FString(Name)+TEXT(".png")),false,false);UE_LOG(LogTemp,Display,TEXT("ASTRAL_CAPTURE %s"),Name);};
    switch(CaptureStep){
        case 0:P->DisableInput(PC);PC->SetIgnoreLookInput(true);PC->SetIgnoreMoveInput(true);View(FVector(-1000,-2100,440),FVector(0,1300,800));break;
        case 1:Shot(TEXT("ASTRAL_APPROACH"));break;
        case 2:View(FVector(440,-130,240),FVector(0,900,560));break;
        case 3:Shot(TEXT("ASTRAL_CLOSED"));break;
        case 4:Gate.align();break;
        case 5:Gate.align();break;
        case 6:Gate.align();break;
        case 7:Shot(TEXT("ASTRAL_OPEN"));break;
        case 8:View(FVector(-510,2050,1290),FVector(410,3000,1450));break;
        case 9:Shot(TEXT("ASTRAL_ARCHIVE"));break;
        case 10:View(FVector(-250,100,250),FVector(0,900,700));CastFocus(P);break;
        case 11:Shot(TEXT("ASTRAL_FOCUS"));break;
        case 12:
            if(!RunProbe()){CaptureFailed=true;UE_LOG(LogTemp,Error,TEXT("ASTRAL_CAPTURE_PROBE_FAILED"));}
            Gate.aligned=3;Gate.openness=1;P->ToggleAstralFlight();break;
        case 13:{const FVector End=P->GetActorLocation()-GetActorLocation();P->ToggleAstralFlight();P->GetCharacterMovement()->StopMovementImmediately();View(End+FVector(-280,-650,200),End-FVector(0,300,50));break;}
        case 14:Shot(TEXT("ASTRAL_FLIGHT"));break;
        case 15:
            UE_LOG(LogTemp,Display,TEXT("ASTRAL_APERTURE_RENDER frames=%d width=%d height=%d max_hz=12 max_distance_cm=2200"),PortalFrames,PortalTarget?PortalTarget->SizeX:0,PortalTarget?PortalTarget->SizeY:0);
            if(PortalFrames<1)CaptureFailed=true;
            if(CaptureFrameMilliseconds.Num()>10){CaptureFrameMilliseconds.Sort();const int Count=CaptureFrameMilliseconds.Num();
                UE_LOG(LogTemp,Display,TEXT("ASTRAL_FRAME_SAMPLE frames=%d median_ms=%.3f p95_ms=%.3f scope=offscreen_capture_after_6s"),Count,CaptureFrameMilliseconds[Count/2],CaptureFrameMilliseconds[FMath::Min(Count-1,int(Count*.95))]);}
            UE_LOG(LogTemp,Display,TEXT("ASTRAL_CAPTURE result=%s"),CaptureFailed?TEXT("FAIL"):TEXT("PASS"));FPlatformMisc::RequestExitWithStatus(false,CaptureFailed?3:0);break;
    }CaptureStep++;
}
