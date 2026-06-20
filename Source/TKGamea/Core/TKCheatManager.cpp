#include "TKGamea.h"
// Fill out your copyright notice in the Description page of Project Settings.

#include "TKCheatManager.h"
#include "TKPlayerControllerBase.h"
#include "TKPlayerStateBase.h"
#include "TKGameModeBase.h"
#include "TKGameStateBase.h"
#include "TKTurnComponentBase.h"
#include "TKResponseComponent.h"
#include "TKEventComponentBase.h"
#include "Game/TKCardBase.h"
#include "Cards/TKCardZoneComponent.h"
#include "Ui/TKDebugHudWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"

UTKCheatManager::UTKCheatManager()
{
}

// ===== 游戏控制命令 =====

void UTKCheatManager::StartDebugGame()
{
	ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetPlayerController());
	if (PC == nullptr) return;

	if (PC->HasAuthority())
	{
		ATKGameModeBase* GameMode = PC->GetWorld()->GetAuthGameMode<ATKGameModeBase>();
		if (GameMode)
		{
			GameMode->StartIdentityGame();
		}
	}
	else
	{
		PC->ServerStartGame();
	}
}

void UTKCheatManager::AdvancePhase()
{
	ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetPlayerController());
	if (PC == nullptr) return;

	if (PC->HasAuthority())
	{
		ATKGameStateBase* TKGameState = PC->GetWorld() ? PC->GetWorld()->GetGameState<ATKGameStateBase>() : nullptr;
		if (TKGameState)
		{
			UTKTurnComponentBase* TurnComp = TKGameState->GetTurnComponent();
			if (TurnComp && TurnComp->GetCurrentPlayer())
			{
				TurnComp->AdvancePhase();
			}
		}
	}
	else
	{
		PC->ServerAdvancePhase();
	}
}

// ===== 卡牌测试命令 =====

void UTKCheatManager::ShowHand()
{
	ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetPlayerController());
	if (!PC) return;

	ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PC->PlayerState);
	if (!TKPS) return;

	UTKCardZoneComponent* Zone = TKPS->GetCardZone();
	if (!Zone) return;

	TArray<UTKCardBase*> Hand = Zone->GetHandCards();
	UE_LOG(LogTKGame, Log, TEXT("========== [%s] Hand (%d cards) =========="), *TKPS->GetPlayerName(), Hand.Num());
	for (int32 i = 0; i < Hand.Num(); i++)
	{
		UTKCardBase* Card = Hand[i];
		if (Card)
		{
			FString TagStr = Card->EffectTags.Num() > 0 ? Card->EffectTags[0].GetTagName().ToString() : TEXT("None");
			UE_LOG(LogTKGame, Log, TEXT("  [%d] %s | Type=%d | Suit=%d | Rank=%d | Tag=%s"),
				i, *Card->CardName.ToString(), (uint8)Card->CardType, (uint8)Card->Suit, Card->Rank, *TagStr);
		}
	}
	UE_LOG(LogTKGame, Log, TEXT("============================================"));
}

void UTKCheatManager::UseCard(int32 CardIndex, int32 TargetPlayerIndex)
{
	ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetPlayerController());
	if (!PC) { UE_LOG(LogTKGame, Error, TEXT("UseCard: PC is null")); return; }

	ATKPlayerStateBase* UserPS = Cast<ATKPlayerStateBase>(PC->PlayerState);
	if (!UserPS) { UE_LOG(LogTKGame, Error, TEXT("UseCard: UserPS is null")); return; }

	UTKCardZoneComponent* Zone = UserPS->GetCardZone();
	if (!Zone) { UE_LOG(LogTKGame, Error, TEXT("UseCard: Zone is null")); return; }

	TArray<UTKCardBase*> Hand = Zone->GetHandCards();
	if (CardIndex < 0 || CardIndex >= Hand.Num())
	{
		UE_LOG(LogTKGame, Error, TEXT("UseCard: Invalid card index %d (hand size=%d)"), CardIndex, Hand.Num());
		return;
	}

	UTKCardBase* Card = Hand[CardIndex];
	if (!Card) { UE_LOG(LogTKGame, Error, TEXT("UseCard: Card is null")); return; }

	// 获取目标
	ATKPlayerStateBase* TargetPS = UserPS;
	if (TargetPlayerIndex >= 0)
	{
		ATKGameStateBase* TKGS = PC->GetWorld() ? PC->GetWorld()->GetGameState<ATKGameStateBase>() : nullptr;
		if (TKGS)
		{
			const TArray<TObjectPtr<APlayerState>>& Players = TKGS->PlayerArray;
			if (TargetPlayerIndex < Players.Num())
			{
				TargetPS = Cast<ATKPlayerStateBase>(Players[TargetPlayerIndex]);
			}
		}
	}

	if (!TargetPS) { UE_LOG(LogTKGame, Error, TEXT("UseCard: TargetPS is null")); return; }

	UE_LOG(LogTKGame, Log, TEXT(">>> UseCard: [%s] uses [%s] on [%s]"),
		*UserPS->GetPlayerName(), *Card->CardName.ToString(), *TargetPS->GetPlayerName());

	// 服务端执行
	if (PC->HasAuthority())
	{
		Card->Use(PC, TargetPS);
	}
	else
	{
		// 通过 RPC 代理
		PC->ServerUseCard(Card);
	}
}

