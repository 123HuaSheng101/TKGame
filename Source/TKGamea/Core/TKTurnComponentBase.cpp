// Fill out your copyright notice in the Description page of Project Settings.

#include "TKTurnComponentBase.h"
#include "TKGamea.h"
#include "TKGameStateBase.h"
#include "TKPlayerStateBase.h"
#include "TKGameModeBase.h"
#include "Cards/TKDeckComponent.h"
#include "Cards/TKCardZoneComponent.h"
#include "Game/TKCard_DelayedTrick.h"
#include "Game/TKCard_Equipment.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

UTKTurnComponentBase::UTKTurnComponentBase(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	CurrentPhase = ETKTurnPhase::Prepare;
	CurrentTurnNumber = 0;
	SlashUsedThisPhase = 0;
}

UTKTurnComponentBase* UTKTurnComponentBase::Get(const ATKGameStateBase* GameState)
{
	if (GameState == nullptr)
		return nullptr;
	if (!IsValid(GameState))
		return nullptr;
	return GameState->TurnComponent.Get();
}

void UTKTurnComponentBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTKTurnComponentBase, CurrentPlayer);
	DOREPLIFETIME(UTKTurnComponentBase, CurrentPhase);
	DOREPLIFETIME(UTKTurnComponentBase, CurrentTurnNumber);
}

void UTKTurnComponentBase::StartTurn(APlayerState* Player)
{
	if (Player == nullptr) return;

	CurrentPlayer = Player;
	CurrentTurnNumber++;
	SlashUsedThisPhase = 0;
	bWined = false;
	bSkipDrawPhase = false;
	bSkipPlayPhase = false;
	CurrentPhase = ETKTurnPhase::Prepare;

	UE_LOG(LogTKGame, Log, TEXT("=== Turn %d Start: Player [%s] ==="), CurrentTurnNumber, *Player->GetPlayerName());
	EnterPhase(ETKTurnPhase::Prepare);
}

void UTKTurnComponentBase::EnterPhase(ETKTurnPhase NewPhase)
{
	CurrentPhase = NewPhase;
	UE_LOG(LogTKGame, Log, TEXT("  Enter Phase: %d"), (uint8)CurrentPhase);

	switch (CurrentPhase)
	{
	case ETKTurnPhase::Prepare:
		// 准备阶段：暂无特殊逻辑，直接推进
		AdvancePhase();
		break;

	case ETKTurnPhase::Judge:
		// 判定阶段：结算延时锦囊
		OnJudgePhase();
		break;

	case ETKTurnPhase::Draw:
		// 摸牌阶段：检查兵粮寸断 Skip 标记
		if (bSkipDrawPhase)
		{
			UE_LOG(LogTKGame, Log, TEXT("  Draw Phase SKIPPED (兵粮寸断) for [%s]"), *CurrentPlayer->GetPlayerName());
			AdvancePhase();
		}
		else
		{
			OnDrawPhase();
		}
		break;

	case ETKTurnPhase::Play:
		// 出牌阶段：检查乐不思蜀 Skip 标记
		if (bSkipPlayPhase)
		{
			UE_LOG(LogTKGame, Log, TEXT("  Play Phase SKIPPED (乐不思蜀) for [%s]"), *CurrentPlayer->GetPlayerName());
			AdvancePhase();
		}
		else
		{
			SlashUsedThisPhase = 0;
			UE_LOG(LogTKGame, Log, TEXT("  Play Phase: waiting for player actions..."));
		}
		break;

	case ETKTurnPhase::Discard:
		// 弃牌阶段：校验手牌数
		OnDiscardPhase();
		break;

	case ETKTurnPhase::Finish:
		// 结束阶段
		EndTurn();
		break;
	}
}

void UTKTurnComponentBase::AdvancePhase()
{
	switch (CurrentPhase)
	{
	case ETKTurnPhase::Prepare:		EnterPhase(ETKTurnPhase::Judge);	break;
	case ETKTurnPhase::Judge:		EnterPhase(ETKTurnPhase::Draw);		break;
	case ETKTurnPhase::Draw:		EnterPhase(ETKTurnPhase::Play);		break;
	case ETKTurnPhase::Play:		EnterPhase(ETKTurnPhase::Discard);	break;
	case ETKTurnPhase::Discard:		EnterPhase(ETKTurnPhase::Finish);	break;
	case ETKTurnPhase::Finish:		EndTurn();							break;
	}
}

