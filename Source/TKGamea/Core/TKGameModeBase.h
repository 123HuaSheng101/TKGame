// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TKGameTypes.h"
#include "Cards/TKDeckComponent.h"
#include "Rules/TKIdentityRuleComponent.h"
#include "TKGameModeBase.generated.h"

class ATKPlayerStateBase;


/**
 * 游戏模式基类
 * 负责管理游戏生命周期、身份模式开局、规则与牌堆组件的持有
 */
UCLASS()
class TKGAMEA_API ATKGameModeBase : public AGameMode
{
	GENERATED_BODY()
public:
	ATKGameModeBase(const FObjectInitializer& Initializer);

	// ---- 模式配置 ----

	/** 当前游戏模式的配置参数（人数、模式等） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	FTKModeConfig ModeConfig;

	// ---- 身份模式入口 ----

	/**
	 * 启动身份模式游戏
	 * 流程：人数校验 → 初始化牌堆 → 分配身份 → 发起始手牌 → 主公先手
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartIdentityGame();

	// ---- 工具 ----

	/**
	 * 结束游戏，记录胜方并通知 GameState 复制结果
	 * @param Result  胜者阵营
	 * @param Winners 胜利玩家列表
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void EndGame(ETKGameResult Result, const TArray<APlayerState*>& Winners);

protected:
	/** 匹配状态变更回调 */
	virtual void OnMatchStateSet() override;

	/** 调用规则组件执行身份分配 */
	void AssignIdentities();

	/** 给所有存活玩家发初始手牌 */
	void DealInitialCards();

	/** 找到主公并设置其第一个回合 */
	void SetLordFirstTurn();

	/** 牌堆组件（管理牌堆和弃牌堆） */
	UPROPERTY()
	TObjectPtr<UTKDeckComponent> DeckComponent;

	/** 身份规则组件（身份分配、死亡、胜负判定） */
	UPROPERTY()
	TObjectPtr<UTKIdentityRuleComponent> IdentityRule;
};
