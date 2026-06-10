#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TKGameTypes.h"
#include "TKCardTypes.h"
#include "TKDeckComponent.generated.h"

class UTKCardBase;

/**
 * 牌堆组件
 * 挂载在 GameMode 上，仅服务器执行
 * 负责卡牌的权威创建、洗牌、摸牌（牌堆为空时自动将弃牌堆洗回）、弃牌回收
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TKGAMEA_API UTKDeckComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UTKDeckComponent(const FObjectInitializer& Initializer);

	// ---- 初始化 ----

	/** 初始化调试牌堆：根据模板条目生成卡牌并洗牌 */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	void InitDebugDeck(const TArray<FTKDebugCardEntry>& DebugCards);

	// ---- 牌堆操作 ----

	/** Fisher-Yates 洗牌 */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	void Shuffle();

	/** 摸一张牌（牌堆为空时自动洗回弃牌堆） */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	UTKCardBase* DrawCard();

	/** 摸多张牌 */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	TArray<UTKCardBase*> DrawMultipleCards(int32 Count);

	/** 将一张牌放入弃牌堆 */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	void DiscardCard(UTKCardBase* Card);

	// ---- 查询 ----

	/** 获取牌堆剩余卡牌数量 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Deck")
	int32 GetDeckCount() const { return DeckCards.Num(); }

	/** 获取弃牌堆卡牌数量 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Deck")
	int32 GetDiscardPileCount() const { return DiscardPile.Num(); }

	// ---- 复制 ----
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** 牌堆（摸牌从这里取） */
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UTKCardBase>> DeckCards;

	/** 弃牌堆 */
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UTKCardBase>> DiscardPile;

	/** 全局卡牌实例 ID 自增计数器 */
	int32 NextInstanceId = 0;

	/** 根据调试条目创建一张卡牌实例 */
	UTKCardBase* CreateCardInstance(const FTKDebugCardEntry& Entry);
};
