#include "TKGamea.h"
#include "TKResponseComponent.h"
#include "TKGameStateBase.h"
#include "TKPlayerStateBase.h"
#include "TKPlayerControllerBase.h"
#include "TKEventComponentBase.h"
#include "Game/TKCardBase.h"
#include "Game/TKCard_Basic.h"
#include "Engine/World.h"
#include "TimerManager.h"

static FGameplayTag Tag_Card_Negate = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Negate"), false);
static FGameplayTag Tag_Card_Peach  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Peach"), false);
static FGameplayTag Tag_Card_Wine   = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Wine"), false);
static FGameplayTag Tag_Card_Slash  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Slash"), false);
static FGameplayTag Tag_Card_Dodge  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Dodge"), false);

UTKResponseComponent::UTKResponseComponent(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

UTKResponseComponent* UTKResponseComponent::Get(const ATKGameStateBase* GameState)
{
	if (!GameState || !IsValid(GameState)) return nullptr;
	return GameState->ResponseComponent.Get();
}

TArray<APlayerState*> UTKResponseComponent::GetAlivePlayers() const
{
	TArray<APlayerState*> Result;
	ATKGameStateBase* GS = Cast<ATKGameStateBase>(GetOwner());
	if (!GS) return Result;

	for (const TObjectPtr<APlayerState>& PS : GS->PlayerArray)
	{
		ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
		if (TKPS && TKPS->IsAlive())
			Result.Add(PS);
	}
	return Result;
}

TArray<APlayerState*> UTKResponseComponent::BuildResponderQueue(const TArray<APlayerState*>& AllPlayers, APlayerState* StartFrom, APlayerState* SkipPlayer)
{
	TArray<APlayerState*> Queue;
	int32 StartIdx = AllPlayers.IndexOfByKey(StartFrom);
	if (StartIdx == INDEX_NONE) StartIdx = 0;

	int32 N = AllPlayers.Num();
	for (int32 i = 1; i <= N; i++)
	{
		int32 Idx = (StartIdx + i) % N;
		APlayerState* PS = AllPlayers[Idx];
		if (PS == SkipPlayer) continue;
		ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
		if (TKPS && TKPS->IsAlive())
			Queue.Add(PS);
	}
	return Queue;
}

// ===== 工具 =====

void UTKResponseComponent::StartTimeout()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(TimeoutHandle, this, &UTKResponseComponent::OnRequestTimeout, ResponseTimeout, false);
	}
}

bool UTKResponseComponent::IsWaitingForResponse() const
{
	return ActiveRequest.Result == ETKResponseResult::Pending;
}

void UTKResponseComponent::ProcessNextPending()
{
	if (PendingRequests.Num() == 0 || PendingCallbacks.Num() == 0) return;

	ActiveRequest = PendingRequests[0];
	ActiveCallback = PendingCallbacks[0];
	PendingRequests.RemoveAt(0);
	PendingCallbacks.RemoveAt(0);
	ChainDepth = 0;
	SequentialPassed = 0;

	BroadcastRequestToClients();

	if (ActiveRequest.Type == ETKResponseType::Sequential)
		StartNextInQueue();
	else
		StartTimeout();
}

// ===== 发起请求 =====

void UTKResponseComponent::RequestResponse(const FResponseRequest& Request, FOnResponseResolved OnResolved)
{
	// 当前有活跃请求 → 入队等待
	if (ActiveRequest.IsPending())
	{
		PendingRequests.Add(Request);
		PendingCallbacks.Add(OnResolved);
		return;
	}

	ActiveRequest = Request;
	ActiveCallback = OnResolved;
	ChainDepth = 0;
	SequentialPassed = 0;

	// Sequential 模式：初始化队列
	if (ActiveRequest.Type == ETKResponseType::Sequential && ActiveRequest.ResponderQueue.Num() == 0)
	{
		ActiveRequest.ResponderQueue = GetAlivePlayers();
		if (ActiveRequest.Initiator)
		{
			ActiveRequest.ResponderQueue.Remove(ActiveRequest.Initiator);
		}
		ActiveRequest.CurrentResponderIndex = 0;
	}

	BroadcastRequestToClients();

	if (ActiveRequest.Type == ETKResponseType::Sequential)
	{
		StartNextInQueue();
	}
	else
	{
		StartTimeout();
	}

	UE_LOG(LogTKGame, Log, TEXT("ResponseComponent: RequestResponse type=%d tag=[%s]"),
		(uint8)ActiveRequest.Type, *ActiveRequest.RequiredTag.GetTagName().ToString());
}

