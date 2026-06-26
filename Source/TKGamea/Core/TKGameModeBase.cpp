// Fill out your copyright notice in the Description page of Project Settings.

#include "TKGameModeBase.h"
#include "TKGamea.h"
#include "TKGameHUD.h"
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
	HUDClass = ATKGameHUD::StaticClass();
	PlayerControllerClass = ATKPlayerControllerBase::StaticClass();
	PlayerStateClass = ATKPlayerStateBase::StaticClass();
	GameStateClass = ATKGameStateBase::StaticClass();
}

void ATKGameModeBase::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	UE_LOG(LogTKGame, Log, TEXT("GameMode: Match state set to %s"), *MatchState.ToString());
}

void ATKGameModeBase::StartIdentityGame()
{
	if (GetMatchState() != MatchState::WaitingToStart &&
		GetMatchState() != MatchState::EnteringMap)
	{
		UE_LOG(LogTKGame, Warning, TEXT("StartIdentityGame: Cannot start, match state is %s"), *MatchState.ToString());
		return;
	}

	// 1. 人数校验
	if (GameState == nullptr)
	{
		UE_LOG(LogTKGame, Error, TEXT("StartIdentityGame: GameState is null"));
		return;
	}
	int32 PlayerCount = GameState->PlayerArray.Num();
	if (PlayerCount < 4 || PlayerCount > 10)
	{
		UE_LOG(LogTKGame, Warning, TEXT("StartIdentityGame: Need 4-10 players, got %d"), PlayerCount);
		return;
	}
	ModeConfig.PlayerCount = PlayerCount;

	UE_LOG(LogTKGame, Log, TEXT("=== Starting Identity Game with %d players ==="), PlayerCount);

	// 2. 初始化牌堆（优先 DataTable，回退到调试牌堆）
	if (DeckDataTable)
	{
		DeckComponent->InitFromDataTable(DeckDataTable);
	}
	else
	{
		TArray<FTKDebugCardEntry> DebugDeck;
		for (int32 i = 0; i < 2; i++)
		{
			DebugDeck.Add({ FName(TEXT("Slash_S")),  FText::FromString(TEXT("杀♠")),  ETKCardType::Basic, ETKCardSuit::Spade,   7 + i, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Slash")) } });
			DebugDeck.Add({ FName(TEXT("Slash_H")),  FText::FromString(TEXT("杀♥")),  ETKCardType::Basic, ETKCardSuit::Heart,  10 + i, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Slash")) } });
			DebugDeck.Add({ FName(TEXT("Slash_C")),  FText::FromString(TEXT("杀♣")),  ETKCardType::Basic, ETKCardSuit::Club,    7 + i, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Slash")) } });
			DebugDeck.Add({ FName(TEXT("Slash_D")),  FText::FromString(TEXT("杀♦")),  ETKCardType::Basic, ETKCardSuit::Diamond, 10 + i, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Slash")) } });
		}
		for (int32 i = 0; i < 3; i++)
		{
			DebugDeck.Add({ FName(TEXT("Dodge_H")), FText::FromString(TEXT("闪♥")), ETKCardType::Basic, ETKCardSuit::Heart,   2 + i * 2, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Dodge")) } });
			DebugDeck.Add({ FName(TEXT("Dodge_D")), FText::FromString(TEXT("闪♦")), ETKCardType::Basic, ETKCardSuit::Diamond, 2 + i * 2, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Dodge")) } });
		}
		for (int32 i = 0; i < 2; i++)
		{
			DebugDeck.Add({ FName(TEXT("Peach_H")), FText::FromString(TEXT("桃♥")), ETKCardType::Basic, ETKCardSuit::Heart,   3 + i * 4, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Peach")) } });
			DebugDeck.Add({ FName(TEXT("Peach_D")), FText::FromString(TEXT("桃♦")), ETKCardType::Basic, ETKCardSuit::Diamond, 4 + i * 3, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Peach")) } });
		}
		DebugDeck.Add({ FName(TEXT("Wine_C")), FText::FromString(TEXT("酒♣")), ETKCardType::Basic, ETKCardSuit::Club, 9, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Wine")) } });
		DebugDeck.Add({ FName(TEXT("Wine_D")), FText::FromString(TEXT("酒♦")), ETKCardType::Basic, ETKCardSuit::Diamond, 9, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Wine")) } });
		DebugDeck.Add({ FName(TEXT("Snatch_D")),     FText::FromString(TEXT("顺手牵羊♦")), ETKCardType::Trick, ETKCardSuit::Diamond, 3, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Snatch")) } });
		DebugDeck.Add({ FName(TEXT("Snatch_S")),     FText::FromString(TEXT("顺手牵羊♠")), ETKCardType::Trick, ETKCardSuit::Spade,   4, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Snatch")) } });
		DebugDeck.Add({ FName(TEXT("Dismantle_H")),  FText::FromString(TEXT("过河拆桥♥")), ETKCardType::Trick, ETKCardSuit::Heart,   5, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Dismantle")) } });
		DebugDeck.Add({ FName(TEXT("Dismantle_C")),  FText::FromString(TEXT("过河拆桥♣")), ETKCardType::Trick, ETKCardSuit::Club,    6, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Dismantle")) } });
		DebugDeck.Add({ FName(TEXT("Harvest_H")),    FText::FromString(TEXT("无中生有♥")), ETKCardType::Trick, ETKCardSuit::Heart,   7, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Draw")) } });
		DebugDeck.Add({ FName(TEXT("Harvest_D")),    FText::FromString(TEXT("无中生有♦")), ETKCardType::Trick, ETKCardSuit::Diamond, 8, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Draw")) } });
		DebugDeck.Add({ FName(TEXT("Negate_C")),     FText::FromString(TEXT("无懈可击♣")), ETKCardType::Trick, ETKCardSuit::Club,   11, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Negate")) } });
		DebugDeck.Add({ FName(TEXT("Negate_S")),     FText::FromString(TEXT("无懈可击♠")), ETKCardType::Trick, ETKCardSuit::Spade,   11, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Negate")) } });
		DebugDeck.Add({ FName(TEXT("Duel_S")),       FText::FromString(TEXT("决斗♠")),     ETKCardType::Trick, ETKCardSuit::Spade,    1, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Duel")) } });
		DebugDeck.Add({ FName(TEXT("Barbarian_S")),  FText::FromString(TEXT("南蛮入侵♠")), ETKCardType::Trick, ETKCardSuit::Spade,    7, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.AOE.RequireSlash")) } });
		DebugDeck.Add({ FName(TEXT("Volley_H")),     FText::FromString(TEXT("万箭齐发♥")), ETKCardType::Trick, ETKCardSuit::Heart,    1, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.AOE.RequireDodge")) } });
		DebugDeck.Add({ FName(TEXT("Spear")),        FText::FromString(TEXT("丈八蛇矛♠")), ETKCardType::Equipment, ETKCardSuit::Spade, 12, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Weapon")) } });
		DebugDeck.Add({ FName(TEXT("Blade")),        FText::FromString(TEXT("青龙偃月刀♠")), ETKCardType::Equipment, ETKCardSuit::Spade, 5, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Weapon")) } });
		DebugDeck.Add({ FName(TEXT("Shield")),       FText::FromString(TEXT("八卦阵♠")),   ETKCardType::Equipment, ETKCardSuit::Spade,  2, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Armor")) } });
		DebugDeck.Add({ FName(TEXT("Plus1Horse")),   FText::FromString(TEXT("+1马♣")),     ETKCardType::Equipment, ETKCardSuit::Club,    5, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Defense")) } });
		DebugDeck.Add({ FName(TEXT("Minus1Horse")),  FText::FromString(TEXT("-1马♠")),     ETKCardType::Equipment, ETKCardSuit::Spade,   13, 1, { FGameplayTag::RequestGameplayTag(TEXT("Card.Equip.Offense")) } });
		UE_LOG(LogTKGame, Log, TEXT("Test deck: %d cards total"), DebugDeck.Num());
		DeckComponent->InitDebugDeck(DebugDeck);
	}

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

		UE_LOG(LogTKGame, Log, TEXT("Dealt %d initial cards to player [%s]"),
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
				UE_LOG(LogTKGame, Log, TEXT("Lord [%s] starts first turn"), *LordPlayer->GetPlayerName());
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
	UE_LOG(LogTKGame, Log, TEXT("=== GAME OVER! %s Wins! ==="), *ResultStr);
}
