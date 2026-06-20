#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TKGameTypes.generated.h"

class ATKPlayerControllerBase;

// ============================================================
// 核心枚举
// ============================================================

/** 游戏模式类型 */
UENUM(BlueprintType)
enum class ETKGameModeType : uint8
{
    Identity,		// 身份模式
    NationalWar		// 国战模式（预留）
};

/** 回合阶段：标准三国杀六阶段推进 */
UENUM(BlueprintType)
enum class ETKTurnPhase : uint8
{
    Prepare,		// 准备阶段
    Judge,			// 判定阶段
    Draw,			// 摸牌阶段
    Play,			// 出牌阶段
    Discard,		// 弃牌阶段
    Finish			// 结束阶段
};

/** 玩家身份 */
UENUM(BlueprintType)
enum class ETKIdentity : uint8
{
    Unknown,		// 未知（未分配或未暴露）
    Lord,			// 主公
    Loyalist,		// 忠臣
    Rebel,			// 反贼
    Renegade		// 内奸
};

/** 牌所在区域 */
UENUM(BlueprintType)
enum class ETKCardZone : uint8
{
    Deck,			// 牌堆
    DiscardPile,	// 弃牌堆
    Hand,			// 手牌区
    Equipment,		// 装备区
    Judgement,		// 判定区
    OutOfGame		// 移出游戏
};

/** 游戏胜负结果 */
UENUM(BlueprintType)
enum class ETKGameResult : uint8
{
    None,				// 游戏进行中
    LordLoyalistWin,	// 主忠阵营胜利
    RebelWin,			// 反贼阵营胜利
    RenegadeWin			// 内奸单独胜利
};

/** 卡牌花色 */
UENUM(BlueprintType)
enum class ETKCardSuit : uint8
{
    Spade,			// 黑桃
    Heart,			// 红桃
    Club,			// 梅花
    Diamond,		// 方块
    NoSuit			// 无色
};

/** 卡牌大类类型 */
UENUM(BlueprintType)
enum class ETKCardType : uint8
{
    Basic,			// 基本牌（杀、闪、桃）
    Trick,			// 锦囊牌
    Equipment,		// 装备牌
    DelayedTrick	// 延时锦囊
};

/** 响应类型 */
UENUM(BlueprintType)
enum class ETKResponseType : uint8
{
    None,           // 不需要响应（直接结算）
    SingleTarget,   // 单目标响应（杀→闪）
    Chain,          // 链式响应（无懈可击链）
    Sequential,     // 逐人响应（AOE群伤/濒死求桃）
    Duel            // 决斗（双方轮流）
};

/** 响应结果 */
UENUM(BlueprintType)
enum class ETKResponseResult : uint8
{
    Pending,        // 等待响应中
    Responded,      // 有人响应（伤害抵消/锦囊被无懈）
    Timeout,        // 超时无人响应（效果生效）
    AllPassed       // 所有人跳过（AOE全部出杀/闪）
};

// ============================================================
// 核心结构体
// ============================================================

/** 游戏模式配置参数 */
USTRUCT(BlueprintType)
struct FTKModeConfig
{
    GENERATED_BODY()
public:
    /** 玩家人数 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 PlayerCount = 4;

    /** 模式类型 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ETKGameModeType ModeType = ETKGameModeType::Identity;

    /** 起始手牌数 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 InitialHandCount = 4;

    /** 每回合摸牌数 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 DrawCountPerTurn = 2;

    /** 是否启用武将（M3 启用） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bEnableGenerals = false;
};

/** 卡牌模板定义（来自数据表） */
USTRUCT(BlueprintType)
struct FTKCardDef
{
    GENERATED_BODY()
public:
    /** 唯一标识 ID */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName CardDefId;

    /** 显示名称 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText Name;

    /** 卡牌类型 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ETKCardType CardType = ETKCardType::Basic;

    /** 花色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ETKCardSuit Suit = ETKCardSuit::NoSuit;

    /** 点数（1~13，对应 A~K） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Rank = 0;

    /** 效果标签列表 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FGameplayTag> EffectTags;
};

/** 卡牌运行时实例（轻量引用结构，非 UObject） */
USTRUCT(BlueprintType)
struct FTKCardInstance
{
    GENERATED_BODY()
public:
    /** 运行时实例 ID */
    UPROPERTY(BlueprintReadOnly)
    int32 InstanceId = INDEX_NONE;