// ===== 广播 =====

void UTKResponseComponent::BroadcastRequestToClients()
{
	FEventContext Ctx;
	Ctx.EventTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Response.Request"), false);

	if (ActiveRequest.Initiator)
	{
		APlayerController* PC = ActiveRequest.Initiator->GetPlayerController();
		Ctx.EventInitiator = Cast<ATKPlayerControllerBase>(PC);
	}

	// Chain 模式：广播给所有存活玩家
	if (ActiveRequest.Type == ETKResponseType::Chain)
	{
		TArray<APlayerState*> Alive = GetAlivePlayers();
		for (APlayerState* PS : Alive)
		{
			APlayerController* PC = PS->GetPlayerController();
			if (ATKPlayerControllerBase* TKPC = Cast<ATKPlayerControllerBase>(PC))
				Ctx.Targets.Add(TKPC);
		}
	}
	// Sequential / SingleTarget：只通知当前等待者
	else if (ActiveRequest.PrimaryTarget)
	{
		APlayerController* PC = ActiveRequest.PrimaryTarget->GetPlayerController();
		if (ATKPlayerControllerBase* TKPC = Cast<ATKPlayerControllerBase>(PC))
			Ctx.Targets.Add(TKPC);
	}

	Ctx.Params_g.Add(ActiveRequest.RequiredTag);

	if (Ctx.EventInitiator)
	{
		UTKEventComponentBase* EventComp = Ctx.EventInitiator->GetEventComponent();
		if (EventComp) EventComp->PostEvent(Ctx);
	}

	UE_LOG(LogTKGame, Log, TEXT("ResponseComponent: Broadcast type=%d tag=[%s] targets=%d"),
		(uint8)ActiveRequest.Type, *ActiveRequest.RequiredTag.GetTagName().ToString(), Ctx.Targets.Num());
}

// ===== Sequential 逐人推进 =====

void UTKResponseComponent::StartNextInQueue()
{
	const TArray<TObjectPtr<APlayerState>>& Queue = ActiveRequest.ResponderQueue;
	// 跳过已响应或已死亡的玩家
	while (ActiveRequest.CurrentResponderIndex < Queue.Num())
	{
		APlayerState* Next = Queue[ActiveRequest.CurrentResponderIndex];
		ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(Next);
		if (TKPS && TKPS->IsAlive())
		{
			ActiveRequest.PrimaryTarget = Next;
			BroadcastRequestToClients();
			StartTimeout();
			UE_LOG(LogTKGame, Log, TEXT("ResponseComponent: Sequential → asking [%s] (index=%d/%d)"),
				*Next->GetPlayerName(), ActiveRequest.CurrentResponderIndex, Queue.Num());
			return;
		}
		ActiveRequest.CurrentResponderIndex++;
	}

	// 队列遍历完毕
	ResolveRequest(ETKResponseResult::AllPassed);
}

void UTKResponseComponent::OnSequentialTimeout()
{
	// AOE 响应超时：当前玩家未出杀/闪 → 扣1血
	if (ActiveRequest.RequiredTag != Tag_Card_Peach)
	{
		ATKPlayerStateBase* CurrentTarget = Cast<ATKPlayerStateBase>(ActiveRequest.PrimaryTarget);
		if (CurrentTarget && CurrentTarget->IsAlive())
		{
			CurrentTarget->ApplyDamage(1);
			UE_LOG(LogTKGame, Log, TEXT("ResponseComponent: AOE timeout → [%s] takes 1 damage"),
				*CurrentTarget->GetPlayerName());
		}
	}

	ActiveRequest.CurrentResponderIndex++;
	StartNextInQueue();
}

// ===== 提交响应 =====

