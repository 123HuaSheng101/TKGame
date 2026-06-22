#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TKGameTypes.h"
#include "TKDebugHudWidget.generated.h"

class UTextBlock;
class UCanvasPanel;

/**
 * 调试 HUD 覆盖层
 * 屏幕左上角显示游戏关键状态信息：回合、阶段、玩家状态表、牌堆信息、游戏结果
 * 每帧通过 NativeTick 刷新显示
 */
UCLASS()
class TKGAMEA_API UTKDebugHudWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UTKDebugHudWidget(const FObjectInitializer& Initializer);

	/** 每帧刷新显示 */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 绑定回合信息文本块 */
	void SetTurnInfoText(UTextBlock* Text) { TurnInfoText = Text; }

	/** 绑定阶段信息文本块 */
	void SetPhaseText(UTextBlock* Text) { PhaseText = Text; }

	/** 绑定玩家状态表文本块（表格格式） */
	void SetPlayerStatusText(UTextBlock* Text) { PlayerStatusText = Text; }

	/** 绑定牌堆信息文本块 */
	void SetDeckInfoText(UTextBlock* Text) { DeckInfoText = Text; }

	/** 绑定游戏结果文本块 */
	void SetGameResultText(UTextBlock* Text) { GameResultText = Text; }

	/** 绑定手牌信息文本块 */
	void SetHandCardsText(UTextBlock* Text) { HandCardsText = Text; }

protected:
	/** 回合信息（回合号、当前玩家） */
	UPROPERTY()
	TObjectPtr<UTextBlock> TurnInfoText;

	/** 阶段信息 */
	UPROPERTY()
	TObjectPtr<UTextBlock> PhaseText;

	/** 玩家状态表（座位、身份、存活、HP、手牌数） */
	UPROPERTY()
	TObjectPtr<UTextBlock> PlayerStatusText;

	/** 牌堆信息 */
	UPROPERTY()
	TObjectPtr<UTextBlock> DeckInfoText;

	/** 游戏结果 */
	UPROPERTY()
	TObjectPtr<UTextBlock> GameResultText;

	/** 手牌信息 */
	UPROPERTY()
	TObjectPtr<UTextBlock> HandCardsText;

	/** 刷新所有显示数据 */
	void RefreshDisplay();

	/** 将阶段枚举转为可读字符串 */
	FString GetPhaseName(ETKTurnPhase Phase) const;

	/** 将身份枚举转为可读字符串 */
	FString GetIdentityName(ETKIdentity Identity) const;
};
