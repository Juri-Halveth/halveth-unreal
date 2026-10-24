// Copyright (c) 2026 Juri Janovski. SPDX-License-Identifier: LicenseRef-HALVETH-PIRL-2.0
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HALVETHMeadow.generated.h"

class AHALVETHRealmWorld;
class UInstancedStaticMeshComponent;

// A bounded sliding window. Slots are recycled by world-cell identity; the
// player does not carry a rotating carpet of plants around the landscape.
UCLASS()
class HALVETHREALMS_API AHALVETHMeadow : public AActor {
    GENERATED_BODY()
public:
    AHALVETHMeadow();
    void Initialize(AHALVETHRealmWorld* Source);
    virtual void Tick(float Dt) override;
    int32 GetRootedClumps() const { return RootedClumps; }
    int32 GetSlotBudget() const { return Grid*Grid*4; }
private:
    static constexpr int32 Grid=31;
    UPROPERTY() TObjectPtr<AHALVETHRealmWorld> Field;
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Groups;
    TArray<FIntPoint> Cells;
    TArray<int32> LivePerCell;
    TArray<TArray<float>> CellPhases;
    int32 Realm=-1,Cursor=0,RootedClumps=0;
    bool Announced=false;
    bool CaptureMode=false,CaptureDone=false,Sequence=false,Benchmark=false;
    int32 SequenceFrame=-1;
    double CaptureAge=0,LastFrame=0;
    int32 CaptureStage=-1;
    TArray<double> FrameMs;
    void RefreshCell(int32 Slot,FIntPoint Cell);
    void TickCapture(float Dt);
    bool ExportScene();
};
