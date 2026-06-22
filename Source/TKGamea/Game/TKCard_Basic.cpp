// TKCard_Basic.cpp

#include "TKCard_Basic.h"
#include "TKGamea.h"
#include "Core/TKPlayerControllerBase.h"
#include "Core/TKPlayerStateBase.h"
#include "Core/TKGameStateBase.h"
#include "Core/TKTurnComponentBase.h"

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

	// 杀不能对自己 + 检查次数限制
	if (FirstTag == Tag_Card_Slash)
	{
		if (Target == User->PlayerState) return false;

		// 检查杀次数
		ATKGameStateBase* TKGS = Cast<ATKGameStateBase>(User->GetWorld()->GetGameState());
		if (TKGS)
		{
			UTKTurnComponentBase* TurnComp = TKGS->GetTurnComponent();
			if (TurnComp && !TurnComp->CanUseSlash())
			{
				UE_LOG(LogTKGame, Warning, TEXT("CanUse: Slash limit reached! Used=%d"),
					TurnComp->GetSlashUsedCount());
				return false;
			}
		}
		return true;
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
		// 酒状态：伤害+1
		int32 Damage = 1;
		ATKGameStateBase* TKGS = Cast<ATKGameStateBase>(User->GetWorld()->GetGameState());
		if (TKGS)
		{
			UTKTurnComponentBase* TurnComp = TKGS->GetTurnComponent();
			if (TurnComp)
			{
				// 消耗杀次数
				TurnComp->ConsumeSlash();

				// 酒加成
				if (TurnComp->IsWined())
				{
					Damage = 2;
					TurnComp->SetWined(false);
					UE_LOG(LogTKGame, Log, TEXT("TKCard_Basic::Slash - Wine bonus! damage=%d"), Damage);
				}
			}
		}

		Target->ApplyDamage(Damage);
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Basic::Slash - [%s] deals %d damage to [%s]"),
			*User->GetName(), Damage, *Target->GetPlayerName());
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
			// 设置酒状态（下一张杀伤害+1）
			ATKGameStateBase* TKGS = Cast<ATKGameStateBase>(User->GetWorld()->GetGameState());
			if (TKGS)
			{
				UTKTurnComponentBase* TurnComp = TKGS->GetTurnComponent();
				if (TurnComp)
				{
					TurnComp->SetWined(true);
					UE_LOG(LogTKGame, Log, TEXT("TKCard_Basic::Wine - [%s] set wine state, next slash damage +1"), *User->GetName());
				}
			}
		}
	}
	else
	{
		Super::OnUse(User, Target);
	}
}
