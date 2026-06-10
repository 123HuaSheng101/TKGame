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

protected:
	/** 确保调试 HUD 已创建并显示 */
	void EnsureDebugHud();

	/** 销毁调试 HUD */
	void DestroyDebugHud();

	/** 调试 HUD 实例引用 */
	UPROPERTY()
	TObjectPtr<UTKDebugHudWidget> DebugHud;
};
