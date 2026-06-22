// Fill out your copyright notice in the Description page of Project Settings.

#include "TKTurnComponentBase.h"
#include "TKGamea.h"
#include "TKGameStateBase.h"
#include "TKPlayerStateBase.h"
#include "TKGameModeBase.h"
#include "Cards/TKDeckComponent.h"
#include "Cards/TKCardZoneComponent.h"
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
		// 准备阶段：M1 阶段暂无特殊逻辑，直接推进
		AdvancePhase();
		break;

	case ETKTurnPhase::Judge:
		// 判定阶段：M1 阶段暂无延时锦囊，直接推进
		AdvancePhase();
		break;

	case ETKTurnPhase::Draw:
		// 摸牌阶段：默认摸 2 张
		OnDrawPhase();
		break;

	case ETKTurnPhase::Play:
		// 出牌阶段：重置杀计数，等待玩家操作
		// 玩家通过 ServerAdvancePhase 主动结束出牌阶段
		SlashUsedThisPhase = 0;
		UE_LOG(LogTKGame, Log, TEXT("  Play Phase: waiting for player actions..."));
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
