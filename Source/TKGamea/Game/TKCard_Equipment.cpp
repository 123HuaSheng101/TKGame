// TKCard_Equipment.cpp

#include "TKCard_Equipment.h"
#include "TKGamea.h"
#include "Core/TKPlayerControllerBase.h"
#include "Core/TKPlayerStateBase.h"
#include "Core/TKEventComponentBase.h"
#include "Cards/TKCardZoneComponent.h"

static FGameplayTag Tag_Equip_Weapon   = FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Weapon"),   false);
static FGameplayTag Tag_Equip_Armor    = FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Armor"),    false);
static FGameplayTag Tag_Equip_Offense  = FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Offense"),  false);
static FGameplayTag Tag_Equip_Defense  = FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Defense"),  false);
static FGameplayTag Tag_Equip_Treasure = FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Treasure"), false);

FGameplayTag UTKCard_Equipment::GetSlotTag() const
{
	for (const FGameplayTag& Tag : EffectTags)
	{
		if (Tag.MatchesTag(Tag_Equip_Weapon))   return Tag;
		if (Tag.MatchesTag(Tag_Equip_Armor))    return Tag;
		if (Tag.MatchesTag(Tag_Equip_Offense))  return Tag;
		if (Tag.MatchesTag(Tag_Equip_Defense))  return Tag;
		if (Tag.MatchesTag(Tag_Equip_Treasure)) return Tag;
	}
	return FGameplayTag();
}

bool UTKCard_Equipment::CanUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) const
{
	if (!User || !Target) return false;

	// 装备只能对自己使用
	ATKPlayerStateBase* UserPS = Cast<ATKPlayerStateBase>(User->PlayerState);
	if (Target != UserPS) return false;

	if (!Target->IsAlive()) return false;

	return true;
}

bool UTKCard_Equipment::Use(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target)
{
	if (!User || !Target) return false;

	if (!PreUseCheck(User)) return false;
	if (!CanUse(User, Target)) return false;

	// 装备只对自己，Target 就是使用者自己
	UTKCardZoneComponent* Zone = Target->GetCardZone();
	if (!Zone) return false;

	FGameplayTag SlotTag = GetSlotTag();
	if (SlotTag.IsValid())
	{
		// 检查同槽位旧装备，顶掉
		TArray<UTKCardBase*> EquipCards = Zone->GetEquipmentCards();
		for (UTKCardBase* Existing : EquipCards)
		{
			UTKCard_Equipment* Equip = Cast<UTKCard_Equipment>(Existing);
			if (Equip && Equip->GetSlotTag() == SlotTag)
			{
				// 顶掉旧装备
				Zone->RemoveFromEquipment(Existing);
				UE_LOG(LogTKGame, Log, TEXT("TKCard_Equipment - Replaced old equipment [%s] in slot [%s]"),
					*Existing->CardDefId.ToString(), *SlotTag.GetTagName().ToString());
			}
		}
	}

	// 从手牌区移除
	ATKPlayerStateBase* UserPS = Cast<ATKPlayerStateBase>(User->PlayerState);
	UTKCardZoneComponent* UserZone = UserPS ? UserPS->GetCardZone() : nullptr;
	if (UserZone)
	{
		UserZone->RemoveCardFromHand(this);
	}

	// 放入装备区
	Zone->AddToEquipment(this);

	UE_LOG(LogTKGame, Log, TEXT("TKCard_Equipment - [%s] equipped card [%s] in slot [%s]"),
		*User->GetName(), *CardDefId.ToString(), *SlotTag.GetTagName().ToString());

	OnAfterUse(User, Target);
	return true;
}

void UTKCard_Equipment::OnAfterUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target)
{
	// 装备不进入弃牌堆，只触发事件
	FEventContext Ctx = BuildUseEvent(User, Target);
	UTKEventComponentBase* EventComp = UTKEventComponentBase::Get(User);
	if (EventComp)
	{
		EventComp->PostEvent(Ctx);
	}
}

void UTKCard_Equipment::Unequip(UTKCardZoneComponent* Zone)
{
	if (!Zone) return;
	Zone->RemoveFromEquipment(this);
	UE_LOG(LogTKGame, Log, TEXT("TKCard_Equipment::Unequip - Card [%s] removed from equipment"), *CardDefId.ToString());
}
