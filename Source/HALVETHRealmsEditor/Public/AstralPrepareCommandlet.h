#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "AstralPrepareCommandlet.generated.h"
UCLASS()
class UAstralPrepareCommandlet : public UCommandlet {
    GENERATED_BODY()
public:
    UAstralPrepareCommandlet();
    virtual int32 Main(const FString& Params) override;
};
