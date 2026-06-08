#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TKGameTypes.generated.h"
class ATKPlayerControllerBase;

USTRUCT(BlueprintType)
struct FEventContext
{
    GENERATED_BODY()
public:
    //事件Tag
    UPROPERTY(BlueprintReadOnly)
    FGameplayTag EventTag;
    UPROPERTY(BlueprintReadOnly)
    //事件发起者
    TObjectPtr<ATKPlayerControllerBase> EventInitiator;
    UPROPERTY(BlueprintReadOnly)
    //目标
    TArray<TObjectPtr<ATKPlayerControllerBase>> Targets;
    //额外变量
    UPROPERTY(BlueprintReadOnly)
    TArray<int32> Params_i; 
    UPROPERTY(BlueprintReadOnly)
    TArray<FGameplayTag> Params_g;
    UPROPERTY(BlueprintReadOnly)
    TArray<TObjectPtr<UObject>> Params_O;
};