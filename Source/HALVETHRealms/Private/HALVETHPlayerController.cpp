// Copyright (c) 2026 Juri Janovski. MIT.
#include "HALVETHPlayerController.h"

AHALVETHPlayerController::AHALVETHPlayerController(){PlayerCameraManagerClass=AHALVETHPlayerCameraManager::StaticClass();}

void AHALVETHPlayerCameraManager::ProcessViewRotation(float DeltaTime,FRotator& View,FRotator& Delta){
    const FQuat Incoming=View.Quaternion();
    // Explicit controller changes (travel, capture, aim) rebase the frame.
    // Ordinary mouse input keeps its quaternion through every pole crossing.
    if(!Initialized||FMath::Abs(Incoming|LastOutput)<1.-1e-9){
        Frame.orientation={Incoming.W,Incoming.X,Incoming.Y,Incoming.Z};Initialized=true;
    }
    Frame.apply(Delta.Yaw,Delta.Pitch,Delta.Roll);
    const auto Q=Frame.orientation;LastOutput=FQuat(Q.x,Q.y,Q.z,Q.w);
    View=LastOutput.Rotator();Delta=FRotator::ZeroRotator;
}
