// Fill out your copyright notice in the Description page of Project Settings.

#include "TKCheatManager.h"
#include "TKPlayerControllerBase.h"
#include "TKGameModeBase.h"
#include "TKGameStateBase.h"
#include "TKTurnComponentBase.h"
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
		UE_LOG(LogTemp, Log, TEXT("Debug HUD shown (CheatManager)"));
	}
}

void UTKCheatManager::HideDebugHud()
{
	if (DebugHud && DebugHud->IsInViewport())
	{
		DebugHud->RemoveFromParent();
		DebugHud = nullptr;
		UE_LOG(LogTemp, Log, TEXT("Debug HUD hidden (CheatManager)"));
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
