// TKCard_Trick.cpp

#include "TKCard_Trick.h"
#include "TKGamea.h"
#include "Core/TKPlayerControllerBase.h"
#include "Core/TKPlayerStateBase.h"
#include "Core/TKGameStateBase.h"
#include "Core/TKGameModeBase.h"
#include "Core/TKResponseComponent.h"
#include "Cards/TKCardZoneComponent.h"
#include "Cards/TKDeckComponent.h"
#include "GameFramework/GameStateBase.h"
#include "TKCard_Basic.h"



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

	// AOE 需要玩家出的牌类型（必须用实际牌 Tag 匹配，而非 AOE 标签）
	if (FirstTag.MatchesTag(Tag_Card_AOE_RequireSlash))
		Req.RequiredTag = Tag_Card_Slash;
	else if (FirstTag.MatchesTag(Tag_Card_AOE_RequireDodge))
		Req.RequiredTag = Tag_Card_Dodge;
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
		// 弃置目标区域 1 张牌（简化：随机弃置手牌）
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
		// 决斗：双方轮流出杀（简化2轮：目标先出→使用者后出）
		ATKPlayerStateBase* UserPS = Cast<ATKPlayerStateBase>(User->PlayerState);
		if (!Target || !UserPS) return;

		ATKGameStateBase* TKGS = Cast<ATKGameStateBase>(User->GetWorld()->GetGameState());
		UTKResponseComponent* RespComp = TKGS ? TKGS->GetResponseComponent() : nullptr;
		if (!RespComp)
		{
			// 回退：直接扣血
			Target->ApplyDamage(1, UserPS);
			UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Duel - no ResponseComponent, fallback: [%s] takes 1 damage"), *Target->GetPlayerName());
			return;
		}

		// 第1轮：目标先出杀
		FResponseRequest DuelReq1;
		DuelReq1.Type = ETKResponseType::Sequential;
		DuelReq1.RequiredTag = Tag_Card_Slash;
		DuelReq1.Initiator = UserPS;
		DuelReq1.PrimaryTarget = Target;
		DuelReq1.ResponderQueue.Add(Target);
		DuelReq1.CurrentResponderIndex = 0;

		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Duel - [%s] challenges [%s], target must respond first"),
			*User->GetName(), *Target->GetPlayerName());

		RespComp->RequestResponse(DuelReq1,
			FOnResponseResolved::CreateLambda([UserPS, Target, RespComp, Tag_Slash = Tag_Card_Slash](const FResponseResult& R1) {
				if (!R1.bNegated)
				{
					// 目标没出杀 → 目标受1伤（伤害来源=UserPS）
					UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Duel - [%s] failed to slash, takes 1 damage"), *Target->GetPlayerName());
					Target->ApplyDamage(1, UserPS);
				}
				else
				{
					// 目标出杀 → 第2轮：使用者出杀
					UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Duel - [%s] slashed, now [%s] must respond"),
						*Target->GetPlayerName(), *UserPS->GetPlayerName());

					FResponseRequest DuelReq2;
					DuelReq2.Type = ETKResponseType::Sequential;
					DuelReq2.RequiredTag = Tag_Slash;
					DuelReq2.Initiator = Target;
					DuelReq2.PrimaryTarget = UserPS;
					DuelReq2.ResponderQueue.Add(UserPS);
					DuelReq2.CurrentResponderIndex = 0;

					RespComp->RequestResponse(DuelReq2,
						FOnResponseResolved::CreateLambda([UserPS, Target](const FResponseResult& R2) {
							if (!R2.bNegated)
							{
								// 使用者没出杀 → 使用者受1伤（伤害来源=Target）
								UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Duel - [%s] failed to slash, takes 1 damage"), *UserPS->GetPlayerName());
								UserPS->ApplyDamage(1, Target);
							}
							else
							{
								UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Duel - both slashed, no damage"));
							}
						}));
				}
			}));
	}
	else if (FirstTag.MatchesTag(Tag_Card_AOE_RequireSlash))
	{
		// 南蛮入侵：全员出杀已在 Sequential 响应流程中由 ResponseComponent 处理
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::AOE_Slash - [%s] Barbarian Invasion resolved by response system"), *User->GetName());
	}
	else if (FirstTag.MatchesTag(Tag_Card_AOE_RequireDodge))
	{
		// 万箭齐发：全员出闪已在 Sequential 响应流程中由 ResponseComponent 处理
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::AOE_Dodge - [%s] Volley of Arrows resolved by response system"), *User->GetName());
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
		// 无懈可击：由 ResponseComponent 的 Chain 流程消耗，此处不执行额外逻辑
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Negate - [%s] negate card consumed by response chain"), *User->GetName());
	}
	else if (FirstTag.MatchesTag(Tag_Card_Harvest))
	{
		// 五谷丰登 (M3 阶段实现)
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Harvest - [%s] plays Bountiful Harvest (pending M3)"), *User->GetName());
	}
	else if (FirstTag.MatchesTag(Tag_Card_BorrowKnife))
	{
		// 借刀杀人 (M3 阶段实现)
		UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::BorrowKnife - [%s] plays Borrow Knife on [%s] (pending M3)"),
			*User->GetName(), Target ? *Target->GetPlayerName() : TEXT("None"));
	}
	else
	{
		// 检查是否是 Draw 系列标签（Card.Effect.Draw = 无中生有等摸牌效果）
		bool bHandled = false;
		for (const FGameplayTag& Tag : EffectTags)
		{
			if (Tag.GetTagName().ToString().StartsWith(TagPrefix_Draw.ToString()))
			{
				// 解析摸牌数量
				int32 DrawCount = 2;
				TArray<FString> Parts;
				Tag.GetTagName().ToString().ParseIntoArray(Parts, TEXT("."));
				if (Parts.Num() > 0)
				{
					DrawCount = FCString::Atoi(*Parts.Last());
				}
				DrawCount = FMath::Max(DrawCount, 1);

				// 从牌堆摸牌到手牌
				ATKGameModeBase* GameMode = Cast<ATKGameModeBase>(User->GetWorld()->GetAuthGameMode());
				UTKDeckComponent* Deck = GameMode ? GameMode->GetDeckComponent() : nullptr;
				ATKPlayerStateBase* UserPS = Cast<ATKPlayerStateBase>(User->PlayerState);
				UTKCardZoneComponent* UserZone = UserPS ? UserPS->GetCardZone() : nullptr;

				if (Deck && UserZone)
				{
					TArray<UTKCardBase*> Drawn = Deck->DrawMultipleCards(DrawCount);
					for (UTKCardBase* Card : Drawn)
					{
						if (Card) UserZone->AddCardToHand(Card);
					}
					UE_LOG(LogTKGame, Log, TEXT("TKCard_Trick::Draw - [%s] draws %d cards"), *User->GetName(), Drawn.Num());
				}
				bHandled = true;
				break;
			}
		}
		if (!bHandled)
		{
			Super::OnUse(User, Target);
		}
	}
}
