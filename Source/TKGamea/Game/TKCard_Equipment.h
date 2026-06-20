// TKCard_Equipment.h — 装备牌：武器/防具/+1马/-1马/宝物

#pragma once

#include "CoreMinimal.h"
#include "TKCardBase.h"
#include "TKCard_Equipment.generated.h"

/**
 * 装备牌子类
 *
 * 使用后放入装备区而非弃牌堆。同类型装备自动顶替旧装备。
 *
 * 槽位判定由 EffectTags 驱动：
 *   Card.Equip.Weapon     — 武器槽
 *   Card.Equip.Armor      — 防具槽
 *   Card.Equip.Offense    — 进攻马（-1马）
 *   Card.Equip.Defense    — 防御马（+1马）
 *   Card.Equip.Treasure   — 宝物槽
 */
UCLASS()
class TKGAMEA_API UTKCard_Equipment : public UTKCardBase
{
	GENERATED_BODY()

public:
	virtual bool Use(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) override;

	/** 卸下装备（从装备区移除并进入弃牌堆） */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void Unequip(UTKCardZoneComponent* Zone);

	/** 获取装备槽 Tag */
	FGameplayTag GetSlotTag() const;

protected:
	virtual bool CanUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) const override;
	virtual void OnUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) override {}
	virtual void OnAfterUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) override;
};
