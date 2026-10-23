// Copyright (c) 2026 Juri Janovski. MIT.
#pragma once
#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "ViewSkyMath.h"
#include "HALVETHPlayerController.generated.h"

UCLASS()
class HALVETHREALMS_API AHALVETHPlayerCameraManager : public APlayerCameraManager {
    GENERATED_BODY()
public:
    virtual void ProcessViewRotation(float DeltaTime,FRotator& View,FRotator& Delta) override;
private:
    HalvethView::Frame Frame;
    FQuat LastOutput=FQuat::Identity;
    bool Initialized=false;
};

UCLASS()
class HALVETHREALMS_API AHALVETHPlayerController : public APlayerController {
    GENERATED_BODY()
public:
    AHALVETHPlayerController();
};
