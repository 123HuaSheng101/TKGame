#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TKGameTypes.h"
#include "TKIdentityRuleComponent.generated.h"

class ATKPlayerStateBase;

/**
 * 身份规则组件
 * 挂载在 GameMode 上，仅服务器执行
 * 职责：身份分配（4~10 人标准配置）、死亡处理、三种胜利条件判定
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TKGAMEA_API UTKIdentityRuleComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UTKIdentityRuleComponent(const FObjectInitializer& Initializer);

	// ---- 身份分配 ----

	/**
	 * 给所有玩家分配身份
	 * 标准分配表：4人(1主1忠1反1内) ~ 10人(1主3忠5反1内)
	 * @param Players 所有玩家的 PlayerState 数组
	 */
	UFUNCTION(BlueprintCallable, Category = "Identity")
	void AssignIdentities(const TArray<APlayerState*>& Players);

	// ---- 胜负判定 ----

	/**
	 * 评估当前游戏胜负
	 * 判定逻辑：
	 *   主公死亡 + 内奸单独存活 → 内奸胜
	 *   主公死亡 + 有反贼存活   → 反贼胜
	 *   反贼/内奸全灭 + 主公存活 → 主忠胜
	 * @return 胜者阵营，None 表示游戏继续
	 */
	UFUNCTION(BlueprintCallable, Category = "Identity")
	ETKGameResult EvaluateVictory() const;

	// ---- 死亡处理 ----

	/**
	 * 处理玩家死亡
	 * 清空死亡玩家的牌区，并触发胜负判定
	 * @param DeadPlayer 已标记死亡的玩家
	 */
	UFUNCTION(BlueprintCallable, Category = "Identity")
	void ResolveDeath(ATKPlayerStateBase* DeadPlayer);

	// ---- 工具 ----

	/** 统计指定玩家数组中主公的数量 */
	static int32 GetLordCount(const TArray<ATKPlayerStateBase*>& Players);

	/** 统计指定玩家数组中忠臣的数量 */
	static int32 GetLoyalistCount(const TArray<ATKPlayerStateBase*>& Players);

	/** 统计指定玩家数组中反贼的数量 */
	static int32 GetRebelCount(const TArray<ATKPlayerStateBase*>& Players);

	/** 统计指定玩家数组中内奸的数量 */
	static int32 GetRenegadeCount(const TArray<ATKPlayerStateBase*>& Players);

	/** 统计指定玩家数组中存活的人数 */
	static int32 GetAliveCount(const TArray<ATKPlayerStateBase*>& Players);

	/** 筛选出指定 PlayerState 数组中存活的玩家 */
	static TArray<ATKPlayerStateBase*> GetAlivePlayers(const TArray<TObjectPtr<APlayerState>>& Players);

protected:
	/**
	 * 根据玩家索引和总人数确定该玩家的身份
	 * 索引 0 固定为主公，其余玩家随机分配
	 */
	static ETKIdentity DetermineIdentityForPlayer(int32 Index, int32 TotalPlayers);

	/** 根据总人数返回反贼数量 */
	static int32 GetRebelCountForPlayers(int32 TotalPlayers);

	/** 根据总人数返回忠臣数量 */
	static int32 GetLoyalistCountForPlayers(int32 TotalPlayers);
};
