// TKCard_Trick.h — 非延时锦囊：过河拆桥/顺手牵羊/无中生有/南蛮入侵等

#pragma once

#include "CoreMinimal.h"
#include "TKCardBase.h"
#include "TKCard_Trick.generated.h"

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
