#include "TKGamea.h"
// Fill out your copyright notice in the Description page of Project Settings.

#include "TKTurnComponentBase.h"
#include "TKGameStateBase.h"
#include "TKPlayerStateBase.h"
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
	// 默认摸牌逻辑：摸 2 张
	// 具体摸牌由 GameMode 的规则组件完成，这里仅输出日志
	UE_LOG(LogTKGame, Log, TEXT("  Draw Phase: default draw 2 cards"));
	AdvancePhase();
}

void UTKTurnComponentBase::OnDiscardPhase()
{
	// 默认弃牌逻辑：检查手牌数是否超过体力
	// 具体弃牌由 GameMode 的规则组件完成，这里仅输出日志
	UE_LOG(LogTKGame, Log, TEXT("  Discard Phase: discard down to health limit"));
	AdvancePhase();
}
