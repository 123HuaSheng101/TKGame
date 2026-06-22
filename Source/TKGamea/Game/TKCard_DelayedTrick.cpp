// TKCard_DelayedTrick.cpp

#include "TKCard_DelayedTrick.h"
#include "TKGamea.h"
#include "Core/TKPlayerControllerBase.h"
#include "Core/TKPlayerStateBase.h"
#include "Core/TKEventComponentBase.h"
#include "Cards/TKCardZoneComponent.h"

static FGameplayTag Tag_Card_Skip_Play = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Skip.Play"), false);
static FGameplayTag Tag_Card_Skip_Draw = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Skip.Draw"), false);
static FGameplayTag Tag_Card_Lightning  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Lightning"), false);

bool UTKCard_DelayedTrick::Use(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target)
{
	if (!User || !Target) return false;

	// 前置校验
	if (!PreUseCheck(User)) return false;

	// 目标合法性
	if (!CanUse(User, Target)) return false;

	// 延时锦囊不直接结算，放入目标判定区
	UTKCardZoneComponent* Zone = Target->GetCardZone();
	if (!Zone) return false;

	// 从手牌区移除
	ATKPlayerStateBase* UserPS = Cast<ATKPlayerStateBase>(User->PlayerState);
	UTKCardZoneComponent* UserZone = UserPS ? UserPS->GetCardZone() : nullptr;
	if (UserZone)
	{
		UserZone->RemoveCardFromHand(this);
	}

	// 放入判定区
	Zone->AddToJudgement(this);

	UE_LOG(LogTKGame, Log, TEXT("TKCard_DelayedTrick - Card [%s] placed in judgement zone of [%s]"),
		*CardDefId.ToString(), *Target->GetPlayerName());

	// 后置处理（不进入弃牌堆）
	OnAfterUse(User, Target);
	return true;
}

void UTKCard_DelayedTrick::OnAfterUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target)
{
	// 延时锦囊不进入弃牌堆，只触发事件
	FEventContext Ctx = BuildUseEvent(User, Target);
	UTKEventComponentBase* EventComp = UTKEventComponentBase::Get(User);
	if (EventComp)
	{
		EventComp->PostEvent(Ctx);
	}
}

bool UTKCard_DelayedTrick::OnJudge(ATKPlayerStateBase* Owner)
{
	if (!Owner) return false;

	UTKCardZoneComponent* Zone = Owner->GetCardZone();
	if (!Zone) return false;

	// TODO: 真正的判定需要从牌堆顶翻一张牌
	// 此处简化为随机花色/点数进行判定
	ETKCardSuit JudgedSuit = ETKCardSuit(FMath::RandRange(0, 3));
	int32 JudgedRank = FMath::RandRange(1, 13);

	const FGameplayTag& EffectTag = EffectTags.Num() > 0 ? EffectTags[0] : FGameplayTag();
	bool bEffective = IsJudgementEffective(EffectTag, JudgedSuit, JudgedRank);

	if (bEffective)
	{
		// 判定生效
		if (EffectTag == Tag_Card_Skip_Play)
		{
			UE_LOG(LogTKGame, Log, TEXT("DelayedTrick::SkipPlay - [%s] judged non-Heart, skips Play phase"), *Owner->GetPlayerName());
			// TODO: 标记 SkipPlayPhase 状态，由 TurnComponent 检查
		}
		else if (EffectTag == Tag_Card_Skip_Draw)
		{
			UE_LOG(LogTKGame, Log, TEXT("DelayedTrick::SkipDraw - [%s] judged non-Club, skips Draw phase"), *Owner->GetPlayerName());
			// TODO: 标记 SkipDrawPhase 状态
		}
		else if (EffectTag == Tag_Card_Lightning)
		{
			Owner->ApplyDamage(3);
			UE_LOG(LogTKGame, Log, TEXT("DelayedTrick::Lightning - [%s] takes 3 damage!"), *Owner->GetPlayerName());
		}
	}
	else
	{
		if (EffectTag == Tag_Card_Lightning)
		{
			// 闪电判定失效，传给下家
			UE_LOG(LogTKGame, Log, TEXT("DelayedTrick::Lightning - [%s] judged safe, passes to next player"), *Owner->GetPlayerName());
			// TODO: 从当前判定区移除，传给下一个存活玩家
		}
		else
		{
			UE_LOG(LogTKGame, Log, TEXT("DelayedTrick - [%s] judged safe, card [%s] has no effect"),
				*Owner->GetPlayerName(), *CardDefId.ToString());
		}
	}

	// 结算完毕，从判定区移除并进入弃牌堆
	// TODO: 由 TurnComponent 或 ZoneComponent 处理移除
	Zone->RemoveFromJudgement(this);
	return true;
}

bool UTKCard_DelayedTrick::IsJudgementEffective(const FGameplayTag& EffectTag, ETKCardSuit JudgedSuit, int32 JudgedRank) const
{
	if (EffectTag == Tag_Card_Skip_Play)
	{
		// 乐不思蜀：非红桃生效
		return JudgedSuit != ETKCardSuit::Heart;
	}
	if (EffectTag == Tag_Card_Skip_Draw)
	{
		// 兵粮寸断：非梅花生效
		return JudgedSuit != ETKCardSuit::Club;
	}
	if (EffectTag == Tag_Card_Lightning)
	{
		// 闪电：黑桃 2~9 生效
		return JudgedSuit == ETKCardSuit::Spade && JudgedRank >= 2 && JudgedRank <= 9;
	}
	return false;
}
