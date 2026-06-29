#include "TKCardZoneComponent.h"
#include "TKGamea.h"
#include "Game/TKCardBase.h"
#include "Cards/TKDeckComponent.h"
#include "Core/TKPlayerStateBase.h"
#include "Net/UnrealNetwork.h"

UTKCardZoneComponent::UTKCardZoneComponent(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UTKCardZoneComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTKCardZoneComponent, HandCards);
	DOREPLIFETIME(UTKCardZoneComponent, EquipmentCards);
	DOREPLIFETIME(UTKCardZoneComponent, JudgementCards);
	DOREPLIFETIME(UTKCardZoneComponent, ReplicatedHandCards);
	DOREPLIFETIME(UTKCardZoneComponent, ReplicatedEquipmentCards);
	DOREPLIFETIME(UTKCardZoneComponent, ReplicatedJudgementCards);
}

// ---- 客户端副本构建 ----

TArray<FTKCardInstance> UTKCardZoneComponent::BuildInstanceArray(const TArray<TObjectPtr<UTKCardBase>>& Cards)
{
	TArray<FTKCardInstance> Result;
	for (const UTKCardBase* Card : Cards)
	{
		if (Card)
		{
			FTKCardInstance Inst;
			Inst.CardDefId  = Card->CardDefId;
			Inst.CardType   = Card->CardType;
			Inst.Suit       = Card->Suit;
			Inst.Rank       = Card->Rank;
			Inst.CardName   = Card->CardName;
			Inst.EffectTags = Card->EffectTags;
			Inst.ZoneType   = Card->ZoneType;
			Result.Add(Inst);
		}
	}
	return Result;
}

// ---- HandCardCount 自动同步 (P5) ----

void UTKCardZoneComponent::SyncHandCardCount()
{
	ATKPlayerStateBase* OwnerPS = Cast<ATKPlayerStateBase>(GetOwner());
	if (OwnerPS)
	{
		OwnerPS->HandCardCount = HandCards.Num();
	}
}

void UTKCardZoneComponent::AddCardToHand(UTKCardBase* Card)
{
	if (Card == nullptr) return;
	HandCards.Add(Card);
	Card->ZoneType = ETKCardZone::Hand;
	SyncHandCardCount();
	ReplicatedHandCards = BuildInstanceArray(HandCards);
	UE_LOG(LogTKGame, Log, TEXT("CardZone: Added card [%s] to hand, total: %d"), *Card->CardDefId.ToString(), HandCards.Num());
}

bool UTKCardZoneComponent::RemoveCardFromHand(UTKCardBase* Card)
{
	if (Card == nullptr) return false;
	int32 Removed = HandCards.Remove(Card);
	if (Removed > 0)
	{
		Card->ZoneType = ETKCardZone::OutOfGame;
		SyncHandCardCount();
		ReplicatedHandCards = BuildInstanceArray(HandCards);
		UE_LOG(LogTKGame, Log, TEXT("CardZone: Removed card [%s] from hand"), *Card->CardDefId.ToString());
		return true;
	}
	return false;
}

void UTKCardZoneComponent::AddToEquipment(UTKCardBase* Card)
{
	if (Card == nullptr) return;
	EquipmentCards.Add(Card);
	Card->ZoneType = ETKCardZone::Equipment;
	ReplicatedEquipmentCards = BuildInstanceArray(EquipmentCards);
}

void UTKCardZoneComponent::AddToJudgement(UTKCardBase* Card)
{
	if (Card == nullptr) return;
	JudgementCards.Add(Card);
	Card->ZoneType = ETKCardZone::Judgement;
	ReplicatedJudgementCards = BuildInstanceArray(JudgementCards);
}

bool UTKCardZoneComponent::RemoveFromEquipment(UTKCardBase* Card)
{
	if (Card == nullptr) return false;
	int32 Removed = EquipmentCards.Remove(Card);
	if (Removed > 0)
	{
		Card->ZoneType = ETKCardZone::OutOfGame;
		ReplicatedEquipmentCards = BuildInstanceArray(EquipmentCards);
		return true;
	}
	return false;
}

bool UTKCardZoneComponent::RemoveFromJudgement(UTKCardBase* Card)
{
	if (Card == nullptr) return false;
	int32 Removed = JudgementCards.Remove(Card);
	if (Removed > 0)
	{
		Card->ZoneType = ETKCardZone::OutOfGame;
		ReplicatedJudgementCards = BuildInstanceArray(JudgementCards);
		return true;
	}
	return false;
}

void UTKCardZoneComponent::ClearAllZones()
{
	HandCards.Empty();
	EquipmentCards.Empty();
	JudgementCards.Empty();
	SyncHandCardCount();
	ReplicatedHandCards.Empty();
	ReplicatedEquipmentCards.Empty();
	ReplicatedJudgementCards.Empty();
	UE_LOG(LogTKGame, Log, TEXT("CardZone: All zones cleared"));
}

// ---- 回收后清空 (P3) ----

void UTKCardZoneComponent::ClearAndDiscardAll(UTKDeckComponent* Deck)
{
	if (!Deck) return;

	auto DiscardArray = [Deck](TArray<TObjectPtr<UTKCardBase>>& Cards) {
		for (UTKCardBase* Card : Cards)
		{
			if (Card) Deck->DiscardCard(Card);
		}
		Cards.Empty();
	};

	DiscardArray(HandCards);
	DiscardArray(EquipmentCards);
	DiscardArray(JudgementCards);
	SyncHandCardCount();
	ReplicatedHandCards.Empty();
	ReplicatedEquipmentCards.Empty();
	ReplicatedJudgementCards.Empty();
	UE_LOG(LogTKGame, Log, TEXT("CardZone: All cards discarded and zones cleared"));
}
