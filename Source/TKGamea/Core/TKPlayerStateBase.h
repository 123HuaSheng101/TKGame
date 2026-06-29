// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TKGameTypes.h"
#include "TKPlayerStateBase.generated.h"

class UTKCardZoneComponent;

/**
 * 玩家状态
 * 承载每个玩家的运行时数据：座位、存活、身份、体力、手牌数
 * 所有关键字段通过 Replicated 复制到所有客户端
 */
UCLASS()
class TKGAMEA_API ATKPlayerStateBase : public APlayerState
{
	GENERATED_BODY()
public:
	ATKPlayerStateBase(const FObjectInitializer& Initializer);

	// ---- 牌区组件 ----

	/** 获取该玩家所持的牌区组件（管理手牌/装备/判定区） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Card")
	UTKCardZoneComponent* GetCardZone() const { return CardZone; }

	// ---- 座位 ----

	/** 玩家座位号（0 ~ N-1，在 PlayerArray 中的索引），INDEX_NONE 表示未分配 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	int32 SeatIndex = INDEX_NONE;

	// ---- 存活 ----

	/** 是否存活 */
	UPROPERTY(ReplicatedUsing = OnRep_Alive, BlueprintReadOnly, Category = "Player")
	bool bAlive = true;

	/** 存活状态复制回调 */
	UFUNCTION()
	void OnRep_Alive();

	// ---- 身份 ----

	/** 玩家身份（Unknown 表示未分配或隐藏） */
	UPROPERTY(ReplicatedUsing = OnRep_Identity, BlueprintReadOnly, Category = "Player")
	ETKIdentity Identity = ETKIdentity::Unknown;

	/** 身份复制回调 */
	UFUNCTION()
	void OnRep_Identity();

	/**
	 * 身份可见性
	 * M1 调试模式下始终可见
	 * 正式游戏中，仅主公身份对所有玩家公开，其他身份默认隐藏
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Player")
	bool bIdentityRevealed = false;

	// ---- 体力 ----

	/** 当前体力值 */
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Player")
	int32 CurrentHealth = 4;

	/** 体力上限 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	int32 MaxHealth = 4;

	/** 体力变化复制回调 */
	UFUNCTION()
	void OnRep_Health();

	// ---- 手牌数量（公开信息） ----

	/** 手牌数量（公开信息，所有人都能看到手牌数） */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	int32 HandCardCount = 0;

	// ---- 工具函数 ----

	/** 检查玩家是否存活 */
	UFUNCTION(BlueprintCallable, Category = "Player")
	bool IsAlive() const { return bAlive; }

	/**
	 * 设置玩家身份
	 * @param NewIdentity 要分配的身份
	 */
	UFUNCTION(BlueprintCallable, Category = "Player")
	void SetIdentity(ETKIdentity NewIdentity);

	/**
	 * 受到伤害
	 * @param Damage 伤害量
	 * @param DamageSource 伤害来源
	 * @return 如果玩家因此死亡返回 true，否则返回 false
	 */
	UFUNCTION(BlueprintCallable, Category = "Player")
	bool ApplyDamage(int32 Damage, ATKPlayerStateBase* DamageSource = nullptr);

	/**
	 * 恢复体力
	 * @param HealAmount 治疗量
	 * @return 是否成功回血（存活时有效）
	 */
	UFUNCTION(BlueprintCallable, Category = "Player")
	bool Heal(int32 HealAmount);

	/**
	 * 触发濒死求桃（由 ApplyDamage 自动调用，也可外部调用）
	 * 在 ResponseComponent 中发起 Sequential 请求
	 */
	UFUNCTION(BlueprintCallable, Category = "Player")
	void TriggerDyingResponse();

	/** 濒死求桃回调 */
	void OnDyingResolved(const FResponseResult& Result);

	/** 检查游戏结束条件 */
	void CheckGameEnd();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** 牌区组件（自动创建） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UTKCardZoneComponent> CardZone;

	/** 最后一次受到伤害的来源玩家（用于奖惩判定） */
	UPROPERTY(BlueprintReadOnly, Category = "Player")
	TObjectPtr<ATKPlayerStateBase> LastDamageSource;
};
