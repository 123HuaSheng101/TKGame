// TKGameHUD.cpp

#include "TKGameHUD.h"
#include "TKGamea.h"
#include "TKPlayerControllerBase.h"
#include "TKPlayerStateBase.h"
#include "TKGameStateBase.h"
#include "TKTurnComponentBase.h"
#include "TKGameModeBase.h"
#include "Game/TKCardBase.h"
#include "Cards/TKCardZoneComponent.h"
#include "Cards/TKDeckComponent.h"
#include "Ui/TKGameHudWidget.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"

void ATKGameHUD::BeginPlay()
{
	Super::BeginPlay();
	CreateMainHud();
}

void ATKGameHUD::EndPlay(const EEndPlayReason::Type Reason)
{
	if (MainHud && MainHud->IsInViewport())
	{
		MainHud->RemoveFromParent();
		MainHud = nullptr;
	}
	Super::EndPlay(Reason);
}

void ATKGameHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (MainHud)
	{
		MainHud->RefreshDisplay(GetGameUIData(), GetAllPlayerUIData());
	}

	// 更新响应倒计时
	if (ResponseTimeRemaining > 0.0f)
	{
		ResponseTimeRemaining = FMath::Max(0.0f, ResponseTimeRemaining - DeltaSeconds);
		if (MainHud)
		{
			FTKResponseUIData RespData;
			RespData.TimeRemaining = ResponseTimeRemaining;
			RespData.bIsWaiting = true;
			MainHud->SetResponseUIData(RespData);
		}
	}
}

// ===== View 驱动 =====

void ATKGameHUD::ShowResponsePanel(const FGameplayTag& RequiredTag)
{
	if (!MainHud) return;

	FTKResponseUIData Data;
	Data.RequiredTag = RequiredTag.GetTagName().ToString();
	Data.PromptText = FText::FromString(FString::Printf(TEXT("Need: %s"), *Data.RequiredTag));
	Data.TimeRemaining = 10.0f;
	Data.bIsWaiting = true;

	ResponseTimeRemaining = 10.0f;
	MainHud->SetResponseUIData(Data);
	MainHud->ShowResponsePanel();

	UE_LOG(LogTKGame, Log, TEXT("TKGameHUD: ShowResponsePanel tag=[%s]"), *Data.RequiredTag);
}

void ATKGameHUD::HideResponsePanel()
{
	if (MainHud)
	{
		MainHud->HideResponsePanel();
		ResponseTimeRemaining = 0.0f;
	}
}

// ===== View → Controller 回调 =====

void ATKGameHUD::OnCardClicked(int32 CardIndex)
{
	ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetOwningPlayerController());
	if (!PC) return;

	TArray<UTKCardBase*> Hand = GetMyHandCards();
	if (CardIndex < 0 || CardIndex >= Hand.Num()) return;

	UTKCardBase* Card = Hand[CardIndex];
	if (!Card) return;

	UE_LOG(LogTKGame, Log, TEXT("TKGameHUD: Card clicked [%d] = [%s]"), CardIndex, *Card->CardName.ToString());

	// 通过 Server RPC 使用卡牌
	PC->ServerUseCard(Card);
}

void ATKGameHUD::OnResponseCardClicked(int32 CardIndex)
{
	ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetOwningPlayerController());
	if (!PC) return;

	TArray<UTKCardBase*> Hand = GetMyHandCards();
	if (CardIndex < 0 || CardIndex >= Hand.Num()) return;

	UTKCardBase* Card = Hand[CardIndex];

	UE_LOG(LogTKGame, Log, TEXT("TKGameHUD: Response card clicked [%d] = [%s]"), CardIndex, *Card->CardName.ToString());

	// 通过 Server RPC 提交响应
	PC->ServerSubmitResponse(Card);
	HideResponsePanel();
}

void ATKGameHUD::OnPassResponse()
{
	UE_LOG(LogTKGame, Log, TEXT("TKGameHUD: Pass response clicked"));
	HideResponsePanel();
	// 服务端会在超时后自动结算
}

void ATKGameHUD::OnEndTurnClicked()
{
	ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetOwningPlayerController());
	if (!PC) return;

	UE_LOG(LogTKGame, Log, TEXT("TKGameHUD: End turn clicked"));
	PC->ServerAdvancePhase();
}

