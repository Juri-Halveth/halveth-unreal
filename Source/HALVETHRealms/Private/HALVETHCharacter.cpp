#include "HALVETHCharacter.h"
#include "HALVETHGameMode.h"
#include "HALVETHAdventureComponent.h"
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
    Input->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
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
}

void AHALVETHCharacter::Forward(float Value) { AddMovementInput(GetActorForwardVector(), Value); }
void AHALVETHCharacter::Right(float Value) { AddMovementInput(GetActorRightVector(), Value); }
void AHALVETHCharacter::Turn(float Value) { AddControllerYawInput(Value); }
void AHALVETHCharacter::Look(float Value) { AddControllerPitchInput(Value); }
void AHALVETHCharacter::SprintOn() { GetCharacterMovement()->MaxWalkSpeed = 790; }
void AHALVETHCharacter::SprintOff() { GetCharacterMovement()->MaxWalkSpeed = 460; }
void AHALVETHCharacter::Interact()
{
    if (Adventure && Adventure->InteractWithNearbyCharacter()) return;
    if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) Mode->Interact(this);
}
void AHALVETHCharacter::Love()
{
    if (!Adventure) return;
    while (Adventure->GetSelectedAbility() != 0) Adventure->CycleAbility();
    CastSpell();
}
void AHALVETHCharacter::CastSpell()
{
    if (Adventure && Adventure->ActivateAbility() && Adventure->GetSelectedAbility() == 0)
        if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) Mode->CastLove();
}
void AHALVETHCharacter::NextSpell() { if (Adventure) Adventure->CycleAbility(); }
void AHALVETHCharacter::NextItem() { if (Adventure) Adventure->CycleItem(); }
void AHALVETHCharacter::UseItem() { if (Adventure) Adventure->UseItem(); }
void AHALVETHCharacter::Dodge() { if (Adventure) Adventure->Dodge(); }
void AHALVETHCharacter::ReturnHome()
{
    if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) Mode->Travel(0);
    ResetToSpawn();
}
void AHALVETHCharacter::QualityLow() { if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) Mode->ApplyQuality(0); }
void AHALVETHCharacter::QualityBalanced() { if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) Mode->ApplyQuality(1); }
void AHALVETHCharacter::QualityEpic() { if (auto* Mode = Cast<AHALVETHGameMode>(UGameplayStatics::GetGameMode(this))) Mode->ApplyQuality(2); }
void AHALVETHCharacter::ToggleHelp() { bHelpVisible = !bHelpVisible; }
void AHALVETHCharacter::ToggleCredits() { bCreditsVisible = !bCreditsVisible; }
void AHALVETHCharacter::Quit() { UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(Controller), EQuitPreference::Quit, false); }

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
