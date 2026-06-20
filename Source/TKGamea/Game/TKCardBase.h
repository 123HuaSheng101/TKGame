// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "TKGameTypes.h"
#include "TKCardBase.generated.h"

class ATKPlayerControllerBase;
class ATKPlayerStateBase;
class UTKResponseComponent;
class UTKDeckComponent;

/**
 * 卡牌运行时基类（抽象，不可直接实例化）
 *
 * 继承体系：
 *   UTKCardBase
 *     ├── UTKCard_Basic         — 基本牌（杀/闪/桃/酒）
 *     ├── UTKCard_Trick          — 非延时锦囊
 *     ├── UTKCard_DelayedTrick   — 延时锦囊（乐/兵/闪电）
 *     └── UTKCard_Equipment      — 装备牌
 *
 * 使用流程（模板方法，子类重写 OnUse）：
 *   1. PreUseCheck  — 阶段/时机/距离校验
 *   2. CanUse       — 目标合法性校验
 *   3. 如果需要响应 → 等待 ResponseComponent 回调 → ContinueUseAfterResponse
 *   4. OnUse        — 子类具体效果
 *   5. OnAfterUse   — 触发事件、弃牌堆回收
 */
UCLASS(Abstract)
class TKGAMEA_API UTKCardBase : public UObject
{
	GENERATED_BODY()

public:
	UTKCardBase();

	/** 支持网络复制（消除 FNetGUIDCache 警告） */
	virtual bool IsSupportedForNetworking() const override { return true; }

	// ===== 模板方法 =====

	/**
	 * 使用卡牌的完整流程（子类可重写，如延时锦囊/装备）
	 * 主动牌（杀/锦囊）：发起响应请求 → 等回调 → 结算
	 * 装备/延时锦囊：子类重写跳过此流程
	 * @return 是否通过前置校验（不代表已结算）
	 */
	UFUNCTION(BlueprintCallable, Category = "Card")
	virtual bool Use(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target);

protected:
	/** 前置校验：当前阶段/时机/距离是否允许使用 */
	virtual bool PreUseCheck(ATKPlayerControllerBase* User) const;

	/** 目标合法性校验：是否可对指定目标使用 */
	virtual bool CanUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) const;

	/** 是否需要目标响应（杀需要闪，锦囊需要无懈） */
	virtual bool NeedsResponse() const { return false; }

	/** 获取响应类型（默认单目标，锦囊用 Chain，AOE/濒死用 Sequential） */
	virtual ETKResponseType GetResponseType() const { return ETKResponseType::SingleTarget; }

	/** 获取需要的响应牌 Tag（如 Card.Effect.Dodge） */
	virtual FGameplayTag GetResponseRequiredTag() const { return FGameplayTag(); }

	/** 构建响应对列（AOE/濒死用，子类重写） */
	virtual void BuildResponderQueue(FResponseRequest& Req, ATKPlayerStateBase* User, ATKPlayerStateBase* Target) const {}

	/** 卡牌具体效果（子类必须重写） */
	virtual void OnUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) {}

	/** 使用后处理：触发事件、移入弃牌堆（装备/延时锦囊会重写） */
	virtual void OnAfterUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target);

	/** 构建卡牌使用事件上下文 */
	FEventContext BuildUseEvent(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) const;

	// ===== 异步响应流程 =====

	/** 响应结算后的继续执行（由 ResponseComponent 回调） */
	void ContinueUseAfterResponse(const FResponseResult& Result);

	/** 暂存的 User/Target（响应等待期间） */
	UPROPERTY()
	TObjectPtr<ATKPlayerControllerBase> PendingUser;

	UPROPERTY()
	TObjectPtr<ATKPlayerStateBase> PendingTarget;

public:
	// ===== 牌面属性 =====

	/** 卡牌模板标识 ID */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FName CardDefId;

	/** 卡牌显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FText CardName;

	/** 卡牌大类类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	ETKCardType CardType = ETKCardType::Basic;

	/** 花色 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	ETKCardSuit Suit = ETKCardSuit::NoSuit;

	/** 点数（1~13） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	int32 Rank = 0;

	/** 当前所在的牌区（由牌区组件维护） */
	UPROPERTY(BlueprintReadOnly, Category = "Card")
	ETKCardZone ZoneType = ETKCardZone::Deck;

	/** 效果标签列表（从 CardDef/模板复制，驱动卡牌行为） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	TArray<FGameplayTag> EffectTags;
};