// ===== 数据查询 =====

TArray<UTKCardBase*> ATKGameHUD::GetMyHandCards() const
{
	TArray<UTKCardBase*> Result;
	ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetOwningPlayerController());
	if (!PC) return Result;

	ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PC->PlayerState);
	if (!TKPS) return Result;

	UTKCardZoneComponent* Zone = TKPS->GetCardZone();
	if (!Zone) return Result;

	return Zone->GetHandCards();
}

FTKGameUIData ATKGameHUD::GetGameUIData() const
{
	FTKGameUIData Data;
	ATKGameStateBase* TKGS = GetWorld() ? Cast<ATKGameStateBase>(GetWorld()->GetGameState()) : nullptr;
	if (!TKGS) return Data;

	UTKTurnComponentBase* TurnComp = TKGS->GetTurnComponent();
	if (TurnComp)
	{
		Data.TurnNumber = TurnComp->GetCurrentTurnNumber();
		APlayerState* CP = TurnComp->GetCurrentPlayer();
		Data.CurrentPlayerName = CP ? CP->GetPlayerName() : TEXT("None");

		static const TCHAR* PhaseNames[] = { TEXT("Prepare"), TEXT("Judge"), TEXT("Draw"), TEXT("Play"), TEXT("Discard"), TEXT("Finish") };
		uint8 Idx = (uint8)TurnComp->GetCurrentPhase();
		Data.PhaseName = (Idx < 6) ? PhaseNames[Idx] : TEXT("Unknown");
	}

	ATKGameModeBase* GM = Cast<ATKGameModeBase>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		UTKDeckComponent* Deck = GM->GetDeckComponent();
		if (Deck)
		{
			Data.DeckCount = Deck->GetDeckCount();
			Data.DiscardCount = Deck->GetDiscardPileCount();
		}
	}

	Data.WinnerFaction = (uint8)TKGS->GameResult.WinnerFaction;

	return Data;
}

TArray<FTKPlayerUIData> ATKGameHUD::GetAllPlayerUIData() const
{
	TArray<FTKPlayerUIData> Result;
	ATKGameStateBase* TKGS = GetWorld() ? Cast<ATKGameStateBase>(GetWorld()->GetGameState()) : nullptr;
	if (!TKGS) return Result;

	UTKTurnComponentBase* TurnComp = TKGS->GetTurnComponent();
	APlayerState* CurrentPlayer = TurnComp ? TurnComp->GetCurrentPlayer() : nullptr;

	static const TCHAR* IdNames[] = { TEXT("???"), TEXT("Lord"), TEXT("Loyalist"), TEXT("Rebel"), TEXT("Renegade") };

	for (const TObjectPtr<APlayerState>& PS : TKGS->PlayerArray)
	{
		ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
		if (!TKPS) continue;

		FTKPlayerUIData Data;
		Data.SeatIndex = TKPS->SeatIndex;
		Data.PlayerName = TKPS->GetPlayerName();
		uint8 Idx = (uint8)TKPS->Identity;
		Data.IdentityName = (Idx < 5) ? IdNames[Idx] : TEXT("???");
		Data.bAlive = TKPS->IsAlive();
		Data.CurrentHealth = TKPS->CurrentHealth;
		Data.MaxHealth = TKPS->MaxHealth;
		Data.HandCardCount = TKPS->HandCardCount;
		Data.bIsCurrentTurn = (TKPS == CurrentPlayer);
		Result.Add(Data);
	}

	return Result;
}

// ===== 创建 Widget =====

void ATKGameHUD::CreateMainHud()
{
	if (!HUDWidgetClass) return;

	ATKPlayerControllerBase* PC = Cast<ATKPlayerControllerBase>(GetOwningPlayerController());
	if (!PC) return;

	MainHud = CreateWidget<UTKGameHudWidget>(PC, HUDWidgetClass);
	if (MainHud)
	{
		MainHud->AddToViewport(0);
		MainHud->SetOwningHUD(this);
		UE_LOG(LogTKGame, Log, TEXT("TKGameHUD: MainHud created"));
	}
}

void ATKGameHUD::RefreshAll()
{
	if (MainHud)
	{
		MainHud->RefreshDisplay(GetGameUIData(), GetAllPlayerUIData());
	}
}
