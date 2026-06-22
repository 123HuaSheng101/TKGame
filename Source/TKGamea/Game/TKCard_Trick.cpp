// TKCard_Trick.cpp

#include "TKCard_Trick.h"
#include "TKGamea.h"
#include "Core/TKPlayerControllerBase.h"
#include "Core/TKPlayerStateBase.h"
#include "Core/TKResponseComponent.h"
#include "Cards/TKCardZoneComponent.h"
#include "GameFramework/GameStateBase.h"

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

// 摸牌效果使用 Tag 搜索匹配
static FName TagPrefix_Draw = TEXT("Card.Effect.Draw");

// ===== 响应系统 =====

bool UTKCard_Trick::NeedsResponse() const
{
	if (!CanBeNegated()) return false; // 无懈可击自身不触发响应
	return true;
}

bool UTKCard_Trick::IsAOECard() const
{
	const FGameplayTag& FirstTag = EffectTags.Num() > 0 ? EffectTags[0] : FGameplayTag();
	return FirstTag.MatchesTag(Tag_Card_AOE_RequireSlash) ||
		   FirstTag.MatchesTag(Tag_Card_AOE_RequireDodge) ||
		   FirstTag.MatchesTag(Tag_Card_AOE_Heal_1) ||
		   FirstTag.MatchesTag(Tag_Card_Harvest);
}

ETKResponseType UTKCard_Trick::GetResponseType() const
{
	return IsAOECard() ? ETKResponseType::Sequential : ETKResponseType::Chain;
}

FGameplayTag UTKCard_Trick::GetResponseRequiredTag() const
{
	// 无懈可击链用的都是 Negate Tag
	return Tag_Card_Negate;
}

void UTKCard_Trick::BuildResponderQueue(FResponseRequest& Req, ATKPlayerStateBase* User, ATKPlayerStateBase* Target) const
{
	// 只对 Sequential 类型的 AOE 构建队列
	if (!IsAOECard() || !User) return;

	const FGameplayTag& FirstTag = EffectTags.Num() > 0 ? EffectTags[0] : FGameplayTag();

	// AOE 需要玩家出的牌类型
	if (FirstTag.MatchesTag(Tag_Card_AOE_RequireSlash))
		Req.RequiredTag = Tag_Card_AOE_RequireSlash;
	else if (FirstTag.MatchesTag(Tag_Card_AOE_RequireDodge))
		Req.RequiredTag = Tag_Card_AOE_RequireDodge;
	else
		Req.RequiredTag = FGameplayTag();

	// 从 User 逆时针构建响应对列（跳过 User 自己）
	Req.ResponderQueue = UTKResponseComponent::BuildResponderQueue(
		User->GetWorld()->GetGameState<AGameStateBase>()->PlayerArray,
		User,
		User);
}

// ===== CanBeNegated =====
bool UTKCard_Trick::CanBeNegated() const
{
	// 无懈可击本身不能被无懈抵消（无懈可击的 EffectTag 是 Card.Effect.Negate）
	const FGameplayTag& FirstTag = EffectTags.Num() > 0 ? EffectTags[0] : FGameplayTag();
	if (FirstTag == Tag_Card_Negate)
	{
		return false;
	}
	return true;
}