void UTKTurnComponentBase::EndTurn()
{
	UE_LOG(LogTKGame, Log, TEXT("=== Turn %d End: Player [%s] ==="), CurrentTurnNumber,
		CurrentPlayer ? *CurrentPlayer->GetPlayerName() : TEXT("None"));

	APlayerState* NextPlayer = FindNextAlivePlayer();

	if (NextPlayer == nullptr || NextPlayer == CurrentPlayer)
	{
		// 只剩一个存活玩家或找不到下一家，游戏结束
		CurrentPlayer = nullptr;
		CurrentPhase = ETKTurnPhase::Prepare;
		UE_LOG(LogTKGame, Log, TEXT("Only one player alive, game over!"));
		return;
	}

	// 切换到下一个存活玩家，开始新回合
	StartTurn(NextPlayer);
}

void UTKTurnComponentBase::Stop()
{
	CurrentPlayer = nullptr;
	CurrentPhase = ETKTurnPhase::Prepare;
	UE_LOG(LogTKGame, Log, TEXT("TurnComponent: Stopped"));
}

APlayerState* UTKTurnComponentBase::FindNextAlivePlayer() const
{
	AGameStateBase* GS = Cast<AGameStateBase>(GetOwner());
	if (GS == nullptr) return nullptr;

	const TArray<TObjectPtr<APlayerState>>& Players = GS->PlayerArray;
	if (Players.Num() == 0) return nullptr;

	// 找当前玩家在 PlayerArray 中的索引
	int32 CurrentIndex = Players.IndexOfByKey(CurrentPlayer);
	if (CurrentIndex == INDEX_NONE)
	{
		// 当前玩家已不在列表中（已离开），从头找第一个存活玩家
		for (const auto& PS : Players)
		{
			ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
			if (TKPS && TKPS->IsAlive())
				return PS;
		}
		return nullptr;
	}

	// 从下一家开始循环查找，跳过死亡玩家
	int32 NumPlayers = Players.Num();
	for (int32 Offset = 1; Offset < NumPlayers; Offset++)
	{
		int32 NextIndex = (CurrentIndex + Offset) % NumPlayers;
		ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(Players[NextIndex]);
		if (TKPS && TKPS->IsAlive())
			return Players[NextIndex];
	}

	return nullptr;
}

void UTKTurnComponentBase::OnDrawPhase()
{
	// 摸 2 张牌到当前玩家手牌
	ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(CurrentPlayer);
	if (!TKPS || !TKPS->IsAlive())
	{
		AdvancePhase();
		return;
	}

	ATKGameModeBase* GameMode = Cast<ATKGameModeBase>(GetWorld()->GetAuthGameMode());
	if (!GameMode)
	{
		AdvancePhase();
		return;
	}

	UTKDeckComponent* Deck = GameMode->GetDeckComponent();
	UTKCardZoneComponent* Zone = TKPS->GetCardZone();
	if (Deck && Zone)
	{
		TArray<UTKCardBase*> Drawn = Deck->DrawMultipleCards(2);
		for (UTKCardBase* Card : Drawn)
		{
			Zone->AddCardToHand(Card);
		}
		TKPS->HandCardCount = Zone->GetHandCardCount();
		UE_LOG(LogTKGame, Log, TEXT("  Draw Phase: [%s] drew %d cards"), *TKPS->GetPlayerName(), Drawn.Num());
	}

	AdvancePhase();
}

