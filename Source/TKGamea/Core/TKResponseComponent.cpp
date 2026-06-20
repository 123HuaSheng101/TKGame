#include "TKGamea.h"
// TKResponseComponent.cpp

#include "TKResponseComponent.h"
#include "TKGameStateBase.h"
#include "TKPlayerStateBase.h"
#include "TKPlayerControllerBase.h"
#include "TKEventComponentBase.h"
#include "Game/TKCardBase.h"
#include "Game/TKCard_Basic.h"
#include "Engine/World.h"
#include "TimerManager.h"

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

void UTKResponseComponent::RequestResponse(const FResponseRequest& Request, FOnResponseResolved OnResolved)
{
	// 入队列
	PendingRequests.Add(Request);

	// 如果当前没有活跃请求，取出第一个处理
	if (ActiveRequest.Result == ETKResponseResult::Pending && PendingRequests.Num() > 0)
	{
		ActiveRequest = PendingRequests[0];
		PendingRequests.RemoveAt(0);
		ActiveCallback = OnResolved;
	}
	else if (ActiveRequest.Result != ETKResponseResult::Pending)
	{
		// 当前无活跃请求，直接处理
		if (PendingRequests.Num() > 0)
		{
			ActiveRequest = PendingRequests[0];
			PendingRequests.RemoveAt(0);
			ActiveCallback = OnResolved;
		}
	}
	else
	{
		// 已有活跃请求，等结束后再处理
		PendingCallbacks.Add(OnResolved);
		return;
	}

	// 广播到客户端
	BroadcastRequestToClients();

	// 单目标/链式响应：启动超时定时器
	if (ActiveRequest.Type == ETKResponseType::SingleTarget ||
		ActiveRequest.Type == ETKResponseType::Chain)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			World->GetTimerManager().SetTimer(TimeoutHandle, this, &UTKResponseComponent::OnRequestTimeout, ResponseTimeout, false);
		}
	}

	UE_LOG(LogTKGame, Log, TEXT("ResponseComponent: RequestResponse type=%d, target=[%s]"),
		(uint8)ActiveRequest.Type,
		ActiveRequest.PrimaryTarget ? *ActiveRequest.PrimaryTarget->GetPlayerName() : TEXT("None"));
}

void UTKResponseComponent::BroadcastRequestToClients()
{
	// 构建事件
	FEventContext Ctx;
	Ctx.EventTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Response.Request"), false);

	// 发起者
	if (ActiveRequest.Initiator)
	{
		APlayerController* PC = ActiveRequest.Initiator->GetPlayerController();
		Ctx.EventInitiator = Cast<ATKPlayerControllerBase>(PC);
	}

	// 需要响应的玩家
	if (ActiveRequest.PrimaryTarget)
	{
		APlayerController* PC = ActiveRequest.PrimaryTarget->GetPlayerController();
		if (ATKPlayerControllerBase* TKPC = Cast<ATKPlayerControllerBase>(PC))
		{
			Ctx.Targets.Add(TKPC);
		}
	}

	// 写入响应 Tag
	Ctx.Params_g.Add(ActiveRequest.RequiredTag);

	// 通过发起者的 EventComponent 广播
	if (Ctx.EventInitiator)
	{
		UTKEventComponentBase* EventComp = Ctx.EventInitiator->GetEventComponent();
		if (EventComp)
		{
			EventComp->PostEvent(Ctx);
		}
	}

	UE_LOG(LogTKGame, Log, TEXT("ResponseComponent: Broadcast response request to clients"));
}

