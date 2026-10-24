#include "HALVETHCharacter.h"
#include "HALVETHGameMode.h"
#include "HALVETHAdventureComponent.h"
#include "HALVETHKnowledgeComponent.h"
#include "HALVETHRealmWorld.h"
#include "HALVETHAstralSanctum.h"
#include "HALVETHLivingWorld.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

AHALVETHCharacter::AHALVETHCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    Adventure = CreateDefaultSubobject<UHALVETHAdventureComponent>(TEXT("Adventure"));
    Knowledge = CreateDefaultSubobject<UHALVETHKnowledgeComponent>(TEXT("Knowledge"));
    GetCapsuleComponent()->InitCapsuleSize(34, 88);
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(GetCapsuleComponent());
    Camera->SetRelativeLocation(FVector(0, 0, 66));
    Camera->bUsePawnControlRotation = true;
    Camera->FieldOfView = 85;
    GetCharacterMovement()->MaxWalkSpeed = 460;
    GetCharacterMovement()->JumpZVelocity = 500;
    GetCharacterMovement()->AirControl = 0.2f;
    bUseControllerRotationYaw = true;
}

void AHALVETHCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"), this, &AHALVETHCharacter::Forward);
    Input->BindAxis(TEXT("MoveRight"), this, &AHALVETHCharacter::Right);
    Input->BindAxis(TEXT("Turn"), this, &AHALVETHCharacter::Turn);
    Input->BindAxis(TEXT("LookUp"), this, &AHALVETHCharacter::Look);
    Input->BindAction(TEXT("Jump"), IE_Pressed, this, &AHALVETHCharacter::StartJump);
    Input->BindAction(TEXT("Jump"), IE_Released, this, &AHALVETHCharacter::StopJumpOrAscend);
    Input->BindAction(TEXT("Sprint"), IE_Pressed, this, &AHALVETHCharacter::SprintOn);
    Input->BindAction(TEXT("Sprint"), IE_Released, this, &AHALVETHCharacter::SprintOff);
    Input->BindAction(TEXT("Interact"), IE_Pressed, this, &AHALVETHCharacter::Interact);
    Input->BindAction(TEXT("Love"), IE_Pressed, this, &AHALVETHCharacter::Love);
    Input->BindAction(TEXT("CastSpell"), IE_Pressed, this, &AHALVETHCharacter::CastSpell);
    Input->BindAction(TEXT("NextSpell"), IE_Pressed, this, &AHALVETHCharacter::NextSpell);
    Input->BindAction(TEXT("NextItem"), IE_Pressed, this, &AHALVETHCharacter::NextItem);
    Input->BindAction(TEXT("UseItem"), IE_Pressed, this, &AHALVETHCharacter::UseItem);
    Input->BindAction(TEXT("Dodge"), IE_Pressed, this, &AHALVETHCharacter::Dodge);
    Input->BindAction(TEXT("Dodge"), IE_Released, this, &AHALVETHCharacter::StopDescend);
    Input->BindAction(TEXT("AstralFlight"), IE_Pressed, this, &AHALVETHCharacter::FlightToggle);
    Input->BindAction(TEXT("Return"), IE_Pressed, this, &AHALVETHCharacter::ReturnHome);
    Input->BindAction(TEXT("QualityLow"), IE_Pressed, this, &AHALVETHCharacter::QualityLow);
    Input->BindAction(TEXT("QualityBalanced"), IE_Pressed, this, &AHALVETHCharacter::QualityBalanced);
    Input->BindAction(TEXT("QualityEpic"), IE_Pressed, this, &AHALVETHCharacter::QualityEpic);
    Input->BindAction(TEXT("ToggleHelp"), IE_Pressed, this, &AHALVETHCharacter::ToggleHelp);
    Input->BindAction(TEXT("ToggleCredits"), IE_Pressed, this, &AHALVETHCharacter::ToggleCredits);
    Input->BindAction(TEXT("Quit"), IE_Pressed, this, &AHALVETHCharacter::Quit);
    Input->BindAction(TEXT("Library"), IE_Pressed, this, &AHALVETHCharacter::ToggleLibrary);
    Input->BindAction(TEXT("ReaderPanel"), IE_Pressed, this, &AHALVETHCharacter::CycleReaderPanel);
    Input->BindAction(TEXT("NextBook"), IE_Pressed, this, &AHALVETHCharacter::NextBook);
    Input->BindAction(TEXT("NextPage"), IE_Pressed, this, &AHALVETHCharacter::NextPage);
    Input->BindAction(TEXT("PreviousPage"), IE_Pressed, this, &AHALVETHCharacter::PreviousPage);
    Input->BindAction(TEXT("CycleRecipe"), IE_Pressed, this, &AHALVETHCharacter::CycleRecipe);
    Input->BindAction(TEXT("Craft"), IE_Pressed, this, &AHALVETHCharacter::CraftSelected);
    Input->BindAction(TEXT("PackDraught"), IE_Pressed, this, &AHALVETHCharacter::PackDraught);
    Input->BindAction(TEXT("CycleStructure"), IE_Pressed, this, &AHALVETHCharacter::CycleStructure);
    Input->BindAction(TEXT("Build"), IE_Pressed, this, &AHALVETHCharacter::BuildSelected);
    Input->BindAction(TEXT("CycleQuest"), IE_Pressed, this, &AHALVETHCharacter::CycleQuest);
    Input->BindAction(TEXT("QuestOne"), IE_Pressed, this, &AHALVETHCharacter::QuestOne);
    Input->BindAction(TEXT("QuestTwo"), IE_Pressed, this, &AHALVETHCharacter::QuestTwo);
    Input->BindAction(TEXT("QuestThree"), IE_Pressed, this, &AHALVETHCharacter::QuestThree);
    Input->BindAction(TEXT("SaveKnowledge"), IE_Pressed, this, &AHALVETHCharacter::SaveKnowledge);
    Input->BindAction(TEXT("LoadKnowledge"), IE_Pressed, this, &AHALVETHCharacter::LoadKnowledge);
}

