// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TKGameTypes.h"
#include "TKTurnComponentBase.generated.h"

class ATKGameStateBase;
class APlayerState;
class ATKPlayerStateBase;
class UTKCard_DelayedTrick;

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

	/** 强制停止回合状态机（游戏结束时调用） */
	UFUNCTION(BlueprintCallable, Category = "Turn")
	void Stop();

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

	// ---- 跳过阶段标记（延时锦囊用） ----

	/** 是否跳过摸牌阶段（兵粮寸断） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	bool ShouldSkipDrawPhase() const { return bSkipDrawPhase; }

	/** 设置跳过摸牌阶段标记 */
	void SetSkipDrawPhase(bool bSkip) { bSkipDrawPhase = bSkip; }

	/** 是否跳过出牌阶段（乐不思蜀） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	bool ShouldSkipPlayPhase() const { return bSkipPlayPhase; }

	/** 设置跳过出牌阶段标记 */
	void SetSkipPlayPhase(bool bSkip) { bSkipPlayPhase = bSkip; }

	// ---- 装备 / 距离系统 ----

	/**
	 * 计算两个存活玩家之间的座位距离（不含装备修正）
	 * @return 1 ~ N-1 的圈上最短距离
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	int32 GetRawDistance(ATKPlayerStateBase* From, ATKPlayerStateBase* To) const;

	/**
	 * 获取某玩家的武器攻击范围
	 * 默认=1（无武器），装备武器后=武器所给范围值
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	int32 GetWeaponRange(ATKPlayerStateBase* Player) const;

	/**
	 * 检查某玩家是否能攻击到目标（武器范围 + -1马 >= 距离 + 目标+1马）
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	bool CanReachTarget(ATKPlayerStateBase* Attacker, ATKPlayerStateBase* Defender) const;

	/** 检查玩家是否有八卦阵（防具） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	bool HasBaguaArmor(ATKPlayerStateBase* Player) const;

protected:
	/**
	 * 判定阶段默认行为
	 * 遍历当前玩家判定区的延时锦囊并逐一结算
	 */
	virtual void OnJudgePhase();

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

	/** 跳过摸牌阶段标记（兵粮寸断，每回合开始重置） */
	bool bSkipDrawPhase = false;

	/** 跳过出牌阶段标记（乐不思蜀，每回合开始重置） */
	bool bSkipPlayPhase = false;
};
