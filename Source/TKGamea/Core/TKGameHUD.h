// TKGameHUD.h — MVC Controller 层

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TKGameTypes.h"
#include "Ui/TKGameUIData.h"
#include "TKGameHUD.generated.h"

class UTKGameHudWidget;
class UTKCardBase;

/**
 * 游戏 HUD — MVC Controller
 *
 * 职责：
 *   1. 每帧从 Model（GameState/PlayerState/CardZone/DeckComponent）采集数据
 *   2. Push 到 View（UTKGameHudWidget）刷新显示
 *   3. 接收 View 的用户交互委托 → 调用 Server RPC
 *   4. 管理响应面板生命周期
 */
UCLASS()
class TKGAMEA_API ATKGameHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	// ---- View 驱动 ----

	/** 主 HUD Widget（蓝图只读） */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTKGameHudWidget> MainHud;

	/** 弹出响应面板 */
	void ShowResponsePanel(const FGameplayTag& RequiredTag);

	/** 关闭响应面板 */
	void HideResponsePanel();

	// ---- View → Controller 回调（由 Widget 通过蓝图调用） ----

	/** 手牌点击：使用第 CardIndex 张牌 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OnCardClicked(int32 CardIndex);

	/** 响应牌点击：提交第 CardIndex 张牌作为响应 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OnResponseCardClicked(int32 CardIndex);

	/** 跳过响应 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OnPassResponse();

	/** 结束回合 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void OnEndTurnClicked();

	// ---- 数据查询（供 Widget 调用） ----

	/** 获取我的手牌列表 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD")
	TArray<UTKCardBase*> GetMyHandCards() const;

	/** 获取游戏全局数据 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD")
	FTKGameUIData GetGameUIData() const;

	/** 获取所有玩家状态 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HUD")
	TArray<FTKPlayerUIData> GetAllPlayerUIData() const;

protected:
	/** 创建 UMG Widget */
	void CreateMainHud();

	/** 全量刷新 View */
	void RefreshAll();

private:
	/** Widget 类（Blueprint 覆写） */
	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<UTKGameHudWidget> HUDWidgetClass;

	/** 响应计时器 */
	FTimerHandle ResponseTimer;
	float ResponseTimeRemaining = 0.0f;
};
