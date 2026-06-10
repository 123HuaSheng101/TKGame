// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TKGameTypes.h"
#include "TKCardBase.generated.h"

/**
 * 卡牌运行时对象
 * 每张实体卡对应一个 UTKCardBase 实例，包含牌面属性和当前所在区域
 */
UCLASS()
class TKGAMEA_API UTKCardBase : public UObject
{
	GENERATED_BODY()
public:
	UTKCardBase();

	/**
	 * 使用/激活此卡牌
	 * M1 阶段为空实现，M2 会补全完整使用效果逻辑
	 */
	UFUNCTION(BlueprintCallable, Category = "Card")
	void ActiveCard();

	/** 卡牌模板标识 ID */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FName CardDefId;

	/** 卡牌显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FText CardName;

	/** 卡牌大类类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	ETKCardType CardType = ETKCardType::Basic;

	/** 花色 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	ETKCardSuit Suit = ETKCardSuit::NoSuit;

	/** 点数（1~13） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	int32 Rank = 0;

	/** 当前所在的牌区（由牌区组件维护） */
	UPROPERTY(BlueprintReadOnly, Category = "Card")
	ETKCardZone ZoneType = ETKCardZone::Deck;
};