bool AHALVETHCharacter::IsReading() const { return Knowledge && Knowledge->IsReading(); }
void AHALVETHCharacter::Forward(float Value) { if (!IsReading()&&!IsCivicHeld()) AddMovementInput(bAstralFlight?Camera->GetForwardVector():GetActorForwardVector(), Value); }
void AHALVETHCharacter::Right(float Value) { if (!IsReading()&&!IsCivicHeld()) AddMovementInput(GetActorRightVector(), Value); }
void AHALVETHCharacter::Turn(float Value) { if (!IsReading()) AddControllerYawInput(Value); }
void AHALVETHCharacter::Look(float Value) { if (!IsReading()) AddControllerPitchInput(Value); }
void AHALVETHCharacter::StartJump() { if (!IsReading()&&!IsCivicHeld()) {if(bAstralFlight)bFlightAscend=true;else Jump();} }
void AHALVETHCharacter::StopJumpOrAscend(){bFlightAscend=false;StopJumping();}
void AHALVETHCharacter::StopDescend(){bFlightDescend=false;}
void AHALVETHCharacter::SprintOn() { if (!IsReading()){GetCharacterMovement()->MaxWalkSpeed = 790;GetCharacterMovement()->MaxFlySpeed=1700;} }
void AHALVETHCharacter::SprintOff() { GetCharacterMovement()->MaxWalkSpeed = 460;GetCharacterMovement()->MaxFlySpeed=1150; }
void AHALVETHCharacter::Interact()
{
    if (IsReading()) return;
    if(auto* Mode=Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this)))
        if(Mode->GetRealmWorld())if(auto* Sanctum=Mode->GetRealmWorld()->GetAstralSanctum())if(Sanctum->Interact(this))return;
    if (Knowledge && Knowledge->GatherNearby()) return;
    if (Knowledge)
        if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this)))
            if (Mode->GetRealmWorld())
            {
                const int32 Book = Mode->GetRealmWorld()->FindReadable(GetActorLocation());
                if (Book != INDEX_NONE) { Knowledge->OpenBook(Book); GetCharacterMovement()->StopMovementImmediately(); SprintOff(); return; }
            }
    if(auto* Mode=Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this)))if(Mode->GetLivingWorld()&&Mode->GetLivingWorld()->Interact(this))return;
    if (Adventure && Adventure->InteractWithNearbyCharacter()) return;
    if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) Mode->Interact(this);
}
void AHALVETHCharacter::Love()
{
    if (!Adventure || IsReading()) return;
    while (Adventure->GetSelectedAbility() != 0) Adventure->CycleAbility();
    CastSpell();
}
void AHALVETHCharacter::CastSpell()
{
    if (IsReading()) return;
    if (Adventure && Adventure->ActivateAbility())
        if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) {
            if(Adventure->GetSelectedAbility()==0)Mode->CastLove();
            if(Mode->GetRealmWorld())if(auto* Sanctum=Mode->GetRealmWorld()->GetAstralSanctum())Sanctum->CastFocus(this);
        }
}
void AHALVETHCharacter::NextSpell() { if (Adventure && !IsReading()) Adventure->CycleAbility(); }
void AHALVETHCharacter::NextItem() { if (Adventure && !IsReading()) Adventure->CycleItem(); }
void AHALVETHCharacter::UseItem() { if (Adventure && !IsReading()) { Adventure->UseItem(); if (Knowledge) Knowledge->SaveProgress(); } }
void AHALVETHCharacter::Dodge() { if (Adventure && !IsReading()&&!IsCivicHeld()){if(bAstralFlight)bFlightDescend=true;else Adventure->Dodge();} }
void AHALVETHCharacter::FlightToggle(){if(!IsReading())ToggleAstralFlight();}
bool AHALVETHCharacter::ToggleAstralFlight(){
    auto* Mode=Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this));
    if(IsCivicHeld())return false;
    bAstralFlight=!bAstralFlight;bFlightAscend=bFlightDescend=false;
    auto* Move=GetCharacterMovement();Move->MaxFlySpeed=1150;Move->BrakingDecelerationFlying=650;
    Move->SetMovementMode(bAstralFlight?MOVE_Flying:MOVE_Falling);
    if(Mode)Mode->Notify(bAstralFlight?TEXT("Levitation: WASD + Blickrichtung · Leertaste auf · Strg ab · J beendet"):TEXT("Levitation beendet. Die Schwerkraft trägt dich zurück."));
    UE_LOG(LogTemp,Display,TEXT("ASTRAL_FLIGHT active=%d"),bAstralFlight);return true;
}
void AHALVETHCharacter::ApplyCivicHold(float Seconds){
    if(!FMath::IsFinite(Seconds)||Seconds<=0)return;
    CivicHoldRemaining=FMath::Clamp(Seconds,0.f,2.2f);bFlightAscend=bFlightDescend=false;
    auto* Move=GetCharacterMovement();bHoldHover=Move->IsFalling()&&!bAstralFlight;
    if(bHoldHover)Move->SetMovementMode(MOVE_Flying);
    Move->StopMovementImmediately();ConsumeMovementInputVector();
}
void AHALVETHCharacter::ReturnHome()
{
    if (IsReading()) return;
    if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) Mode->Travel(0);
    ResetToSpawn();
}
void AHALVETHCharacter::QualityLow() { if (IsReading()) { Knowledge->RecallPage(0); return; } if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) Mode->ApplyQuality(0); }
void AHALVETHCharacter::QualityBalanced() { if (IsReading()) { Knowledge->RecallPage(1); return; } if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) Mode->ApplyQuality(1); }
void AHALVETHCharacter::QualityEpic() { if (IsReading()) { Knowledge->RecallPage(2); return; } if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) Mode->ApplyQuality(2); }
void AHALVETHCharacter::ToggleHelp() { bHelpVisible = !bHelpVisible; }
void AHALVETHCharacter::ToggleCredits() { bCreditsVisible = !bCreditsVisible; }
void AHALVETHCharacter::ToggleLibrary() { if (Knowledge) { Knowledge->ToggleReading(); GetCharacterMovement()->StopMovementImmediately(); SprintOff(); bCreditsVisible = false; } }
void AHALVETHCharacter::CycleReaderPanel() { if (IsReading()) ReaderPanel = (ReaderPanel + 1) % 4; }
void AHALVETHCharacter::NextBook() { if (IsReading()) Knowledge->NextBook(); }
void AHALVETHCharacter::NextPage() { if (IsReading()) Knowledge->NextPage(); }
void AHALVETHCharacter::PreviousPage() { if (IsReading()) Knowledge->PreviousPage(); }
void AHALVETHCharacter::CycleRecipe() { if (IsReading()) { ReaderPanel = 0; Knowledge->CycleRecipe(); } }
void AHALVETHCharacter::CraftSelected() { if (IsReading()) Knowledge->CraftSelected(); }
void AHALVETHCharacter::PackDraught() { if (IsReading()) { ReaderPanel = 0; Knowledge->PackDraught(); } }
void AHALVETHCharacter::CycleStructure() { if (Knowledge) { ReaderPanel = 1; Knowledge->CycleStructure(); } }
void AHALVETHCharacter::BuildSelected() { if (Knowledge) { if (IsReading()) { ToggleLibrary(); return; } Knowledge->BuildSelected(); } }
void AHALVETHCharacter::CycleQuest() { if (IsReading()) { ReaderPanel = 2; Knowledge->CycleQuest(); } }
void AHALVETHCharacter::QuestOne() { if (IsReading() && ReaderPanel == 2) Knowledge->ChooseQuest(0); }
void AHALVETHCharacter::QuestTwo() { if (IsReading() && ReaderPanel == 2) Knowledge->ChooseQuest(1); }
void AHALVETHCharacter::QuestThree() { if (IsReading() && ReaderPanel == 2) Knowledge->ChooseQuest(2); }
void AHALVETHCharacter::SaveKnowledge() { if (Knowledge) Knowledge->SaveProgress(); }
void AHALVETHCharacter::LoadKnowledge() { if (Knowledge) Knowledge->ReloadProgress(); }
void AHALVETHCharacter::Quit() { if (IsReading()) { ToggleLibrary(); return; } if (Knowledge) Knowledge->SaveProgress(); UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(Controller), EQuitPreference::Quit, false); }

