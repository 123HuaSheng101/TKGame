// Fill out your copyright notice in the Description page of Project Settings.


#include "TKTurnComponentBase.h"
#include "TKGameStateBase.h"

// Sets default values for this component's properties
UTKTurnComponentBase::UTKTurnComponentBase(const FObjectInitializer& Initializer)
: Super(Initializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UTKTurnComponentBase* UTKTurnComponentBase::Get(const ATKGameStateBase* GameState)
{
	if (GameState == nullptr)
		return nullptr;
	if (!IsValid(GameState))
		return nullptr;
	return GameState->TurnComponent.Get();
}
