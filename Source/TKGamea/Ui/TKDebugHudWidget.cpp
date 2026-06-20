#include "TKGamea.h"
#include "TKDebugHudWidget.h"
#include "Core/TKGameStateBase.h"
#include "Core/TKPlayerStateBase.h"
#include "Core/TKTurnComponentBase.h"
#include "Cards/TKDeckComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Components/TextBlock.h"

UTKDebugHudWidget::UTKDebugHudWidget(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
}

void UTKDebugHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshDisplay();
}

void UTKDebugHudWidget::RefreshDisplay()
{
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	ATKGameStateBase* TKGameState = Cast<ATKGameStateBase>(GameState);

	if (TKGameState == nullptr)
	{
		if (TurnInfoText) TurnInfoText->SetText(FText::FromString(TEXT("Game State: N/A")));
		return;
	}

	UTKTurnComponentBase* TurnComp = TKGameState->GetTurnComponent();

	// 回合信息
	{
		FString TurnStr = TEXT("No active turn");
		if (TurnComp && TurnComp->GetCurrentPlayer())
		{
			TurnStr = FString::Printf(TEXT("Turn %d | Current: %s"),
				TurnComp->GetCurrentTurnNumber(),
				*TurnComp->GetCurrentPlayer()->GetPlayerName());
		}
		if (TurnInfoText) TurnInfoText->SetText(FText::FromString(TurnStr));
	}

	// 阶段信息
	{
		FString PhaseStr = TEXT("Phase: ");
		if (TurnComp)
		{
			PhaseStr += GetPhaseName(TurnComp->GetCurrentPhase());
		}
		else
		{
			PhaseStr += TEXT("N/A");
		}
		if (PhaseText) PhaseText->SetText(FText::FromString(PhaseStr));
	}

	// 玩家状态表
	{
		FString StatusStr = TEXT("--- Players ---\n");
		StatusStr += FString::Printf(TEXT("%-4s %-16s %-10s %-6s %-6s %-6s\n"),
			TEXT("Seat"), TEXT("Name"), TEXT("Identity"), TEXT("Alive"), TEXT("HP"), TEXT("Hand"));

		for (const APlayerState* PS : TKGameState->PlayerArray)
		{
			const ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
			if (TKPS == nullptr) continue;

			StatusStr += FString::Printf(TEXT("%-4d %-16s %-10s %-6s %-3d/%-3d %-6d\n"),
				TKPS->SeatIndex,
				*TKPS->GetPlayerName(),
				*GetIdentityName(TKPS->Identity),
				TKPS->IsAlive() ? TEXT("Alive") : TEXT("Dead"),
				TKPS->CurrentHealth,
				TKPS->MaxHealth,
				TKPS->HandCardCount);
		}
		if (PlayerStatusText) PlayerStatusText->SetText(FText::FromString(StatusStr));
	}

	// 牌堆信息
	{
		FString DeckStr = TEXT("Deck: N/A");
		AGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr;
		if (GameMode)
		{
			// 通过 GameMode 获取 DeckComponent
			UTKDeckComponent* DeckComp = GameMode->FindComponentByClass<UTKDeckComponent>();
			if (DeckComp)
			{
				DeckStr = FString::Printf(TEXT("Deck: %d cards | Discard: %d cards"),
					DeckComp->GetDeckCount(), DeckComp->GetDiscardPileCount());
			}
		}
		if (DeckInfoText) DeckInfoText->SetText(FText::FromString(DeckStr));
	}

	// 游戏结果
	{
		FString ResultStr = TEXT("Game: In Progress");
		if (TKGameState->GameResult.WinnerFaction != ETKGameResult::None)
		{
			FString WinnerName;
			switch (TKGameState->GameResult.WinnerFaction)
			{
			case ETKGameResult::LordLoyalistWin: WinnerName = TEXT("Lord & Loyalist"); break;
			case ETKGameResult::RebelWin:		 WinnerName = TEXT("Rebels"); break;
			case ETKGameResult::RenegadeWin:	 WinnerName = TEXT("Renegade"); break;
			default:							 WinnerName = TEXT("Unknown"); break;
			}
			ResultStr = FString::Printf(TEXT("!!! GAME OVER: %s Wins !!!"), *WinnerName);

			ResultStr += TEXT("\nWinners:");
			for (const auto& Winner : TKGameState->GameResult.WinningPlayers)
			{
				if (Winner) ResultStr += FString::Printf(TEXT("\n  - %s"), *Winner->GetPlayerName());
			}
		}
		if (GameResultText) GameResultText->SetText(FText::FromString(ResultStr));
	}
}

FString UTKDebugHudWidget::GetPhaseName(ETKTurnPhase Phase) const
{
	switch (Phase)
	{
	case ETKTurnPhase::Prepare: return TEXT("Prepare");
	case ETKTurnPhase::Judge:	return TEXT("Judge");
	case ETKTurnPhase::Draw:	return TEXT("Draw");
	case ETKTurnPhase::Play:	return TEXT("Play");
	case ETKTurnPhase::Discard:	return TEXT("Discard");
	case ETKTurnPhase::Finish:	return TEXT("Finish");
	default:					return TEXT("Unknown");
	}
}

FString UTKDebugHudWidget::GetIdentityName(ETKIdentity Identity) const
{
	switch (Identity)
	{
	case ETKIdentity::Unknown:	return TEXT("???");
	case ETKIdentity::Lord:		return TEXT("Lord");
	case ETKIdentity::Loyalist:	return TEXT("Loyalist");
	case ETKIdentity::Rebel:	return TEXT("Rebel");
	case ETKIdentity::Renegade:	return TEXT("Renegade");
	default:					return TEXT("???");
	}
}
