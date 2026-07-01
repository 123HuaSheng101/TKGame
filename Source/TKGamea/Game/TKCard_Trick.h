// TKCard_Trick.h — 非延时锦囊：过河拆桥/顺手牵羊/无中生有/南蛮入侵等

#pragma once

#include "CoreMinimal.h"
#include "TKCardBase.h"
#include "TKCard_Trick.generated.h"
// ---- 效果标签 ----
static FGameplayTag Tag_Card_Snatch          = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Snatch"),          false);
static FGameplayTag Tag_Card_Dismantle       = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Dismantle"),       false);
static FGameplayTag Tag_Card_Negate          = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Negate"),          false);
static FGameplayTag Tag_Card_Duel            = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Duel"),            false);
static FGameplayTag Tag_Card_AOE_RequireSlash= FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.AOE.RequireSlash"),false);
static FGameplayTag Tag_Card_AOE_RequireDodge= FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.AOE.RequireDodge"),false);
static FGameplayTag Tag_Card_AOE_Heal_1      = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.AOE.Heal.1"),      false);
static FGameplayTag Tag_Card_Harvest         = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Harvest"),         false);
static FGameplayTag Tag_Card_BorrowKnife     = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.BorrowKnife"),     false);
// static FGameplayTag Tag_Card_Slash           = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Slash"),           false);
// static FGameplayTag Tag_Card_Dodge           = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Dodge"),           false);
/**
 * 非延时锦囊子类
 *
 * 响应模式：
 *   - 普通锦囊（顺手/过河/无中/决斗/借刀）→ Chain（无懈可击链）
 *   - AOE（南蛮/万箭）→ Sequential（逐人问杀/闪）
 *   - 桃园/五谷 → Sequential（逐人受益）
 *   - 无懈可击 → 不触发响应（已在链中消耗）
 */
UCLASS()
class TKGAMEA_API UTKCard_Trick : public UTKCardBase
{
	GENERATED_BODY()

public:
	bool CanBeNegated() const;
	bool IsAOECard() const;

protected:
	virtual bool NeedsResponse() const override;
	virtual ETKResponseType GetResponseType() const override;
	virtual FGameplayTag GetResponseRequiredTag() const override;
	virtual void BuildResponderQueue(FResponseRequest& Req, ATKPlayerStateBase* User, ATKPlayerStateBase* Target) const override;
	virtual void OnUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) override;
};
