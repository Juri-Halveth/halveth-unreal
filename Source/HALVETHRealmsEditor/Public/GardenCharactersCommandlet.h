#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GardenCharactersCommandlet.generated.h"
UCLASS()
class UGardenCharactersCommandlet : public UCommandlet {
    GENERATED_BODY()
public:
    UGardenCharactersCommandlet();
    virtual int32 Main(const FString& Params) override;
};
