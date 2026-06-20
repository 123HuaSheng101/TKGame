#include "TKGamea.h"
// TKCard_Basic.cpp

#include "TKCard_Basic.h"
#include "Core/TKPlayerControllerBase.h"
#include "Core/TKPlayerStateBase.h"

static FGameplayTag Tag_Card_Slash  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Slash"),  false);
static FGameplayTag Tag_Card_Dodge  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Dodge"),  false);
static FGameplayTag Tag_Card_Peach  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Peach"),  false);
static FGameplayTag Tag_Card_Wine   = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Wine"),   false);

bool UTKCard_Basic::IsResponseCard() const
{
	const FGameplayTag& FirstTag = EffectTags.Num() > 0 ? EffectTags[0] : FGameplayTag();
	return FirstTag == Tag_Card_Dodge || FirstTag == Tag_Card_Peach;
}

bool UTKCard_Basic::NeedsResponse() const
{
	const FGameplayTag& FirstTag = EffectTags.Num() > 0 ? EffectTags[0] : FGameplayTag();
	return FirstTag == Tag_Card_Slash;
}

FGameplayTag UTKCard_Basic::GetResponseRequiredTag() const
{
	const FGameplayTag& FirstTag = EffectTags.Num() > 0 ? EffectTags[0] : FGameplayTag();
	if (FirstTag == Tag_Card_Slash) return Tag_Card_Dodge;
	return FGameplayTag();
}

bool UTKCard_Basic::CanUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) const
{
	if (!User || !Target) return false;

	ATKPlayerStateBase* UserPS = Cast<ATKPlayerStateBase>(User->PlayerState);
	if (!UserPS || !UserPS->IsAlive()) return false;

	const FGameplayTag& FirstTag = EffectTags.Num() > 0 ? EffectTags[0] : FGameplayTag();

	// 桃：可对自己使用
	if (FirstTag == Tag_Card_Peach)
	{
		if (Target != User->PlayerState) return false;
		return Target->IsAlive() || Target->CurrentHealth <= 0;
	}

	// 酒：可对自己使用
	if (FirstTag == Tag_Card_Wine)
	{
		if (Target != User->PlayerState) return false;
		return true;
	}

	// 杀、闪：需要目标存活
	if (!Target->IsAlive()) return false;

	// 杀不能对自己
	if (FirstTag == Tag_Card_Slash)
	{
		return Target != User->PlayerState;
	}

	// 闪不在主动 Use 流程中使用，由响应系统处理
	if (FirstTag == Tag_Card_Dodge)
	{
		return false;
	}

	return Super::CanUse(User, Target);
}

void UTKCard_Basic::OnUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target)
{
	const FGameplayTag& FirstTag = EffectTags.Num() > 0 ? EffectTags[0] : FGameplayTag();

	if (FirstTag == Tag_Card_Slash)
	{
		// 对目标造成 1 点伤害（仅在响应窗口超时后执行）
		Target->ApplyDamage(1);
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Basic::Slash - [%s] deals 1 damage to [%s]"),
			*User->GetName(), *Target->GetPlayerName());
	}
	else if (FirstTag == Tag_Card_Dodge)
	{
		// 闪由响应系统通过 SubmitResponse 处理，不走此路径
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Basic::Dodge - used as response (not via Use)")); 
	}
	else if (FirstTag == Tag_Card_Peach)
	{
		Target->Heal(1);
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Basic::Peach - [%s] heals [%s] for 1 HP"),
			*User->GetName(), *Target->GetPlayerName());
	}
	else if (FirstTag == Tag_Card_Wine)
	{
		if (Target->CurrentHealth <= 0)
		{
			Target->Heal(1);
			UE_LOG(LogTKGame, Log, TEXT("TKCard_Basic::Wine - [%s] saves self from dying"), *User->GetName());
		}
		else
		{
			// TODO: 标记 Wine 状态（下一张杀伤害+1）
			UE_LOG(LogTKGame, Log, TEXT("TKCard_Basic::Wine - [%s] used wine, next slash damage +1"), *User->GetName());
		}
	}
	else
	{
		Super::OnUse(User, Target);
	}
}
