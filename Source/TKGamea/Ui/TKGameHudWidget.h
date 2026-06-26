// TKGameHudWidget.h — 主 HUD View 接口

#pragma once

#include "CoreMinimal.h"
#include "TKUserWidgetBase.h"
#include "TKGameUIData.h"
#include "TKGameHudWidget.generated.h"

class ATKGameHUD;

/**
 * 主 HUD Widget — MVC View 层
 *
 * 蓝图子类负责布局和视觉。
 * C++ 侧只定义数据接口，由 ATKGameHUD 推送。
 */
UCLASS()
class TKGAMEA_API UTKGameHudWidget : public UTKUserWidgetBase
{
	GENERATED_BODY()

public:
	/** 设置所属 HUD */
	void SetOwningHUD(ATKGameHUD* HUD) { OwningHUD = HUD; }

	/** 获取所属 HUD */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD")
	ATKGameHUD* GetOwningHUD() const { return OwningHUD; }

	// ===== 数据刷新（C++ 调用，蓝图可在 NativeTick 中覆写） =====

	/** 全量刷新（由 ATKGameHUD::NativeTick 调用） */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void RefreshDisplay(const FTKGameUIData& GameData, const TArray<FTKPlayerUIData>& PlayerData);

	/** 刷新手牌面板 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void RefreshHandCards();

	// ===== 响应面板 =====

	/** 显示响应面板 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowResponsePanel();

	/** 隐藏响应面板 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideResponsePanel();

	/** 更新响应面板数据 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetResponseUIData(const FTKResponseUIData& Data);

	// ===== 蓝图覆写事件 =====

	/** 蓝图事件：当 RefreshDisplay 被调用 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnGameDataUpdated(const FTKGameUIData& GameData, const TArray<FTKPlayerUIData>& PlayerData);

	/** 蓝图事件：当手牌刷新 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnHandCardsUpdated(const TArray<UTKCardBase*>& Cards);

	/** 蓝图事件：响应面板显示 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnResponseShown(const FTKResponseUIData& Data);

	/** 蓝图事件：响应面板隐藏 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnResponseHidden();

protected:
	/** 所属 HUD Controller */
	UPROPERTY()
	TObjectPtr<ATKGameHUD> OwningHUD;
};
