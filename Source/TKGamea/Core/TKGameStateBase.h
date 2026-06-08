// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TKGameStateBase.generated.h"

class UTKTurnComponentBase;
/**
 * 
 */
UCLASS()
class TKGAMEA_API ATKGameStateBase : public AGameState
{
	GENERATED_BODY()
	ATKGameStateBase(const FObjectInitializer& Initializer);
private:
	UPROPERTY()
	TObjectPtr<UTKTurnComponentBase> TurnComponent;
};
