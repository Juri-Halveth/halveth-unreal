#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "HALVETHPrepareCommandlet.generated.h"

UCLASS()
class UHALVETHPrepareCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UHALVETHPrepareCommandlet();
    virtual int32 Main(const FString& Params) override;
};
