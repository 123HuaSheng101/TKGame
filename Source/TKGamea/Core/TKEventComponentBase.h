// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "TKEventComponentBase.generated.h"

struct FEventContext;
class ATKPlayerControllerBase;

/** 事件处理回调：返回 true 表示事件已被消费 */
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnGameEvent, const FEventContext&);

/**
 * 事件组件
 * 挂载在每个 PlayerController 上，用于事件发布和本地处理
 *
 * 使用方式：
 *   - 订阅：EventComp->RegisterHandler(Tag, Delegate)
 *   - 发布：EventComp->PostEvent(Context) → 通知 GameState 广播
 *   - M3 阶段扩展为全服广播
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TKGAMEA_API UTKEventComponentBase : public UActorComponent
{
	GENERATED_BODY()
public:
	UTKEventComponentBase(const FObjectInitializer& Initializer);

	/** 发布事件（服务器→广播到 GameState 事件管理器） */
	UFUNCTION(BlueprintCallable)
	void PostEvent(FEventContext& Context);

	/** 本地处理事件（客户端回调） */
	void HandleLocalEvent(const FEventContext& Context);

	/** 注册事件处理器 */
	void RegisterHandler(const FGameplayTag& EventTag, FOnGameEvent Handler);

	/** 从 PlayerController 上获取事件组件的静态工具方法 */
	static UTKEventComponentBase* Get(const ATKPlayerControllerBase* PlayerController);

private:
	/** 事件处理器映射 */
	TMap<FGameplayTag, TArray<FOnGameEvent>> Handlers;
};
