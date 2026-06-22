// Fill out your copyright notice in the Description page of Project Settings.


#include "TKEventComponentBase.h"
#include "TKGamea.h"
#include "TKPlayerControllerBase.h"
#include "TKGameTypes.h"

UTKEventComponentBase::UTKEventComponentBase(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UTKEventComponentBase::PostEvent(FEventContext& Context)
{
	UE_LOG(LogTKGame, Log, TEXT("Event: [%s] initiator=[%s] targets=%d tags=%d"),
		*Context.EventTag.GetTagName().ToString(),
		Context.EventInitiator ? *Context.EventInitiator->GetName() : TEXT("None"),
		Context.Targets.Num(),
		Context.Params_g.Num());

	// 本地处理
	HandleLocalEvent(Context);

	// TODO M3: 通过 GameState 广播给所有客户端
}

void UTKEventComponentBase::HandleLocalEvent(const FEventContext& Context)
{
	TArray<FOnGameEvent>* HandlerList = Handlers.Find(Context.EventTag);
	if (HandlerList)
	{
		for (const FOnGameEvent& Handler : *HandlerList)
		{
			if (Handler.Execute(Context))
			{
				break; // 事件已被消费
			}
		}
	}
}

void UTKEventComponentBase::RegisterHandler(const FGameplayTag& EventTag, FOnGameEvent Handler)
{
	Handlers.FindOrAdd(EventTag).Add(Handler);
	UE_LOG(LogTKGame, Log, TEXT("EventComponent: Registered handler for tag [%s]"), *EventTag.GetTagName().ToString());
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
