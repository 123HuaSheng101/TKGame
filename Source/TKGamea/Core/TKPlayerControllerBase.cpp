// Fill out your copyright notice in the Description page of Project Settings.

#include "TKPlayerControllerBase.h"
#include "TKGamea.h"
#include "TKEventComponentBase.h"
#include "TKGameModeBase.h"
#include "TKGameHUD.h"
#include "TKGameStateBase.h"
#include "TKTurnComponentBase.h"
#include "TKResponseComponent.h"
#include "TKPlayerStateBase.h"
#include "TKCheatManager.h"
#include "Game/TKCardBase.h"
#include "Cards/TKCardZoneComponent.h"

ATKPlayerControllerBase::ATKPlayerControllerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EventComponent = CreateDefaultSubobject<UTKEventComponentBase>("EventComponent");
	CheatClass = UTKCheatManager::StaticClass();
}

bool ATKPlayerControllerBase::ServerStartGame_Validate() { return true; }
void ATKPlayerControllerBase::ServerStartGame_Implementation()
{
	ATKGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ATKGameModeBase>();
	if (GameMode)
	{
		GameMode->StartIdentityGame();
	}
}

bool ATKPlayerControllerBase::ServerAdvancePhase_Validate() { return true; }
void ATKPlayerControllerBase::ServerAdvancePhase_Implementation()
{
	ATKGameStateBase* TKGameState = GetWorld() ? GetWorld()->GetGameState<ATKGameStateBase>() : nullptr;
	if (TKGameState)
	{
		UTKTurnComponentBase* TurnComp = TKGameState->GetTurnComponent();
		if (TurnComp && TurnComp->GetCurrentPlayer())
		{
			TurnComp->AdvancePhase();
		}
	}
}

// ===== 卡牌使用 RPC =====

bool ATKPlayerControllerBase::ServerUseCard_Validate(UTKCardBase* Card)
{
	return Card != nullptr;
}

void ATKPlayerControllerBase::ServerUseCard_Implementation(UTKCardBase* Card)
{
	if (!Card) return;

	ATKPlayerStateBase* UserPS = Cast<ATKPlayerStateBase>(PlayerState);
	if (!UserPS) return;

	// 默认目标为自己
	Card->Use(this, UserPS);
}

// ===== 响应系统 RPC =====

bool ATKPlayerControllerBase::ServerSubmitResponse_Validate(UTKCardBase* ResponseCard)
{
	return ResponseCard != nullptr;
}

void ATKPlayerControllerBase::ServerSubmitResponse_Implementation(UTKCardBase* ResponseCard)
{
	ATKGameStateBase* TKGS = GetWorld() ? GetWorld()->GetGameState<ATKGameStateBase>() : nullptr;
	if (!TKGS || !ResponseCard) return;

	UTKResponseComponent* ResponseComp = TKGS->GetResponseComponent();
	if (!ResponseComp) return;

	ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PlayerState);
	if (!TKPS) return;

	// 从手牌区移除响应牌
	UTKCardZoneComponent* Zone = TKPS->GetCardZone();
	if (Zone)
	{
		Zone->RemoveCardFromHand(ResponseCard);
	}

	// 提交响应
	bool bSuccess = ResponseComp->SubmitResponse(TKPS, ResponseCard);

	UE_LOG(LogTKGame, Log, TEXT("ServerSubmitResponse: [%s] submitted card [%s], success=%d"),
		*TKPS->GetPlayerName(), *ResponseCard->CardDefId.ToString(), bSuccess);
}

void ATKPlayerControllerBase::ClientPromptResponse_Implementation(const FGameplayTag& RequiredTag, const FText& PromptText)
{
	UE_LOG(LogTKGame, Log, TEXT("ClientPromptResponse: Need [%s], prompt: [%s]"),
		*RequiredTag.GetTagName().ToString(), *PromptText.ToString());

	// 通知 HUD 弹出响应面板
	ATKGameHUD* GameHUD = Cast<ATKGameHUD>(GetHUD());
	if (GameHUD)
	{
		GameHUD->ShowResponsePanel(RequiredTag);
	}
}
