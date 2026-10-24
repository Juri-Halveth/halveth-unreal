#include "HALVETHCitizen.h"
#include "HALVETHTissueComponent.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
AHALVETHCitizen::AHALVETHCitizen(const FObjectInitializer& Initializer)
    :Super(Initializer.SetDefaultSubobjectClass<USkeletalMeshComponentBudgeted>(ACharacter::MeshComponentName)){
    GetCapsuleComponent()->InitCapsuleSize(28,92);GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
    // A person is an obstacle, not a staircase or moving floor.
    GetCapsuleComponent()->CanCharacterStepUpOn=ECB_No;
    GetMesh()->SetRelativeLocation(FVector(0,0,-94.15));GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* Move=GetCharacterMovement();Move->bRunPhysicsWithNoController=true;Move->MaxWalkSpeed=110;Move->MaxAcceleration=380;
    Move->BrakingDecelerationWalking=480;Move->MaxStepHeight=45;Move->SetWalkableFloorAngle(50);Move->bOrientRotationToMovement=true;Move->RotationRate=FRotator(0,220,0);
    bUseControllerRotationYaw=false;
    CreateDefaultSubobject<UHALVETHTissueComponent>(TEXT("TissueCarrier"));
}
