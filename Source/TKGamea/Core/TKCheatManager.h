// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "TKCheatManager.generated.h"

class UTKDebugHudWidget;

/**
 * 调试作弊管理器
 * 所有控制台 Exec 调试命令集中在此处，替代在 PlayerController 上声明 Exec
 * UE 自动将控制台输入路由到 CheatManager 的 UFUNCTION(Exec) 方法
 */
UCLASS()
class TKGAMEA_API UTKCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UTKCheatManager();

	// ---- 调试命令 ----

	/** 启动身份局（4人默认配置，供测试使用） */
	UFUNCTION(Exec)
	void StartDebugGame();

	/** 显示调试 HUD 覆盖层 */
	UFUNCTION(Exec)
	void ShowDebugHud();

	/** 隐藏调试 HUD 覆盖层 */
	UFUNCTION(Exec)
	void HideDebugHud();

	/** 切换调试 HUD 显示/隐藏 */
	UFUNCTION(Exec)
	void ToggleDebugHud();

	/** 推进当前玩家的回合到下一阶段 */
	UFUNCTION(Exec)
	void AdvancePhase();

	// ---- 卡牌测试命令 ----

	/** 查看当前玩家手牌列表 */
	UFUNCTION(Exec)
	void ShowHand();

	/** 对指定玩家使用手牌中的第N张卡（从0开始） */
	UFUNCTION(Exec)
	void UseCard(int32 CardIndex, int32 TargetPlayerIndex = -1);

	/** 响应当前请求：使用手牌中第N张卡作为响应牌 */
	UFUNCTION(Exec)
	void RespondCard(int32 CardIndex);

	/** 查看当前响应组件状态 */
	UFUNCTION(Exec)
	void ShowResponseStatus();

	/** 打印当前所有玩家的状态信息 */
	UFUNCTION(Exec)
	void ShowPlayerStatus();

	/** 装备手牌中的第N张卡 */
	UFUNCTION(Exec)
	void EquipCard(int32 CardIndex);

	/** 查看当前玩家装备区 */
	UFUNCTION(Exec)
	void ShowEquip();

protected:
	/** 确保调试 HUD 已创建并显示 */
	void EnsureDebugHud();

	/** 销毁调试 HUD */
	void DestroyDebugHud();

	/** 调试 HUD 实例引用 */
	UPROPERTY()
	TObjectPtr<UTKDebugHudWidget> DebugHud;
};
