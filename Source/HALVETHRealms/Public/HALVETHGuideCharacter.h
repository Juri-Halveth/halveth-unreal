#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HALVETHGuideCharacter.generated.h"

// A colliding, gravity-driven character; animation follows actual movement.
UCLASS()
class HALVETHREALMS_API AHALVETHGuideCharacter : public ACharacter {
    GENERATED_BODY()
public:
    AHALVETHGuideCharacter();
};
