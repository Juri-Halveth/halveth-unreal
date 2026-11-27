#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VoidHUD.generated.h"
UCLASS()
class HALVETHVOID_API AVoidHUD : public AHUD {
    GENERATED_BODY()
public: virtual void DrawHUD() override;
};
