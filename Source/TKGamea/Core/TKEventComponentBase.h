// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TKEventComponentBase.generated.h"

struct FEventContext;
class ATKPlayerControllerBase;

/**
 * 事件组件
 * 挂载在 PlayerController 上，用于事件的触发和分发
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TKGAMEA_API UTKEventComponentBase : public UActorComponent
{
	GENERATED_BODY()
public:
	UTKEventComponentBase(const FObjectInitializer& Initializer);

	/**
	 * 发布事件到事件系统
	 * @param Context 事件上下文（包含事件 Tag、发起者、目标、参数）
	 */
	UFUNCTION(BlueprintCallable)
	void PostEvent(FEventContext& Context);

	/**
	 * 从 PlayerController 上获取事件组件的静态工具方法
	 * @param PlayerController 目标玩家控制器
	 * @return 找到的事件组件，找不到返回 nullptr（不会断言崩溃）
	 */
	static UTKEventComponentBase* Get(const ATKPlayerControllerBase* PlayerController);
};
