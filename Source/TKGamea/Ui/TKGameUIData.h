// TKGameUIData.h — Model → View 数据传输协议

#pragma once

#include "CoreMinimal.h"
#include "TKGameUIData.generated.h"

/**
 * 游戏全局 UI 数据
 * ATKGameHUD 采集 → UTKGameHudWidget 展示
 */
USTRUCT(BlueprintType)
struct FTKGameUIData
{
    GENERATED_BODY()

    /** 当前回合号 */
    UPROPERTY(BlueprintReadOnly)
    int32 TurnNumber = 0;

    /** 当前回合玩家名 */
    UPROPERTY(BlueprintReadOnly)
    FString CurrentPlayerName;

    /** 当前阶段名 */
    UPROPERTY(BlueprintReadOnly)
    FString PhaseName;

    /** 牌堆剩余张数 */
    UPROPERTY(BlueprintReadOnly)
    int32 DeckCount = 0;

    /** 弃牌堆张数 */
    UPROPERTY(BlueprintReadOnly)
    int32 DiscardCount = 0;

    /** 胜负结果（None=进行中） */
    UPROPERTY(BlueprintReadOnly)
    int32 WinnerFaction = 0;
};

/**
 * 单个玩家 UI 数据
 */
USTRUCT(BlueprintType)
struct FTKPlayerUIData
{
    GENERATED_BODY()

    /** 座位号 */
    UPROPERTY(BlueprintReadOnly)
    int32 SeatIndex = 0;

    /** 玩家名 */
    UPROPERTY(BlueprintReadOnly)
    FString PlayerName;

    /** 身份名 ("Lord"/"Loyalist"/"Rebel"/"Renegade"/"???") */
    UPROPERTY(BlueprintReadOnly)
    FString IdentityName;

    /** 是否存活 */
    UPROPERTY(BlueprintReadOnly)
    bool bAlive = true;

    /** 当前HP */
    UPROPERTY(BlueprintReadOnly)
    int32 CurrentHealth = 0;

    /** 最大HP */
    UPROPERTY(BlueprintReadOnly)
    int32 MaxHealth = 0;

    /** 手牌数量 */
    UPROPERTY(BlueprintReadOnly)
    int32 HandCardCount = 0;

    /** 是否是当前回合玩家 */
    UPROPERTY(BlueprintReadOnly)
    bool bIsCurrentTurn = false;
};

/**
 * 响应提示数据
 * HUD → ResponsePanel 展示
 */
USTRUCT(BlueprintType)
struct FTKResponseUIData
{
    GENERATED_BODY()

    /** 需要出的牌 Tag 名 */
    UPROPERTY(BlueprintReadOnly)
    FString RequiredTag;

    /** 提示文本 */
    UPROPERTY(BlueprintReadOnly)
    FText PromptText;

    /** 倒计时（秒） */
    UPROPERTY(BlueprintReadOnly)
    float TimeRemaining = 0.0f;

    /** 是否等待中 */
    UPROPERTY(BlueprintReadOnly)
    bool bIsWaiting = false;
};