void UTKCard_Trick::OnUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target)
{
	const FGameplayTag& FirstTag = EffectTags.Num() > 0 ? EffectTags[0] : FGameplayTag();

	// 非无懈可击的锦囊，先广播询问是否有人打无懈可击
	// TODO: 事件系统 — 其他玩家响应无懈可击
	if (CanBeNegated())
	{
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick - Card [%s] is negatable, awaiting Negate response..."), *CardDefId.ToString());
	}

	// ---- 分发效果 ----

	if (FirstTag == Tag_Card_Snatch)
	{
		// 从 Target 手牌随机抽 1 张到 User
		UTKCardZoneComponent* TargetZone = Target ? Target->GetCardZone() : nullptr;
		ATKPlayerStateBase* UserPS = Cast<ATKPlayerStateBase>(User->PlayerState);
		UTKCardZoneComponent* UserZone = UserPS ? UserPS->GetCardZone() : nullptr;

		if (TargetZone && UserZone && TargetZone->GetHandCardCount() > 0)
		{
			TArray<UTKCardBase*> TargetHand = TargetZone->GetHandCards();
			int32 RandomIdx = FMath::RandRange(0, TargetHand.Num() - 1);
			UTKCardBase* StolenCard = TargetHand[RandomIdx];
			TargetZone->RemoveCardFromHand(StolenCard);
			UserZone->AddCardToHand(StolenCard);
			UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Snatch - [%s] steals card [%s] from [%s]"),
				*User->GetName(), *StolenCard->CardDefId.ToString(), *Target->GetPlayerName());
		}
	}
	else if (FirstTag == Tag_Card_Dismantle)
	{
		// 弃置目标区域 1 张牌
		// TODO: 需要 UI 选择目标区域和具体卡牌，此处简化：随机弃置手牌
		UTKCardZoneComponent* TargetZone = Target ? Target->GetCardZone() : nullptr;
		if (TargetZone && TargetZone->GetHandCardCount() > 0)
		{
			TArray<UTKCardBase*> TargetHand = TargetZone->GetHandCards();
			UTKCardBase* Discarded = TargetHand[FMath::RandRange(0, TargetHand.Num() - 1)];
			TargetZone->RemoveCardFromHand(Discarded);
			UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Dismantle - [%s] discards card [%s] from [%s]"),
				*User->GetName(), *Discarded->CardDefId.ToString(), *Target->GetPlayerName());
		}
	}
	else if (FirstTag.MatchesTag(Tag_Card_Duel))
	{
		// 决斗：双方轮流出杀，先无法出的一方受 1 点伤害
		// TODO: 响应系统 — 轮流出杀
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Duel - [%s] challenges [%s] to a duel (pending response system)"),
			*User->GetName(), Target ? *Target->GetPlayerName() : TEXT("None"));
		Target->ApplyDamage(1);  // M2 阶段简化：直接扣 1 血
	}
	else if (FirstTag.MatchesTag(Tag_Card_AOE_RequireSlash))
	{
		// 南蛮入侵：全体出杀，否则扣 1 血
		// TODO: 响应系统逐个询问
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::AOE_RequireSlash - [%s] plays Barbarian Invasion (pending response system)"), *User->GetName());
	}
	else if (FirstTag.MatchesTag(Tag_Card_AOE_RequireDodge))
	{
		// 万箭齐发：全体出闪，否则扣 1 血
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::AOE_RequireDodge - [%s] plays Volley of Arrows (pending response system)"), *User->GetName());
	}
	else if (FirstTag.MatchesTag(Tag_Card_AOE_Heal_1))
	{
		// 桃园结义：全体回 1 血
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::AOE_Heal - [%s] plays Peach Garden"), *User->GetName());
		AGameStateBase* GS = User->GetWorld()->GetGameState<AGameStateBase>();
		if (GS)
		{
			for (const TObjectPtr<APlayerState>& PS : GS->PlayerArray)
			{
				ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
				if (TKPS && TKPS->IsAlive())
				{
					TKPS->Heal(1);
					UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::AOE_Heal - [%s] receives 1 HP"), *TKPS->GetPlayerName());
				}
			}
		}
	}
	else if (FirstTag.MatchesTag(Tag_Card_Negate))
	{
		// 无懈可击：抵消一张锦囊
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Negate - [%s] negates a trick card (consumed by event system)"), *User->GetName());
	}
	else if (FirstTag.MatchesTag(Tag_Card_Harvest))
	{
		// 五谷丰登
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Harvest - [%s] plays Bountiful Harvest (pending response system)"), *User->GetName());
	}
	else if (FirstTag.MatchesTag(Tag_Card_BorrowKnife))
	{
		// 借刀杀人
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::BorrowKnife - [%s] plays Borrow Knife on [%s] (pending response system)"),
			*User->GetName(), Target ? *Target->GetPlayerName() : TEXT("None"));
	}
	else
	{
		// 检查是否是 Draw 系列标签（Card.Effect.Draw.{N}）
		for (const FGameplayTag& Tag : EffectTags)
		{
			if (Tag.GetTagName().ToString().StartsWith(TagPrefix_Draw.ToString()))
			{
				// 解析 N 值，默认 2 张
				int32 DrawCount = 2;
				TArray<FString> Parts;
				Tag.GetTagName().ToString().ParseIntoArray(Parts, TEXT("."));
				if (Parts.Num() > 0)
				{
					DrawCount = FCString::Atoi(*Parts.Last());
				}

				// TODO: 通过 DeckComponent 摸牌
				UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Draw - [%s] draws %d cards"), *User->GetName(), FMath::Max(DrawCount, 1));
				break;
			}
		}
		Super::OnUse(User, Target);
	}
}
