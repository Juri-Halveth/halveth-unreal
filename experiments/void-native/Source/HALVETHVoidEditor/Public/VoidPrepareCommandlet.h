#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "VoidPrepareCommandlet.generated.h"
UCLASS()
class UVoidPrepareCommandlet : public UCommandlet {
    GENERATED_BODY()
public:
    UVoidPrepareCommandlet();
    virtual int32 Main(const FString& Params) override;
};
