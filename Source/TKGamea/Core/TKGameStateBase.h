// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TKGameTypes.h"
#include "TKGameStateBase.generated.h"

class UTKTurnComponentBase;

/**
 * 游戏状态基类
 * 持有回合组件引用，管理游戏结果等全局复制状态
 */
UCLASS()
class TKGAMEA_API ATKGameStateBase : public AGameState
{
	GENERATED_BODY()
public:
	ATKGameStateBase(const FObjectInitializer& Initializer);

	/** 获取回合组件 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	UTKTurnComponentBase* GetTurnComponent() const { return TurnComponent; }

	/** 游戏结果（复制到所有客户端，触发 OnRep 响应） */
	UPROPERTY(ReplicatedUsing = OnRep_GameResult, BlueprintReadOnly, Category = "Game")
	FTKGameResultData GameResult;

	/** 游戏结果复制回调：日志输出 */
	UFUNCTION()
	void OnRep_GameResult();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	friend class UTKTurnComponentBase;
	/** 回合阶段状态机组件 */
	UPROPERTY()
	TObjectPtr<UTKTurnComponentBase> TurnComponent;
};