bool UTKResponseComponent::SubmitResponse(ATKPlayerStateBase* Responder, UTKCardBase* ResponseCard)
{
	if (!Responder || !ResponseCard) return false;
	if (!ActiveRequest.IsPending()) return false;

	// 校验：响应卡牌 Tag 是否匹配
	const FGameplayTag& FirstTag = ResponseCard->EffectTags.Num() > 0 ? ResponseCard->EffectTags[0] : FGameplayTag();
	if (FirstTag != ActiveRequest.RequiredTag)
	{
		UE_LOG(LogTKGame, Warning, TEXT("ResponseComponent: Response card tag mismatch. Expected [%s], got [%s]"),
			*ActiveRequest.RequiredTag.GetTagName().ToString(), *FirstTag.GetTagName().ToString());
		return false;
	}

	// 记录响应
	ActiveRequest.AlreadyResponded.Add(Responder);

	UE_LOG(LogTKGame, Log, TEXT("ResponseComponent: [%s] responded with card [%s]"),
		*Responder->GetPlayerName(), *ResponseCard->CardDefId.ToString());

	// 根据响应类型结算
	switch (ActiveRequest.Type)
	{
	case ETKResponseType::SingleTarget:
		// 单目标响应：有人出了响应牌 → 结算为 Responded（伤害抵消）
		ResolveRequest(ETKResponseResult::Responded, Responder, ResponseCard);
		break;

	case ETKResponseType::Chain:
		// 链式响应：有人出无懈 → 重新开一轮无懈窗口
		// TODO: 逆时针重新询问
		ResolveRequest(ETKResponseResult::Responded, Responder, ResponseCard);
		break;

	case ETKResponseType::Sequential:
		// 逐人响应：检查是否所有人都问了
		ActiveRequest.CurrentResponderIndex++;
		if (ActiveRequest.CurrentResponderIndex >= ActiveRequest.ResponderQueue.Num())
		{
			ResolveRequest(ETKResponseResult::AllPassed);
		}
		else
		{
			// 继续问下一个
			BroadcastRequestToClients();
		}
		break;

	case ETKResponseType::Duel:
		// 决斗：双方轮流
		// TODO: 交换攻击方，重新开窗口
		break;

	default:
		break;
	}

	return true;
}

void UTKResponseComponent::OnRequestTimeout()
{
	UE_LOG(LogTKGame, Warning, TEXT("ResponseComponent: Request timeout, no one responded"));
	ResolveRequest(ETKResponseResult::Timeout);
}

void UTKResponseComponent::ResolveRequest(ETKResponseResult Result, ATKPlayerStateBase* Responder, UTKCardBase* ResponseCard)
{
	// 清理定时器
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(TimeoutHandle);
	}

	ActiveRequest.Result = Result;

	// 构建回调结果
	FResponseResult ResolveResult;
	ResolveResult.Result = Result;
	ResolveResult.Responder = Responder;
	ResolveResult.ResponseCard = ResponseCard;
	ResolveResult.bNegated = (Result == ETKResponseResult::Responded);

	// 执行回调
	if (ActiveCallback.IsBound())
	{
		ActiveCallback.Execute(ResolveResult);
	}
	ActiveCallback.Unbind();

	// 重置活跃请求
	ActiveRequest = FResponseRequest();

	// 处理下一个等待中的请求
	if (PendingRequests.Num() > 0 && PendingCallbacks.Num() > 0)
	{
		ActiveRequest = PendingRequests[0];
		ActiveCallback = PendingCallbacks[0];
		PendingRequests.RemoveAt(0);
		PendingCallbacks.RemoveAt(0);

		BroadcastRequestToClients();

		if (ActiveRequest.Type == ETKResponseType::SingleTarget ||
			ActiveRequest.Type == ETKResponseType::Chain)
		{
			if (World)
			{
				World->GetTimerManager().SetTimer(TimeoutHandle, this, &UTKResponseComponent::OnRequestTimeout, ResponseTimeout, false);
			}
		}
	}
}

bool UTKResponseComponent::IsWaitingForResponse() const
{
	return ActiveRequest.Result == ETKResponseResult::Pending;
}

APlayerState* UTKResponseComponent::FindNextAliveInOrder(const TArray<APlayerState*>& PlayerArray, APlayerState* Current)
{
	if (PlayerArray.Num() == 0) return nullptr;

	int32 CurrentIndex = PlayerArray.IndexOfByKey(Current);
	int32 NumPlayers = PlayerArray.Num();
	int32 StartIndex = (CurrentIndex == INDEX_NONE) ? 0 : CurrentIndex;

	for (int32 Offset = 1; Offset <= NumPlayers; Offset++)
	{
		int32 NextIndex = (StartIndex + Offset) % NumPlayers;
		ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PlayerArray[NextIndex]);
		if (TKPS && TKPS->IsAlive())
			return PlayerArray[NextIndex];
	}
	return nullptr;
}
