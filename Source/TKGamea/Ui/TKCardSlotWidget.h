// TKCardSlotWidget.h — 手牌卡牌 View 接口

#pragma once

#include "CoreMinimal.h"
#include "TKUserWidgetBase.h"
#include "TKCardSlotWidget.generated.h"

class UTKCardBase;

/** 卡牌点击委托 */
DECLARE_DELEGATE_OneParam(FOnCardClicked, int32 /* CardIndex */);

/**
 * 手牌卡牌 Widget — 接口层
 *
 * 蓝图子类负责视觉样式。
 * 点击回调通过 OnCardClicked 委托回传 ATKGameHUD。
 */
UCLASS()
class TKGAMEA_API UTKCardSlotWidget : public UTKUserWidgetBase
{
	GENERATED_BODY()

public:
	/** 设置卡牌数据 */
	UFUNCTION(BlueprintCallable, Category = "CardSlot")
	void SetCardData(UTKCardBase* Card, int32 Index);

	/** 获取卡牌对象 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CardSlot")
	UTKCardBase* GetCard() const { return CardData; }

	/** 获取卡牌索引 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CardSlot")
	int32 GetCardIndex() const { return CardIndex; }

	/** 点击委托 */
	FOnCardClicked OnClicked;

	// ===== 蓝图事件 =====

	/** 蓝图事件：卡牌数据更新时 */
	UFUNCTION(BlueprintImplementableEvent, Category = "CardSlot")
	void OnCardDataUpdated(UTKCardBase* Card);

	/** 蓝图事件：卡牌被点击 */
	UFUNCTION(BlueprintImplementableEvent, Category = "CardSlot")
	void OnCardClickedBP();

protected:
	/** 卡牌索引 */
	int32 CardIndex = -1;

	/** 卡牌数据 */
	UPROPERTY(BlueprintReadOnly, Category = "CardSlot")
	TObjectPtr<UTKCardBase> CardData;
};
