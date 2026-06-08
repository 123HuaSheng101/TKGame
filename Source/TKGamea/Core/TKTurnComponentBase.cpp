// Fill out your copyright notice in the Description page of Project Settings.


#include "TKTurnComponentBase.h"

// Sets default values for this component's properties
UTKTurnComponentBase::UTKTurnComponentBase(const FObjectInitializer& Initializer)
: Super(Initializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}