bool UTKResponseComponent::SubmitResponse(ATKPlayerStateBase* Responder, UTKCardBase* ResponseCard)
{
	if (!Responder || !ResponseCard) return false;
	if (!ActiveRequest.IsPending()) return false;

	const FGameplayTag& FirstTag = ResponseCard->EffectTags.Num() > 0 ? ResponseCard->EffectTags[0] : FGameplayTag();

	// 濒死时酒也可用（RequiredTag 是 Peach 时允许 Wine）
	bool bTagMatch = (FirstTag == ActiveRequest.RequiredTag);
	if (!bTagMatch && ActiveRequest.RequiredTag == Tag_Card_Peach && FirstTag == Tag_Card_Wine)
	{
		bTagMatch = true; // 酒可代桃救人
	}

	if (!bTagMatch)
	{
		UE_LOG(LogTKGame, Warning, TEXT("ResponseComponent: Tag mismatch. Expected [%s], got [%s]"),
			*ActiveRequest.RequiredTag.GetTagName().ToString(), *FirstTag.GetTagName().ToString());
		return false;
	}

	ActiveRequest.AlreadyResponded.Add(Responder);
	UE_LOG(LogTKGame, Log, TEXT("ResponseComponent: [%s] responded with [%s]"),
		*Responder->GetPlayerName(), *ResponseCard->CardDefId.ToString());

	switch (ActiveRequest.Type)
	{
	case ETKResponseType::SingleTarget:
		ResolveRequest(ETKResponseResult::Responded, Responder, ResponseCard);
		break;

	case ETKResponseType::Chain:
	{
		// 无懈可击链：有人出无懈 → 链深度+1 → 重新开链（无懈可以无懈无懈）
		ChainDepth++;
		UWorld* World = GetWorld();
		if (World) World->GetTimerManager().ClearTimer(TimeoutHandle);

		UE_LOG(LogTKGame, Log, TEXT("ResponseComponent: Chain depth=%d, restarting chain..."), ChainDepth);
		BroadcastRequestToClients();
		StartTimeout();
		break;
	}

	case ETKResponseType::Sequential:
		// Sequential：判断是濒死求桃还是 AOE
		//   濒死(Peach)：有人出桃→立即结算
		//   AOE(Slash/Dodge)：当前玩家安全→推进下一个
		if (ActiveRequest.RequiredTag == Tag_Card_Peach)
		{
			ResolveRequest(ETKResponseResult::Responded, Responder, ResponseCard);
		}
		else
		{
			SequentialPassed++;
			ActiveRequest.CurrentResponderIndex++;
			StartNextInQueue();
		}
		break;

	default:
		break;
	}

	return true;
}

// ===== 超时 =====

void UTKResponseComponent::OnRequestTimeout()
{
	switch (ActiveRequest.Type)
	{
	case ETKResponseType::Chain:
		// 无懈链超时：链深度为奇数则原效果被抵消
	{
		bool bNegated = (ChainDepth % 2 == 1);
		UE_LOG(LogTKGame, Log, TEXT("ResponseComponent: Chain timeout, depth=%d, bNegated=%d"), ChainDepth, bNegated ? 1 : 0);
		ResolveRequest(bNegated ? ETKResponseResult::Responded : ETKResponseResult::Timeout);
		break;
	}
	case ETKResponseType::Sequential:
		OnSequentialTimeout();
		break;
	default:
		UE_LOG(LogTKGame, Warning, TEXT("ResponseComponent: Request timeout"));
		ResolveRequest(ETKResponseResult::Timeout);
		break;
	}
}

// ===== 结算 =====

void UTKResponseComponent::ResolveRequest(ETKResponseResult Result, ATKPlayerStateBase* Responder, UTKCardBase* ResponseCard)
{
	UWorld* World = GetWorld();
	if (World) World->GetTimerManager().ClearTimer(TimeoutHandle);

	ActiveRequest.Result = Result;

	FResponseResult ResolveResult;
	ResolveResult.Result = Result;
	ResolveResult.Responder = Responder;
	ResolveResult.ResponseCard = ResponseCard;
	ResolveResult.bNegated = (Result == ETKResponseResult::Responded);

	if (ActiveCallback.IsBound())
	{
		ActiveCallback.Execute(ResolveResult);
	}
	ActiveCallback.Unbind();

	ActiveRequest = FResponseRequest();
	ChainDepth = 0;
	SequentialPassed = 0;

	ProcessNextPending();
}