    /** 引用 CardDefId */
    UPROPERTY(BlueprintReadOnly)
    FName CardDefId;

    /** 持有者 PlayerState */
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<APlayerState> Owner;

    /** 当前所在区域 */
    UPROPERTY(BlueprintReadOnly)
    ETKCardZone ZoneType = ETKCardZone::Deck;

    bool IsValid() const { return InstanceId != INDEX_NONE; }
};

/** 游戏结果数据（复制到 GameState） */
USTRUCT(BlueprintType)
struct FTKGameResultData
{
    GENERATED_BODY()
public:
    /** 胜者阵营 */
    UPROPERTY(BlueprintReadOnly)
    ETKGameResult WinnerFaction = ETKGameResult::None;

    /** 胜利玩家列表 */
    UPROPERTY(BlueprintReadOnly)
    TArray<TObjectPtr<APlayerState>> WinningPlayers;
};

// ============================================================
// 事件上下文（事件系统的松散参数结构）
// ============================================================

USTRUCT(BlueprintType)
struct FEventContext
{
    GENERATED_BODY()
public:
    /** 事件类型 Tag */
    UPROPERTY(BlueprintReadOnly)
    FGameplayTag EventTag;

    /** 事件发起者 */
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<ATKPlayerControllerBase> EventInitiator;

    /** 事件目标数组 */
    UPROPERTY(BlueprintReadOnly)
    TArray<TObjectPtr<ATKPlayerControllerBase>> Targets;

    /** 额外整数参数 */
    UPROPERTY(BlueprintReadOnly)
    TArray<int32> Params_i; 

    /** 额外 Tag 参数 */
    UPROPERTY(BlueprintReadOnly)
    TArray<FGameplayTag> Params_g;

    /** 额外对象参数 */
    UPROPERTY(BlueprintReadOnly)
    TArray<TObjectPtr<UObject>> Params_O;
};

// ============================================================
// 响应系统
// ============================================================

class UTKCardBase;
class ATKPlayerStateBase;

/**
 * 响应请求
 * 由发起卡牌创建，ResponseComponent 管理生命周期，分发到目标客户端等待响应
 */
USTRUCT()
struct FResponseRequest
{
    GENERATED_BODY()

    /** 响应类型 */
    UPROPERTY()
    ETKResponseType Type = ETKResponseType::None;

    /** 请求 Tag（需要什么响应牌，如 Card.Effect.Dodge） */
    UPROPERTY()
    FGameplayTag RequiredTag;

    /** 发起此响应的来源卡牌 */
    UPROPERTY()
    TObjectPtr<UTKCardBase> SourceCard;

    /** 发起者 PlayerState */
    UPROPERTY()
    TObjectPtr<APlayerState> Initiator;

    /** 主目标（杀→闪的目标，无懈可击的来源） */
    UPROPERTY()
    TObjectPtr<APlayerState> PrimaryTarget;

    /** 需要依次响应的玩家列表（AOE/濒死用） */
    UPROPERTY()
    TArray<TObjectPtr<APlayerState>> ResponderQueue;

    /** 当前等待响应的玩家索引（Sequential 用） */
    UPROPERTY()
    int32 CurrentResponderIndex = 0;

    /** 已响应的玩家列表（打出了响应牌） */
    UPROPERTY()
    TArray<TObjectPtr<APlayerState>> AlreadyResponded;

    /** 当前结果 */
    UPROPERTY()
    ETKResponseResult Result = ETKResponseResult::Pending;

    bool IsPending() const { return Result == ETKResponseResult::Pending; }
};

/**
 * 响应结果回调数据
 */
USTRUCT()
struct FResponseResult
{
    GENERATED_BODY()

    /** 最终结果 */
    UPROPERTY()
    ETKResponseResult Result = ETKResponseResult::Timeout;

    /** 响应者（谁出了响应牌），Timeout 时为空 */
    UPROPERTY()
    TObjectPtr<APlayerState> Responder;

    /** 响应用的那张卡牌 */
    UPROPERTY()
    TObjectPtr<UTKCardBase> ResponseCard;

    /** 是否被抵消了（无懈/闪生效 = true；AOE 有人未出 = false） */
    bool bNegated = false;
};