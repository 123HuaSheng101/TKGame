// Fill out your copyright notice in the Description page of Project Settings.

#include "TKPlayerControllerBase.h"
#include "TKEventComponentBase.h"
#include "TKGameModeBase.h"
#include "TKGameStateBase.h"
#include "TKTurnComponentBase.h"
#include "TKCheatManager.h"

ATKPlayerControllerBase::ATKPlayerControllerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EventComponent = CreateDefaultSubobject<UTKEventComponentBase>("EventComponent");
	CheatClass = UTKCheatManager::StaticClass();
}

bool ATKPlayerControllerBase::ServerStartGame_Validate() { return true; }
void ATKPlayerControllerBase::ServerStartGame_Implementation()
{
	ATKGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ATKGameModeBase>();
	if (GameMode)
	{
		GameMode->StartIdentityGame();
	}
}

bool ATKPlayerControllerBase::ServerAdvancePhase_Validate() { return true; }
void ATKPlayerControllerBase::ServerAdvancePhase_Implementation()
{
	ATKGameStateBase* TKGameState = GetWorld() ? GetWorld()->GetGameState<ATKGameStateBase>() : nullptr;
	if (TKGameState)
	{
		UTKTurnComponentBase* TurnComp = TKGameState->GetTurnComponent();
		if (TurnComp && TurnComp->GetCurrentPlayer())
		{
			TurnComp->AdvancePhase();
		}
	}
}
