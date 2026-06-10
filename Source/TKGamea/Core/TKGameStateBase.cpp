// Fill out your copyright notice in the Description page of Project Settings.


#include "TKGameStateBase.h"
#include "TKTurnComponentBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

ATKGameStateBase::ATKGameStateBase(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	TurnComponent = CreateDefaultSubobject<UTKTurnComponentBase>("TurnComponent");
	GameResult = FTKGameResultData();
}

void ATKGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATKGameStateBase, GameResult);
}

void ATKGameStateBase::OnRep_GameResult()
{
	if (GameResult.WinnerFaction != ETKGameResult::None)
	{
		UE_LOG(LogTemp, Log, TEXT("Game Over! Winner: %d"), (uint8)GameResult.WinnerFaction);
		for (const auto& Player : GameResult.WinningPlayers)
		{
			if (Player)
			{
				UE_LOG(LogTemp, Log, TEXT("  Winning Player: %s"), *Player->GetPlayerName());
			}
		}
	}
}
