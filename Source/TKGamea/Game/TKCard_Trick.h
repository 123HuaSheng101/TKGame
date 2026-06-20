// TKCard_Trick.h — 非延时锦囊：过河拆桥/顺手牵羊/无中生有/南蛮入侵等

#pragma once

#include "CoreMinimal.h"
#include "TKCardBase.h"
#include "TKCard_Trick.generated.h"

/**
 * 非延时锦囊子类
 *
 * 具体效果由 EffectTags 驱动：
 *   Card.Effect.Snatch            — 从目标手牌随机抽 1 张
 *   Card.Effect.Dismantle         — 弃置目标区域 1 张牌
 *   Card.Effect.Draw.{N}          — 摸 N 张牌
 *   Card.Effect.Negate            — 抵消一张锦囊效果
 *   Card.Effect.Duel              — 与目标决斗
 *   Card.Effect.AOE.RequireSlash  — 全体需出杀（南蛮入侵）
 *   Card.Effect.AOE.RequireDodge  — 全体需出闪（万箭齐发）
 *   Card.Effect.AOE.Heal.{N}      — 全体回 N 血（桃园结义）
 *   Card.Effect.Harvest           — 五谷丰登
 *   Card.Effect.BorrowKnife       — 借刀杀人
 */
UCLASS()
class TKGAMEA_API UTKCard_Trick : public UTKCardBase
{
	GENERATED_BODY()

public:
	/** 此锦囊是否能被无懈可击抵消 */
	bool CanBeNegated() const;

protected:
	virtual void OnUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) override;
};
