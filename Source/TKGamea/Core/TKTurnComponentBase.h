// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TKTurnComponentBase.generated.h"

class ATKGameStateBase;
class UTKTurnComponentBase;
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TKGAMEA_API UTKTurnComponentBase : public UActorComponent
{
	GENERATED_BODY()
public:	
	UTKTurnComponentBase(const FObjectInitializer& Initializer);
	static UTKTurnComponentBase* Get(const ATKGameStateBase* GameState); 
};
