#include "HALVETHCharacter.h"
#include "HALVETHGameMode.h"
#include "HALVETHAdventureComponent.h"
#include "HALVETHKnowledgeComponent.h"
#include "HALVETHRealmWorld.h"
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
    Input->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    Input->BindAction(TEXT("Sprint"), IE_Pressed, this, &AHALVETHCharacter::SprintOn);
    Input->BindAction(TEXT("Sprint"), IE_Released, this, &AHALVETHCharacter::SprintOff);
    Input->BindAction(TEXT("Interact"), IE_Pressed, this, &AHALVETHCharacter::Interact);
    Input->BindAction(TEXT("Love"), IE_Pressed, this, &AHALVETHCharacter::Love);
    Input->BindAction(TEXT("CastSpell"), IE_Pressed, this, &AHALVETHCharacter::CastSpell);
    Input->BindAction(TEXT("NextSpell"), IE_Pressed, this, &AHALVETHCharacter::NextSpell);
    Input->BindAction(TEXT("NextItem"), IE_Pressed, this, &AHALVETHCharacter::NextItem);
    Input->BindAction(TEXT("UseItem"), IE_Pressed, this, &AHALVETHCharacter::UseItem);
    Input->BindAction(TEXT("Dodge"), IE_Pressed, this, &AHALVETHCharacter::Dodge);
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
void AHALVETHCharacter::Forward(float Value) { if (!IsReading()) AddMovementInput(GetActorForwardVector(), Value); }
void AHALVETHCharacter::Right(float Value) { if (!IsReading()) AddMovementInput(GetActorRightVector(), Value); }
void AHALVETHCharacter::Turn(float Value) { if (!IsReading()) AddControllerYawInput(Value); }
void AHALVETHCharacter::Look(float Value) { if (!IsReading()) AddControllerPitchInput(Value); }
void AHALVETHCharacter::StartJump() { if (!IsReading()) Jump(); }
void AHALVETHCharacter::SprintOn() { if (!IsReading()) GetCharacterMovement()->MaxWalkSpeed = 790; }
void AHALVETHCharacter::SprintOff() { GetCharacterMovement()->MaxWalkSpeed = 460; }
void AHALVETHCharacter::Interact()
{
    if (IsReading()) return;
    if (Knowledge && Knowledge->GatherNearby()) return;
    if (Knowledge)
        if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this)))
            if (Mode->GetRealmWorld())
            {
                const int32 Book = Mode->GetRealmWorld()->FindReadable(GetActorLocation());
                if (Book != INDEX_NONE) { Knowledge->OpenBook(Book); GetCharacterMovement()->StopMovementImmediately(); SprintOff(); return; }
            }
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
    if (Adventure && Adventure->ActivateAbility() && Adventure->GetSelectedAbility() == 0)
        if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) Mode->CastLove();
}
void AHALVETHCharacter::NextSpell() { if (Adventure && !IsReading()) Adventure->CycleAbility(); }
void AHALVETHCharacter::NextItem() { if (Adventure && !IsReading()) Adventure->CycleItem(); }
void AHALVETHCharacter::UseItem() { if (Adventure && !IsReading()) { Adventure->UseItem(); if (Knowledge) Knowledge->SaveProgress(); } }
void AHALVETHCharacter::Dodge() { if (Adventure && !IsReading()) Adventure->Dodge(); }
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
    GetCharacterMovement()->StopMovementImmediately();
    SetActorLocation(FVector(0, -1050, 115), false, nullptr, ETeleportType::TeleportPhysics);
    SetActorRotation(FRotator(0, 90, 0));
    if (Controller) Controller->SetControlRotation(FRotator(0, 90, 0));
}

void AHALVETHCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (GetActorLocation().Z < -850) ResetToSpawn();
}
