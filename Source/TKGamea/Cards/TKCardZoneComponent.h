#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TKGameTypes.h"
#include "TKCardZoneComponent.generated.h"

class UTKCardBase;

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

	/** 添加一张卡牌到装备区 */
	UFUNCTION(BlueprintCallable, Category = "CardZone")
	void AddToEquipment(UTKCardBase* Card);

	/** 添加一张卡牌到判定区 */
	UFUNCTION(BlueprintCallable, Category = "CardZone")
	void AddToJudgement(UTKCardBase* Card);

	/**
	 * 清空所有牌区
	 * 通常用于玩家死亡时清理
	 */
	UFUNCTION(BlueprintCallable, Category = "CardZone")
	void ClearAllZones();

	// ---- 复制 ----
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** 手牌区 */
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UTKCardBase>> HandCards;

	/** 装备区 */
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UTKCardBase>> EquipmentCards;

	/** 判定区 */
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UTKCardBase>> JudgementCards;
};
