// Fill out your copyright notice in the Description page of Project Settings.


#include "TKGameStateBase.h"
#include "TKTurnComponentBase.h"

ATKGameStateBase::ATKGameStateBase(const FObjectInitializer& Initializer)
: Super(Initializer)
{
	TurnComponent = CreateDefaultSubobject<UTKTurnComponentBase>("TurnComponent");
}
