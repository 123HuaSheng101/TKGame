#include "TKGamea.h"
// Fill out your copyright notice in the Description page of Project Settings.


#include "TKEventComponentBase.h"
#include "TKPlayerControllerBase.h"

// Sets default values for this component's properties
UTKEventComponentBase::UTKEventComponentBase(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UTKEventComponentBase::PostEvent(FEventContext& Context)
{
}

UTKEventComponentBase* UTKEventComponentBase::Get(const ATKPlayerControllerBase* PlayerController)
{
	if (PlayerController && IsValid(PlayerController))
	{
		UTKEventComponentBase* EventComponent = Cast<UTKEventComponentBase>(PlayerController->GetComponentByClass(UTKEventComponentBase::StaticClass()));
		if (EventComponent == nullptr)
		{
			UE_LOG(LogTKGame, Warning, TEXT("UTKEventComponentBase::Get - EventComponent not found on PlayerController"));
			return nullptr;
		}
		return EventComponent;
	}
	return nullptr;
}