void UTKTurnComponentBase::OnJudgePhase()
{
	ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(CurrentPlayer);
	if (!TKPS || !TKPS->IsAlive())
	{
		AdvancePhase();
		return;
	}

	UTKCardZoneComponent* Zone = TKPS->GetCardZone();
	if (!Zone)
	{
		AdvancePhase();
		return;
	}

	// 获取牌堆用于判定
	ATKGameModeBase* GameMode = Cast<ATKGameModeBase>(GetWorld()->GetAuthGameMode());
	UTKDeckComponent* Deck = GameMode ? GameMode->GetDeckComponent() : nullptr;

	TArray<UTKCardBase*> JudgementCards = Zone->GetJudgementCards();
	if (JudgementCards.Num() == 0)
	{
		UE_LOG(LogTKGame, Log, TEXT("  Judge Phase: [%s] no delayed tricks"), *TKPS->GetPlayerName());
		AdvancePhase();
		return;
	}

	// 遍历并结算延时锦囊
	for (UTKCardBase* Card : JudgementCards)
	{
		UTKCard_DelayedTrick* DT = Cast<UTKCard_DelayedTrick>(Card);
		if (!DT) continue;

		UE_LOG(LogTKGame, Log, TEXT("  Judge Phase: [%s] judging [%s]"), *TKPS->GetPlayerName(), *Card->CardDefId.ToString());

		// 执行判定（通过 DeckComponent）
		if (Deck)
		{
			FGameplayTag EffectTag = Card->EffectTags.Num() > 0 ? Card->EffectTags[0] : FGameplayTag();
			ETKCardSuit JudgedSuit;
			int32 JudgedRank;
			UTKCardBase* JudgedCard = nullptr;

			bool bEffective = Deck->ExecuteJudgement(EffectTag, JudgedSuit, JudgedRank, JudgedCard);

			if (bEffective)
			{
				static FGameplayTag Tag_SkipPlay  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Skip.Play"),  false);
				static FGameplayTag Tag_SkipDraw  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Skip.Draw"),  false);
				static FGameplayTag Tag_Lightning = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Lightning"),  false);

				if (EffectTag == Tag_SkipPlay)
				{
					bSkipPlayPhase = true;
					UE_LOG(LogTKGame, Log, TEXT("    → 乐不思蜀生效！[%s] skips Play phase"), *TKPS->GetPlayerName());
				}
				else if (EffectTag == Tag_SkipDraw)
				{
					bSkipDrawPhase = true;
					UE_LOG(LogTKGame, Log, TEXT("    → 兵粮寸断生效！[%s] skips Draw phase"), *TKPS->GetPlayerName());
				}
				else if (EffectTag == Tag_Lightning)
				{
					TKPS->ApplyDamage(3);
					UE_LOG(LogTKGame, Log, TEXT("    → 闪电生效！[%s] takes 3 damage"), *TKPS->GetPlayerName());
				}
			}
			else
			{
				// 判定失效 → 闪电传给下家
				static FGameplayTag Tag_Lightning = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Lightning"), false);
				if (EffectTag == Tag_Lightning)
				{
					// 从当前判定区移除
					Zone->RemoveFromJudgement(DT);

					// 找下一个存活玩家
					APlayerState* NextPlayer = FindNextAlivePlayer();
					if (NextPlayer && NextPlayer != TKPS)
					{
						ATKPlayerStateBase* NextTK = Cast<ATKPlayerStateBase>(NextPlayer);
						if (NextTK)
						{
							UTKCardZoneComponent* NextZone = NextTK->GetCardZone();
							if (NextZone)
							{
								NextZone->AddToJudgement(DT);
								UE_LOG(LogTKGame, Log, TEXT("    → 闪电未生效，传给 [%s]"), *NextTK->GetPlayerName());
							}
						}
					}
					continue; // 已手动处理移除，跳过下方统一移除
				}
				UE_LOG(LogTKGame, Log, TEXT("    → 判定未生效，[%s] of [%s] has no effect"),
					*Card->CardDefId.ToString(), *TKPS->GetPlayerName());
			}
		}

		// 结算完毕，从判定区移除并弃牌
		Zone->RemoveFromJudgement(DT);
		if (Deck) Deck->DiscardCard(DT);
	}

	AdvancePhase();
}

void UTKTurnComponentBase::OnDiscardPhase()
{
	ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(CurrentPlayer);
	if (!TKPS || !TKPS->IsAlive())
	{
		AdvancePhase();
		return;
	}

	UTKCardZoneComponent* Zone = TKPS->GetCardZone();
	if (Zone && Zone->GetHandCardCount() > TKPS->CurrentHealth)
	{
		// TODO: 玩家手动选择弃牌，此处自动随机弃掉多余的牌
		ATKGameModeBase* GameMode = Cast<ATKGameModeBase>(GetWorld()->GetAuthGameMode());
		UTKDeckComponent* Deck = GameMode ? GameMode->GetDeckComponent() : nullptr;
		int32 ToDiscard = Zone->GetHandCardCount() - TKPS->CurrentHealth;
		UE_LOG(LogTKGame, Log, TEXT("  Discard Phase: [%s] needs to discard %d cards (hand=%d > hp=%d)"),
			*TKPS->GetPlayerName(), ToDiscard, Zone->GetHandCardCount(), TKPS->CurrentHealth);

		if (Deck)
		{
			TArray<UTKCardBase*> Hand = Zone->GetHandCards();
			for (int32 i = 0; i < ToDiscard && Hand.Num() > 0; i++)
			{
				int32 Idx = FMath::RandRange(0, Hand.Num() - 1);
				UTKCardBase* Card = Hand[Idx];
				Zone->RemoveCardFromHand(Card);
				Deck->DiscardCard(Card);
			}
			TKPS->HandCardCount = Zone->GetHandCardCount();
		}
	}

	AdvancePhase();
}

