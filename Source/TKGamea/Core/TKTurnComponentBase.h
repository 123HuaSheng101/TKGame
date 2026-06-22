// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TKGameTypes.h"
#include "TKTurnComponentBase.generated.h"

class ATKGameStateBase;
class APlayerState;

/**
 * 回合阶段状态机组件
 * 挂载在 GameState 上，管理 Prepare → Judge → Draw → Play → Discard → Finish 六阶段推进
 * 仅服务器有权推进阶段，客户端通过复制接收当前状态
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TKGAMEA_API UTKTurnComponentBase : public UActorComponent
{
	GENERATED_BODY()
public:
	UTKTurnComponentBase(const FObjectInitializer& Initializer);

	/**
	 * 从 GameState 上获取回合组件的静态工具方法
	 * @param GameState 目标游戏状态
	 * @return 找到的回合组件，找不到返回 nullptr
	 */
	static UTKTurnComponentBase* Get(const ATKGameStateBase* GameState);

	// ---- 阶段状态机接口 ----

	/**
	 * 开始新回合
	 * @param Player 当前回合的玩家
	 */
	UFUNCTION(BlueprintCallable, Category = "Turn")
	void StartTurn(APlayerState* Player);

	/**
	 * 进入指定阶段
	 * @param NewPhase 要进入的阶段
	 */
	UFUNCTION(BlueprintCallable, Category = "Turn")
	void EnterPhase(ETKTurnPhase NewPhase);

	/** 推进到下一个阶段 */
	UFUNCTION(BlueprintCallable, Category = "Turn")
	void AdvancePhase();

	/** 结束当前回合 */
	UFUNCTION(BlueprintCallable, Category = "Turn")
	void EndTurn();

	// ---- 查询 ----

	/** 获取当前回合的玩家 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	APlayerState* GetCurrentPlayer() const { return CurrentPlayer; }

	/** 获取当前阶段 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	ETKTurnPhase GetCurrentPhase() const { return CurrentPhase; }

	/** 获取当前回合数（从 1 开始） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	int32 GetCurrentTurnNumber() const { return CurrentTurnNumber; }

	/** 当前阶段中"杀"的使用次数 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	int32 GetSlashUsedCount() const { return SlashUsedThisPhase; }

	/** 每回合默认杀次数上限（无装备/技能时=1） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	int32 GetMaxSlashCount() const { return 1; }

	/** 消耗一次杀次数 */
	void ConsumeSlash() { SlashUsedThisPhase++; }

	/** 是否还有杀使用次数 */
	bool CanUseSlash() const { return SlashUsedThisPhase < GetMaxSlashCount(); }

	// ---- 酒状态 ----

	/** 当前回合玩家是否有酒状态（下张杀伤害+1） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	bool IsWined() const { return bWined; }

	/** 设置酒状态 */
	void SetWined(bool bVal) { bWined = bVal; }

protected:
	/**
	 * 摸牌阶段默认行为
	 * 子类可重写自定义摸牌逻辑
	 */
	virtual void OnDrawPhase();

	/**
	 * 弃牌阶段默认行为
	 * 子类可重写自定义弃牌逻辑
	 */
	virtual void OnDiscardPhase();

	/**
	 * 查找当前玩家的下一个存活玩家（循环座位，跳过死亡玩家）
	 * @return 下一个存活玩家，如果只剩自己则返回 nullptr
	 */
	APlayerState* FindNextAlivePlayer() const;

	// ---- 复制 ----
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 当前回合的玩家 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Turn")
	TObjectPtr<APlayerState> CurrentPlayer;

	/** 当前阶段 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Turn")
	ETKTurnPhase CurrentPhase = ETKTurnPhase::Prepare;

	/** 当前回合编号 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Turn")
	int32 CurrentTurnNumber = 0;

	/** 当前阶段中"杀"的使用计数（仅服务器，不同步） */
	UPROPERTY(BlueprintReadOnly, Category = "Turn")
	int32 SlashUsedThisPhase = 0;

	/** 酒状态（仅服务器，不同步） */
	UPROPERTY(BlueprintReadOnly, Category = "Turn")
	bool bWined = false;
};
