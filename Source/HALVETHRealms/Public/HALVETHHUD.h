#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HALVETHHUD.generated.h"

UCLASS()
class HALVETHREALMS_API AHALVETHHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
