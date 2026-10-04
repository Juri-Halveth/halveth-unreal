#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoidSoul.h"
#include "VoidWorld.generated.h"

UCLASS()
class HALVETHVOID_API AVoidWorld : public AActor {
    GENERATED_BODY()
public:
    AVoidWorld();
    FVoidSoul Soul;
    int32 TreeCount = 0, RockCount = 0, FernCount = 0;
    bool bTerrainReady = false;
    class UStaticMeshComponent* GetTerrain() const { return Terrain; }
protected:
    virtual void BeginPlay() override;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Terrain;
    void AddNature();
    void AddAtmosphere();
};
