#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TKGameTypes.h"
#include "TKCardZoneComponent.generated.h"

class UTKCardBase;
class UTKDeckComponent;

/**
 * 牌区组件
 * 挂载在 PlayerState 上，管理单个玩家的手牌区、装备区和判定区
 * 所有牌区支持复制到客户端，手牌仅 owner 可见（通过复制条件控制）
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TKGAMEA_API UTKCardZoneComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UTKCardZoneComponent(const FObjectInitializer& Initializer);

	// ---- 牌区操作 ----

	/** 添加一张卡牌到手牌区 */
	UFUNCTION(BlueprintCallable, Category = "CardZone")
	void AddCardToHand(UTKCardBase* Card);

	/** 从手牌区移除一张卡牌 */
	UFUNCTION(BlueprintCallable, Category = "CardZone")
	bool RemoveCardFromHand(UTKCardBase* Card);

	/** 获取手牌区所有卡牌 */
	UFUNCTION(BlueprintCallable, Category = "CardZone")
	TArray<UTKCardBase*> GetHandCards() const { return HandCards; }

	/** 获取手牌数量 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CardZone")
	int32 GetHandCardCount() const { return HandCards.Num(); }

	/** 获取装备区所有卡牌 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CardZone")
	TArray<UTKCardBase*> GetEquipmentCards() const { return EquipmentCards; }

	/** 获取判定区所有卡牌 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CardZone")
	TArray<UTKCardBase*> GetJudgementCards() const { return JudgementCards; }

	/** 添加一张卡牌到装备区 */
	UFUNCTION(BlueprintCallable, Category = "CardZone")
	void AddToEquipment(UTKCardBase* Card);

	/** 添加一张卡牌到判定区 */
	UFUNCTION(BlueprintCallable, Category = "CardZone")
	void AddToJudgement(UTKCardBase* Card);

	/** 从装备区移除一张卡牌 */
	UFUNCTION(BlueprintCallable, Category = "CardZone")
	bool RemoveFromEquipment(UTKCardBase* Card);

	/** 从判定区移除一张卡牌 */
	UFUNCTION(BlueprintCallable, Category = "CardZone")
	bool RemoveFromJudgement(UTKCardBase* Card);

	/**
	 * 清空所有牌区（仅清空数组，不回收卡牌）
	 * 
	 * ⚠️ 调用前必须先遍历牌区将牌移入弃牌堆。
	 *    需要自动回收请使用 ClearAndDiscardAll()。
	 */
	UFUNCTION(BlueprintCallable, Category = "CardZone")
	void ClearAllZones();

	/**
	 * 清空所有牌区并将牌回收至弃牌堆（死亡/断线时使用）
	 * @param Deck 牌堆组件（用于 DiscardCard 回收）
	 */
	UFUNCTION(BlueprintCallable, Category = "CardZone")
	void ClearAndDiscardAll(UTKDeckComponent* Deck);

	// ---- 复制 ----
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** 同步 HandCardCount 到 Owner PlayerState（操作手牌后自动调用） */
	void SyncHandCardCount();

	/** 根据 UTKCardBase 数组构建 FTKCardInstance 数组（客户端可见） */
	static TArray<FTKCardInstance> BuildInstanceArray(const TArray<TObjectPtr<UTKCardBase>>& Cards);

	// ---- 服务器端 UObject 牌区（权威数据） ----

	/** 手牌区 */
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UTKCardBase>> HandCards;

	/** 装备区 */
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UTKCardBase>> EquipmentCards;

	/** 判定区 */
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UTKCardBase>> JudgementCards;

	// ---- 客户端可读副本（FTKCardInstance 为 USTRUCT，天然支持网络复制） ----

	/** 手牌区客户端副本 */
	UPROPERTY(Replicated)
	TArray<FTKCardInstance> ReplicatedHandCards;

	/** 装备区客户端副本 */
	UPROPERTY(Replicated)
	TArray<FTKCardInstance> ReplicatedEquipmentCards;

	/** 判定区客户端副本 */
	UPROPERTY(Replicated)
	TArray<FTKCardInstance> ReplicatedJudgementCards;
};
