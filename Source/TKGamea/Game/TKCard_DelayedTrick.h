// TKCard_DelayedTrick.h — 延时锦囊：乐不思蜀/兵粮寸断/闪电

#pragma once

#include "CoreMinimal.h"
#include "TKCardBase.h"
#include "TKCard_DelayedTrick.generated.h"

/**
 * 延时锦囊子类
 *
 * 使用时不直接结算，而是放入目标的判定区，在目标判定阶段自动触发结算。
 *
 * 具体效果由 EffectTags 驱动：
 *   Card.Effect.Skip.Play     — 跳过出牌阶段（乐不思蜀）
 *   Card.Effect.Skip.Draw     — 跳过摸牌阶段（兵粮寸断）
 *   Card.Effect.Lightning     — 判定黑桃2~9 则扣 3 血，否则传给下家
 */
UCLASS()
class TKGAMEA_API UTKCard_DelayedTrick : public UTKCardBase
{
	GENERATED_BODY()

public:
	/**
	 * 重写 Use：延时锦囊不进入弃牌堆，而是放入目标判定区
	 */
	virtual bool Use(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) override;

	/**
	 * 判定结算（由 TurnComponent 在 Judge 阶段调用）
	 * @param Owner 判定区所属玩家
	 * @return true 表示结算完毕，卡牌应从判定区移除
	 */
	UFUNCTION(BlueprintCallable, Category = "DelayedTrick")
	bool OnJudge(ATKPlayerStateBase* Owner);

protected:
	virtual void OnUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) override {}
	virtual void OnAfterUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) override;

	/**
	 * 判定：是否是有效花色/点数
	 * 乐 — 非红桃即生效
	 * 兵 — 非梅花即生效
	 * 闪电 — 黑桃 2~9 即生效
	 */
	bool IsJudgementEffective(const FGameplayTag& EffectTag, ETKCardSuit JudgedSuit, int32 JudgedRank) const;
};
