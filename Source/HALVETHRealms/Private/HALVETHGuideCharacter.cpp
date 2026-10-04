#include "HALVETHGuideCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

AHALVETHGuideCharacter::AHALVETHGuideCharacter(){
    GetCapsuleComponent()->InitCapsuleSize(28,92);
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
    GetMesh()->SetRelativeLocation(FVector(0,0,-94.15));
    GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* Movement=GetCharacterMovement();
    Movement->GravityScale=1;
    Movement->bRunPhysicsWithNoController=true;
    Movement->MaxWalkSpeed=84;
    Movement->MaxAcceleration=260;
    Movement->BrakingDecelerationWalking=340;
    Movement->GroundFriction=3;
    Movement->MaxStepHeight=30;
    Movement->SetWalkableFloorAngle(46);
    Movement->bOrientRotationToMovement=false;
    bUseControllerRotationYaw=false;
}