void AHALVETHCharacter::ResetToSpawn()
{
    CivicHoldRemaining=0;bHoldHover=false;
    bAstralFlight=bFlightAscend=bFlightDescend=false;
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    SetActorLocation(FVector(0, -1050, 115), false, nullptr, ETeleportType::TeleportPhysics);
    SetActorRotation(FRotator(0, 90, 0));
    if (Controller) Controller->SetControlRotation(FRotator(0, 90, 0));
}

void AHALVETHCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(IsCivicHeld()){
        CivicHoldRemaining=FMath::Max(0.f,CivicHoldRemaining-DeltaSeconds);
        GetCharacterMovement()->StopMovementImmediately();ConsumeMovementInputVector();
        if(!IsCivicHeld()&&bHoldHover){bHoldHover=false;GetCharacterMovement()->SetMovementMode(MOVE_Falling);}
    }
    if(bAstralFlight&&!IsReading()&&!IsCivicHeld()){
        auto* Move=GetCharacterMovement();const double SpeedMps=Move->Velocity.Size()/100.;
        // Declared magical flight model: finite speed, quadratic drag, capsule
        // collision and bounded world extent. It is not a fluid simulation.
        Move->Velocity*=1./(1.+.018*SpeedMps*FMath::Max(0.f,DeltaSeconds));
        AddMovementInput(FVector::UpVector,(bFlightAscend?1.f:0.f)-(bFlightDescend?1.f:0.f));
        FVector At=GetActorLocation();FVector Bounded=At;Bounded.X=FMath::Clamp(At.X,-28500.,28500.);Bounded.Y=FMath::Clamp(At.Y,-28500.,28500.);Bounded.Z=FMath::Min(At.Z,15000.);
        if(!At.Equals(Bounded,.01))SetActorLocation(Bounded,true);
    }
    if(IsReading()&&bAstralFlight)GetCharacterMovement()->StopMovementImmediately();
    const float TargetFov=AppearanceInspectionFov>0?AppearanceInspectionFov:(bAstralFlight?85+float(FMath::Clamp(GetVelocity().Size()/1700.,0.,1.))*7:85);
    Camera->FieldOfView=FMath::FInterpTo(Camera->FieldOfView,TargetFov,DeltaSeconds,4);
    if (GetActorLocation().Z < -12000) ResetToSpawn();
}

void AHALVETHCharacter::SetAppearanceInspectionFov(float Degrees){
    AppearanceInspectionFov=FMath::IsFinite(Degrees)&&Degrees>=20&&Degrees<=100?Degrees:0;
    Camera->SetFieldOfView(AppearanceInspectionFov>0?AppearanceInspectionFov:85);
}
