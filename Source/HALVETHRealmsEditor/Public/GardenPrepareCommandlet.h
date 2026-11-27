#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GardenPrepareCommandlet.generated.h"
UCLASS()
class UGardenPrepareCommandlet : public UCommandlet {
    GENERATED_BODY()
public:
    UGardenPrepareCommandlet();
    virtual int32 Main(const FString& Params) override;
};