void UTKCheatManager::RespondCard(int32 CardIndex)
{
	ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetPlayerController());
	if (!PC) return;

	ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PC->PlayerState);
	if (!TKPS) return;

	UTKCardZoneComponent* Zone = TKPS->GetCardZone();
	if (!Zone) return;

	TArray<UTKCardBase*> Hand = Zone->GetHandCards();
	if (CardIndex < 0 || CardIndex >= Hand.Num())
	{
		UE_LOG(LogTKGame, Error, TEXT("RespondCard: Invalid card index %d (hand size=%d)"), CardIndex, Hand.Num());
		return;
	}

	UTKCardBase* Card = Hand[CardIndex];

	UE_LOG(LogTKGame, Log, TEXT(">>> RespondCard: [%s] responds with [%s]"),
		*TKPS->GetPlayerName(), *Card->CardName.ToString());

	if (PC->HasAuthority())
	{
		ATKGameStateBase* TKGS = PC->GetWorld() ? PC->GetWorld()->GetGameState<ATKGameStateBase>() : nullptr;
		if (TKGS)
		{
			UTKResponseComponent* ResponseComp = TKGS->GetResponseComponent();
			if (ResponseComp)
			{
				Zone->RemoveCardFromHand(Card);
				ResponseComp->SubmitResponse(TKPS, Card);
			}
		}
	}
	else
	{
		PC->ServerSubmitResponse(Card);
	}
}

void UTKCheatManager::ShowResponseStatus()
{
	ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetPlayerController());
	if (!PC) return;

	ATKGameStateBase* TKGS = PC->GetWorld() ? PC->GetWorld()->GetGameState<ATKGameStateBase>() : nullptr;
	if (!TKGS) return;

	UTKResponseComponent* ResponseComp = TKGS->GetResponseComponent();
	if (!ResponseComp) return;

	const FResponseRequest& Req = ResponseComp->GetActiveRequest();
	UE_LOG(LogTKGame, Log, TEXT("========== Response Status =========="));
	UE_LOG(LogTKGame, Log, TEXT("  IsWaiting: %d"), ResponseComp->IsWaitingForResponse());
	UE_LOG(LogTKGame, Log, TEXT("  Result: %d"), (uint8)Req.Result);
	if (Req.IsPending())
	{
		UE_LOG(LogTKGame, Log, TEXT("  Type: %d | RequiredTag: %s"), (uint8)Req.Type, *Req.RequiredTag.GetTagName().ToString());
		UE_LOG(LogTKGame, Log, TEXT("  PrimaryTarget: %s"), Req.PrimaryTarget ? *Req.PrimaryTarget->GetPlayerName() : TEXT("None"));
		UE_LOG(LogTKGame, Log, TEXT("  ResponderQueue: %d | CurrentIndex: %d"), Req.ResponderQueue.Num(), Req.CurrentResponderIndex);
	}
	UE_LOG(LogTKGame, Log, TEXT("======================================"));
}

void UTKCheatManager::ShowPlayerStatus()
{
	ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetPlayerController());
	if (!PC) return;

	ATKGameStateBase* TKGS = PC->GetWorld() ? PC->GetWorld()->GetGameState<ATKGameStateBase>() : nullptr;
	if (!TKGS) return;

	UTKTurnComponentBase* TurnComp = TKGS->GetTurnComponent();

	UE_LOG(LogTKGame, Log, TEXT("========== All Players =========="));
	UE_LOG(LogTKGame, Log, TEXT("  CurrentTurn: %s (Phase=%d, Turn=%d)"),
		TurnComp && TurnComp->GetCurrentPlayer() ? *TurnComp->GetCurrentPlayer()->GetPlayerName() : TEXT("None"),
		(uint8)(TurnComp ? TurnComp->GetCurrentPhase() : ETKTurnPhase::Prepare),
		TurnComp ? TurnComp->GetCurrentTurnNumber() : 0);

	for (const TObjectPtr<APlayerState>& PS : TKGS->PlayerArray)
	{
		ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
		if (!TKPS) continue;
		UTKCardZoneComponent* Zone = TKPS->GetCardZone();
		UE_LOG(LogTKGame, Log, TEXT("  [%s] Alive=%d HP=%d/%d Identity=%d Hand=%d Seat=%d"),
			*TKPS->GetPlayerName(),
			TKPS->IsAlive() ? 1 : 0,
			TKPS->CurrentHealth,
			TKPS->MaxHealth,
			(uint8)TKPS->Identity,
			Zone ? Zone->GetHandCardCount() : 0,
			TKPS->SeatIndex);
	}
	UE_LOG(LogTKGame, Log, TEXT("=================================="));
}

// ===== HUD 命令 =====

void UTKCheatManager::ShowDebugHud()
{
	if (DebugHud)
	{
		DebugHud->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	APlayerController* PC = GetPlayerController();
	if (PC == nullptr) return;

	DebugHud = CreateWidget<UTKDebugHudWidget>(PC, UTKDebugHudWidget::StaticClass());
	if (DebugHud)
	{
		DebugHud->AddToViewport(9999);
		UE_LOG(LogTKGame, Log, TEXT("Debug HUD shown (CheatManager)"));
	}
}

void UTKCheatManager::HideDebugHud()
{
	if (DebugHud && DebugHud->IsInViewport())
	{
		DebugHud->RemoveFromParent();
		DebugHud = nullptr;
		UE_LOG(LogTKGame, Log, TEXT("Debug HUD hidden (CheatManager)"));
	}
}

void UTKCheatManager::ToggleDebugHud()
{
	if (DebugHud && DebugHud->IsInViewport())
	{
		HideDebugHud();
	}
	else
	{
		ShowDebugHud();
	}
}

void UTKCheatManager::EnsureDebugHud()
{
	if (DebugHud == nullptr)
	{
		ShowDebugHud();
	}
}

void UTKCheatManager::DestroyDebugHud()
{
	HideDebugHud();
}
