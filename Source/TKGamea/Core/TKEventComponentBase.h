// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TKEventComponentBase.generated.h"

struct  FEventContext;
class	ATKPlayerControllerBase;
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TKGAMEA_API UTKEventComponentBase : public UActorComponent
{
	GENERATED_BODY()
public:	
	UTKEventComponentBase(const FObjectInitializer& Initializer);
	UFUNCTION(BlueprintCallable)
	void PostEvent(FEventContext& Context);
	static UTKEventComponentBase* Get(const ATKPlayerControllerBase* PlayerController);
};
