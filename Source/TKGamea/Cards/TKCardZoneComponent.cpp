#include "TKCardZoneComponent.h"
#include "TKGamea.h"
#include "Game/TKCardBase.h"
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
}

void UTKCardZoneComponent::AddCardToHand(UTKCardBase* Card)
{
	if (Card == nullptr) return;
	HandCards.Add(Card);
	Card->ZoneType = ETKCardZone::Hand;
	UE_LOG(LogTKGame, Log, TEXT("CardZone: Added card [%s] to hand, total: %d"), *Card->CardDefId.ToString(), HandCards.Num());
}

bool UTKCardZoneComponent::RemoveCardFromHand(UTKCardBase* Card)
{
	if (Card == nullptr) return false;
	int32 Removed = HandCards.Remove(Card);
	if (Removed > 0)
	{
		Card->ZoneType = ETKCardZone::OutOfGame;
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
}

void UTKCardZoneComponent::AddToJudgement(UTKCardBase* Card)
{
	if (Card == nullptr) return;
	JudgementCards.Add(Card);
	Card->ZoneType = ETKCardZone::Judgement;
}

bool UTKCardZoneComponent::RemoveFromEquipment(UTKCardBase* Card)
{
	if (Card == nullptr) return false;
	int32 Removed = EquipmentCards.Remove(Card);
	if (Removed > 0)
	{
		Card->ZoneType = ETKCardZone::OutOfGame;
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
		return true;
	}
	return false;
}

void UTKCardZoneComponent::ClearAllZones()
{
	HandCards.Empty();
	EquipmentCards.Empty();
	JudgementCards.Empty();
	UE_LOG(LogTKGame, Log, TEXT("CardZone: All zones cleared"));
}
