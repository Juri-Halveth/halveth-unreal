#include "VoidCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"
AVoidCharacter::AVoidCharacter() {
    GetCapsuleComponent()->InitCapsuleSize(34, 88);
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Eyes"));
    Camera->SetupAttachment(GetCapsuleComponent()); Camera->SetRelativeLocation(FVector(0, 0, 64));
    Camera->bUsePawnControlRotation = true; Camera->FieldOfView = 85;
    bUseControllerRotationYaw = true;
    GetCharacterMovement()->MaxWalkSpeed = 440;
    GetCharacterMovement()->JumpZVelocity = 450;
    GetCharacterMovement()->MaxStepHeight = 48;
    GetCharacterMovement()->SetWalkableFloorAngle(47);
}
void AVoidCharacter::SetupPlayerInputComponent(UInputComponent* I) {
    Super::SetupPlayerInputComponent(I);
    I->BindAxis(TEXT("MoveForward"), this, &AVoidCharacter::Forward);
    I->BindAxis(TEXT("MoveRight"), this, &AVoidCharacter::Right);
    I->BindAxisKey(EKeys::MouseX, this, &AVoidCharacter::LookX);
    I->BindAxisKey(EKeys::MouseY, this, &AVoidCharacter::LookY);
    I->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ACharacter::Jump);
    I->BindKey(EKeys::SpaceBar, IE_Released, this, &ACharacter::StopJumping);
    I->BindKey(EKeys::LeftShift, IE_Pressed, this, &AVoidCharacter::Sprint);
    I->BindKey(EKeys::LeftShift, IE_Released, this, &AVoidCharacter::Walk);
    I->BindKey(EKeys::I, IE_Pressed, this, &AVoidCharacter::Inventory);
    I->BindKey(EKeys::Tab, IE_Pressed, this, &AVoidCharacter::Inventory);
    I->BindKey(EKeys::Escape, IE_Pressed, this, &AVoidCharacter::Quit);
}
void AVoidCharacter::Forward(float V) {
    if (bInventory || !Controller) return;
    AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X), V);
}
void AVoidCharacter::Right(float V) {
    if (bInventory || !Controller) return;
    AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), V);
}
void AVoidCharacter::LookX(float V) { if (!bInventory) AddControllerYawInput(V); }
void AVoidCharacter::LookY(float V) { if (!bInventory) AddControllerPitchInput(-V); }
void AVoidCharacter::Sprint() { GetCharacterMovement()->MaxWalkSpeed = 760; }
void AVoidCharacter::Walk() { GetCharacterMovement()->MaxWalkSpeed = 440; }
void AVoidCharacter::Inventory() {
    bInventory = !bInventory;
    if (APlayerController* PC = Cast<APlayerController>(Controller)) {
        PC->bShowMouseCursor = bInventory;
        if (bInventory) { FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); PC->SetInputMode(Mode); }
        else { PC->SetInputMode(FInputModeGameOnly()); }
    }
}
void AVoidCharacter::Quit() { UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(Controller), EQuitPreference::Quit, false); }
