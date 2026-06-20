#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TKGameTypes.h"
#include "TKCardTypes.generated.h"

/**
 * 调试牌堆模板条目
 * 通过配置若干条 FTKDebugCardEntry 即可快速初始化一叠测试牌
 * 每条指定卡牌 ID、类型、花色、点数、效果标签、数量
 */
USTRUCT(BlueprintType)
struct FTKDebugCardEntry
{
    GENERATED_BODY()
public:
    /** 卡牌标识 ID */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName CardDefId;

    /** 卡牌显示名称 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText CardName;

    /** 卡牌类型 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ETKCardType CardType = ETKCardType::Basic;

    /** 花色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ETKCardSuit Suit = ETKCardSuit::NoSuit;

    /** 点数 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Rank = 0;

    /** 该种牌的数量 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Count = 1;

    /** 效果标签列表（驱动卡牌行为，如 Card.Effect.Slash / Card.Effect.AOE.RequireSlash） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FGameplayTag> EffectTags;
};
