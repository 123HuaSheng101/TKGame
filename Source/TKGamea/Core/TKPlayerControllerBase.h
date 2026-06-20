// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TKPlayerControllerBase.generated.h"

class UTKEventComponentBase;
class UTKCardBase;
class ATKPlayerStateBase;

/**
 * 玩家控制器基类
 * 自动创建事件组件；设置自定义作弊管理器
 */
UCLASS()
class TKGAMEA_API ATKPlayerControllerBase : public APlayerController
{
	GENERATED_BODY()
public:
	ATKPlayerControllerBase(const FObjectInitializer& ObjectInitializer);

	/** 获取事件组件 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UTKEventComponentBase* GetEventComponent() const { return EventComponent; }

	// ---- 服务端 RPC ----

	/** 服务端 RPC：启动身份局（供客户端 CheatManager 调用） */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartGame();

	/** 服务端 RPC：推进阶段（供客户端 CheatManager 调用） */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAdvancePhase();

	/** 服务端 RPC：使用手牌（供客户端 CheatManager 调用）*/
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUseCard(UTKCardBase* Card);

	/** 服务端 RPC：提交响应牌（客户端在响应窗口中选择打出的牌） */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSubmitResponse(UTKCardBase* ResponseCard);

	// ---- 客户端 RPC ----

	/** 客户端 RPC：通知客户端需要响应 */
	UFUNCTION(Client, Reliable)
	void ClientPromptResponse(const FGameplayTag& RequiredTag, const FText& PromptText);

protected:
	/** 事件组件（自动创建） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UTKEventComponentBase> EventComponent;
};