// ===== 装备 / 距离系统 =====

int32 UTKTurnComponentBase::GetRawDistance(ATKPlayerStateBase* From, ATKPlayerStateBase* To) const
{
	if (!From || !To) return 1;

	AGameStateBase* GS = Cast<AGameStateBase>(GetOwner());
	if (!GS) return 1;

	const TArray<TObjectPtr<APlayerState>>& Players = GS->PlayerArray;
	int32 FromIdx = Players.IndexOfByKey(From);
	int32 ToIdx   = Players.IndexOfByKey(To);
	if (FromIdx == INDEX_NONE || ToIdx == INDEX_NONE) return 1;

	int32 TotalAlive = 0;
	for (const auto& PS : Players)
	{
		ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
		if (TKPS && TKPS->IsAlive()) TotalAlive++;
	}

	// 计算存活玩家环上的最短距离
	int32 RawDist = FMath::Abs(ToIdx - FromIdx);
	int32 Dist = FMath::Min(RawDist, TotalAlive - RawDist);
	return FMath::Max(1, Dist);
}

int32 UTKTurnComponentBase::GetWeaponRange(ATKPlayerStateBase* Player) const
{
	if (!Player) return 1;

	UTKCardZoneComponent* Zone = Player->GetCardZone();
	if (!Zone) return 1;

	static FGameplayTag Tag_Equip_Weapon = FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Weapon"), false);

	TArray<UTKCardBase*> EquipCards = Zone->GetEquipmentCards();
	for (UTKCardBase* Card : EquipCards)
	{
		if (!Card) continue;
		for (const FGameplayTag& Tag : Card->EffectTags)
		{
			if (Tag.MatchesTag(Tag_Equip_Weapon))
			{
				// TODO: 具体武器范围应从配置表读取，此处默认武器范围=3
				return 3;
			}
		}
	}
	return 1;
}

bool UTKTurnComponentBase::CanReachTarget(ATKPlayerStateBase* Attacker, ATKPlayerStateBase* Defender) const
{
	if (!Attacker || !Defender) return false;
	if (Attacker == Defender) return false;

	int32 RawDist = GetRawDistance(Attacker, Defender);
	int32 WeaponRange = GetWeaponRange(Attacker);

	// 检查攻击者是否有 -1马
	bool bAttackerHasMinus1 = false;
	UTKCardZoneComponent* AttackerZone = Attacker->GetCardZone();
	if (AttackerZone)
	{
		static FGameplayTag Tag_Equip_Offense = FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Offense"), false);
		for (UTKCardBase* Card : AttackerZone->GetEquipmentCards())
		{
			if (Card)
			{
				for (const FGameplayTag& Tag : Card->EffectTags)
				{
					if (Tag.MatchesTag(Tag_Equip_Offense)) { bAttackerHasMinus1 = true; break; }
				}
			}
			if (bAttackerHasMinus1) break;
		}
	}

	// 检查防御者是否有 +1马
	bool bDefenderHasPlus1 = false;
	UTKCardZoneComponent* DefenderZone = Defender->GetCardZone();
	if (DefenderZone)
	{
		static FGameplayTag Tag_Equip_Defense = FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Defense"), false);
		for (UTKCardBase* Card : DefenderZone->GetEquipmentCards())
		{
			if (Card)
			{
				for (const FGameplayTag& Tag : Card->EffectTags)
				{
					if (Tag.MatchesTag(Tag_Equip_Defense)) { bDefenderHasPlus1 = true; break; }
				}
			}
			if (bDefenderHasPlus1) break;
		}
	}

	// 有效攻击范围 = 武器范围 + (-1马) - (目标+1马)
	int32 EffectiveRange = WeaponRange + (bAttackerHasMinus1 ? 1 : 0) - (bDefenderHasPlus1 ? 1 : 0);
	EffectiveRange = FMath::Max(1, EffectiveRange);

	return RawDist <= EffectiveRange;
}

bool UTKTurnComponentBase::HasBaguaArmor(ATKPlayerStateBase* Player) const
{
	if (!Player) return false;

	UTKCardZoneComponent* Zone = Player->GetCardZone();
	if (!Zone) return false;

	static FGameplayTag Tag_Equip_Armor = FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Armor"), false);

	for (UTKCardBase* Card : Zone->GetEquipmentCards())
	{
		if (!Card) continue;
		for (const FGameplayTag& Tag : Card->EffectTags)
		{
			if (Tag.MatchesTag(Tag_Equip_Armor)) return true;
		}
	}
	return false;
}
