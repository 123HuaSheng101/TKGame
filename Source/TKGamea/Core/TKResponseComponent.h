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
 *   SingleTarget — 单目标等待超时后自动结算（杀→闪）
 *   Chain        — 无懈可击链：全服广播，有人出无懈则重新开链
 *   Sequential   — 逐人问询：AOE/濒死，逆时针逐人超时后自动下一个
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TKGAMEA_API UTKResponseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTKResponseComponent(const FObjectInitializer& Initializer);

	static UTKResponseComponent* Get(const ATKGameStateBase* GameState);

	/** 发起响应请求 */
	void RequestResponse(const FResponseRequest& Request, FOnResponseResolved OnResolved);

	/** 提交响应牌 */
	bool SubmitResponse(ATKPlayerStateBase* Responder, UTKCardBase* ResponseCard);

	/** 检查是否有待处理请求 */
	bool IsWaitingForResponse() const;

	const FResponseRequest& GetActiveRequest() const { return ActiveRequest; }

	/** 获取所有存活玩家（供外部构建 ResponderQueue） */
	TArray<APlayerState*> GetAlivePlayers() const;

	/** 构建逆时针响应对列（从指定玩家起，跳过被跳过者） */
	static TArray<APlayerState*> BuildResponderQueue(const TArray<TObjectPtr<APlayerState>>& AllPlayers, APlayerState* StartFrom, APlayerState* SkipPlayer = nullptr);

protected:
	void OnRequestTimeout();
	void OnSequentialTimeout();
	void ResolveRequest(ETKResponseResult Result, ATKPlayerStateBase* Responder = nullptr, UTKCardBase* ResponseCard = nullptr);
	void BroadcastRequestToClients();
	void StartNextInQueue();
	void ProcessNextPending();
	void StartTimeout();

private:
	UPROPERTY()
	TArray<FResponseRequest> PendingRequests;
	TArray<FOnResponseResolved> PendingCallbacks;

	FResponseRequest ActiveRequest;
	FOnResponseResolved ActiveCallback;
	FTimerHandle TimeoutHandle;

	UPROPERTY(EditAnywhere, Category = "Response")
	float ResponseTimeout = 10.0f;

	/** 无懈可击链深度（奇数次=原效果被抵消） */
	int32 ChainDepth = 0;

	/** Sequential 模式下已跳过（被响应）的玩家数 */
	int32 SequentialPassed = 0;
};

