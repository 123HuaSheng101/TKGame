// TKResponseComponent.h — 响应系统核心管理器

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TKGameTypes.h"
#include "TKResponseComponent.generated.h"

class UTKCardBase;
class ATKPlayerControllerBase;
class ATKPlayerStateBase;
class ATKGameStateBase;

/** 响应结果委托 */
DECLARE_DELEGATE_OneParam(FOnResponseResolved, const FResponseResult&);

/**
 * 响应系统核心管理器
 * 挂载在 GameState 上，管理所有待处理的响应请求
 *
 * 三种响应模式：
 *   SingleTarget — 单目标等待超时后自动结算
 *   Chain        — 无懈可击链，逆时针轮询
 *   Sequential   — AOE/濒死，逆时针逐人询问
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TKGAMEA_API UTKResponseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTKResponseComponent(const FObjectInitializer& Initializer);

	/** 静态获取 */
	static UTKResponseComponent* Get(const ATKGameStateBase* GameState);

	// ---- 提交流程 ----

	/**
	 * 发起响应请求
	 * @param Request   响应请求数据
	 * @param OnResolved 结算回调（请求结束时调用）
	 */
	void RequestResponse(const FResponseRequest& Request, FOnResponseResolved OnResolved);

	/**
	 * 客户端通过 RPC 提交响应
	 * @param Responder  响应者 PlayerState
	 * @param ResponseCard 响应用的卡牌（杀/闪/无懈可击/桃）
	 */
	bool SubmitResponse(ATKPlayerStateBase* Responder, UTKCardBase* ResponseCard);

	/** 检查是否有待处理的响应请求 */
	bool IsWaitingForResponse() const;

	// ---- 查询 ----

	/** 获取当前活跃的响应请求 */
	const FResponseRequest& GetActiveRequest() const { return ActiveRequest; }

protected:
	/** 超时回调（Timer 触发） */
	void OnRequestTimeout();

	/** 结算当前请求 */
	void ResolveRequest(ETKResponseResult Result, ATKPlayerStateBase* Responder = nullptr, UTKCardBase* ResponseCard = nullptr);

	/** 将活跃请求广播到目标客户端 */
	void BroadcastRequestToClients();

	/** 按座位找下一个存活玩家 */
	static APlayerState* FindNextAliveInOrder(const TArray<APlayerState*>& PlayerArray, APlayerState* Current);

protected:
	/** 请求队列 */
	UPROPERTY()
	TArray<FResponseRequest> PendingRequests;

	/** 对应请求队列的回调 */
	TArray<FOnResponseResolved> PendingCallbacks;

	/** 当前活跃请求 */
	FResponseRequest ActiveRequest;

	/** 活跃请求的回调 */
	FOnResponseResolved ActiveCallback;

	/** 超时句柄 */
	FTimerHandle TimeoutHandle;

	/** 响应超时时间（秒） */
	UPROPERTY(EditAnywhere, Category = "Response")
	float ResponseTimeout = 10.0f;
};
