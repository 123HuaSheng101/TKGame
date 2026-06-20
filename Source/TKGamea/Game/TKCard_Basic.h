// TKCard_Basic.h — 基本牌：杀/闪/桃/酒

#pragma once

#include "CoreMinimal.h"
#include "TKCardBase.h"
#include "TKCard_Basic.generated.h"

/**
 * 基本牌子类
 *
 * 具体效果由 EffectTags 驱动：
 *   Card.Effect.Slash       — 对目标造成 1 点伤害（需目标出闪响应）
 *   Card.Effect.Dodge       — 响应杀，抵消伤害（被动使用，不走 Use 流程）
 *   Card.Effect.Peach       — 回复 1 点体力
 *   Card.Effect.Wine        — 本回合下一张杀伤害+1，或濒死回 1 血
 */
UCLASS()
class TKGAMEA_API UTKCard_Basic : public UTKCardBase
{
	GENERATED_BODY()

public:
	/** 是否是一张响应牌（闪、桃、无懈可击）—— 用于 UI 判断是否在响应窗口可选中 */
	bool IsResponseCard() const;

protected:
	virtual bool CanUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) const override;
	virtual void OnUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) override;

	/** 杀需要目标出闪响应 */
	virtual bool NeedsResponse() const override;

	/** 杀需要的响应牌 Tag */
	virtual FGameplayTag GetResponseRequiredTag() const override;
};
