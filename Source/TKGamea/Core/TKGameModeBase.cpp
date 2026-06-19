// Fill out your copyright notice in the Description page of Project Settings.

#include "TKGameModeBase.h"
#include "TKGameStateBase.h"
#include "TKPlayerStateBase.h"
#include "TKPlayerControllerBase.h"
#include "TKTurnComponentBase.h"
#include "Cards/TKDeckComponent.h"
#include "Cards/TKCardTypes.h"
#include "Cards/TKCardZoneComponent.h"
#include "Rules/TKIdentityRuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"

ATKGameModeBase::ATKGameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 默认配置
	ModeConfig.PlayerCount = 4;
	ModeConfig.ModeType = ETKGameModeType::Identity;
	ModeConfig.InitialHandCount = 4;
	ModeConfig.DrawCountPerTurn = 2;
	ModeConfig.bEnableGenerals = false;

	// 创建规则和牌堆组件
	DeckComponent = CreateDefaultSubobject<UTKDeckComponent>("DeckComponent");
	IdentityRule = CreateDefaultSubobject<UTKIdentityRuleComponent>("IdentityRuleComponent");

	// 禁止自动开始匹配，等待 StartIdentityGame 手动触发
	bDelayedStart = true;

	// 默认类绑定
	PlayerControllerClass = ATKPlayerControllerBase::StaticClass();
	PlayerStateClass = ATKPlayerStateBase::StaticClass();
	GameStateClass = ATKGameStateBase::StaticClass();
}

void ATKGameModeBase::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	UE_LOG(LogTemp, Log, TEXT("GameMode: Match state set to %s"), *MatchState.ToString());
}

void ATKGameModeBase::StartIdentityGame()
{
	if (GetMatchState() != MatchState::WaitingToStart &&
		GetMatchState() != MatchState::EnteringMap)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartIdentityGame: Cannot start, match state is %s"), *MatchState.ToString());
		return;
	}

	// 1. 人数校验
	if (GameState == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("StartIdentityGame: GameState is null"));
		return;
	}
	int32 PlayerCount = GameState->PlayerArray.Num();
	if (PlayerCount < 4 || PlayerCount > 10)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartIdentityGame: Need 4-10 players, got %d"), PlayerCount);
		return;
	}
	ModeConfig.PlayerCount = PlayerCount;

	UE_LOG(LogTemp, Log, TEXT("=== Starting Identity Game with %d players ==="), PlayerCount);

	// 2. 初始化牌堆（调试牌堆）
	TArray<FTKDebugCardEntry> DebugDeck;
	for (int32 i = 0; i < 40; i++)
	{
		FTKDebugCardEntry Entry;
		Entry.CardDefId = FName(*FString::Printf(TEXT("DebugCard_%d"), i));
		Entry.CardType = ETKCardType::Basic;
		Entry.Suit = ETKCardSuit::NoSuit;
		Entry.Rank = 0;
		Entry.Count = 1;
		DebugDeck.Add(Entry);
	}
	DeckComponent->InitDebugDeck(DebugDeck);

	// 3. 分配身份
	IdentityRule->AssignIdentities(GameState->PlayerArray);

	// 4. 发起始手牌
	DealInitialCards();

	// 5. 设置主公先手
	SetLordFirstTurn();

	// 设置匹配状态为进行中
	SetMatchState(MatchState::InProgress);
}

void ATKGameModeBase::AssignIdentities()
{
	IdentityRule->AssignIdentities(GameState->PlayerArray);
}

void ATKGameModeBase::DealInitialCards()
{
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		ATKPlayerStateBase* TKPlayer = Cast<ATKPlayerStateBase>(PlayerState);
		if (TKPlayer == nullptr || !TKPlayer->IsAlive()) continue;

		UTKCardZoneComponent* CardZone = TKPlayer->GetCardZone();
		if (CardZone == nullptr) continue;

		// 发初始手牌
		TArray<UTKCardBase*> InitialCards = DeckComponent->DrawMultipleCards(ModeConfig.InitialHandCount);
		for (UTKCardBase* Card : InitialCards)
		{
			CardZone->AddCardToHand(Card);
		}
		TKPlayer->HandCardCount = CardZone->GetHandCardCount();

		UE_LOG(LogTemp, Log, TEXT("Dealt %d initial cards to player [%s]"),
			InitialCards.Num(), *TKPlayer->GetPlayerName());
	}
}

void ATKGameModeBase::SetLordFirstTurn()
{
	// 找到主公并设置先手
	APlayerState* LordPlayer = nullptr;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		ATKPlayerStateBase* TKPlayer = Cast<ATKPlayerStateBase>(PlayerState);
		if (TKPlayer && TKPlayer->Identity == ETKIdentity::Lord)
		{
			LordPlayer = TKPlayer;
			break;
		}
	}

	if (LordPlayer)
	{
		ATKGameStateBase* TKGameState = Cast<ATKGameStateBase>(GameState);
		if (TKGameState)
		{
			UTKTurnComponentBase* TurnComp = TKGameState->GetTurnComponent();
			if (TurnComp)
			{
				TurnComp->StartTurn(LordPlayer);
				UE_LOG(LogTemp, Log, TEXT("Lord [%s] starts first turn"), *LordPlayer->GetPlayerName());
			}
		}
	}
}

void ATKGameModeBase::EndGame(ETKGameResult Result, const TArray<APlayerState*>& Winners)
{
	ATKGameStateBase* TKGameState = Cast<ATKGameStateBase>(GameState);
	if (TKGameState == nullptr) return;

	FTKGameResultData ResultData;
	ResultData.WinnerFaction = Result;
	ResultData.WinningPlayers = Winners;
	TKGameState->GameResult = ResultData;

	FString ResultStr;
	switch (Result)
	{
	case ETKGameResult::LordLoyalistWin: ResultStr = TEXT("Lord & Loyalist"); break;
	case ETKGameResult::RebelWin:		ResultStr = TEXT("Rebels");	break;
	case ETKGameResult::RenegadeWin:	ResultStr = TEXT("Renegade");	break;
	default:							ResultStr = TEXT("Unknown");	break;
	}
	UE_LOG(LogTemp, Log, TEXT("=== GAME OVER! %s Wins! ==="), *ResultStr);
}
