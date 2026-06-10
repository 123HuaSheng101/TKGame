#include "TKDeckComponent.h"
#include "Game/TKCardBase.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

UTKDeckComponent::UTKDeckComponent(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UTKDeckComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTKDeckComponent, DeckCards);
	DOREPLIFETIME(UTKDeckComponent, DiscardPile);
}

UTKCardBase* UTKDeckComponent::CreateCardInstance(const FTKDebugCardEntry& Entry)
{
	UTKCardBase* Card = NewObject<UTKCardBase>(GetWorld() ? GetWorld()->GetOuter() : GetOuter());
	if (Card)
	{
		Card->CardDefId = Entry.CardDefId;
		Card->CardType = Entry.CardType;
		Card->Suit = Entry.Suit;
		Card->Rank = Entry.Rank;
		Card->ZoneType = ETKCardZone::Deck;
		NextInstanceId++;
	}
	return Card;
}

void UTKDeckComponent::InitDebugDeck(const TArray<FTKDebugCardEntry>& DebugCards)
{
	DeckCards.Empty();
	DiscardPile.Empty();
	NextInstanceId = 0;

	for (const FTKDebugCardEntry& Entry : DebugCards)
	{
		for (int32 i = 0; i < Entry.Count; i++)
		{
			UTKCardBase* Card = CreateCardInstance(Entry);
			if (Card)
			{
				DeckCards.Add(Card);
			}
		}
	}

	Shuffle();
	UE_LOG(LogTemp, Log, TEXT("Deck initialized with %d cards, shuffled"), DeckCards.Num());
}

void UTKDeckComponent::Shuffle()
{
	if (DeckCards.Num() <= 1)
		return;

	// Fisher-Yates 洗牌
	int32 LastIndex = DeckCards.Num() - 1;
	for (int32 i = 0; i <= LastIndex; i++)
	{
		int32 SwapIndex = FMath::RandRange(i, LastIndex);
		DeckCards.Swap(i, SwapIndex);
	}
}

UTKCardBase* UTKDeckComponent::DrawCard()
{
	if (DeckCards.Num() == 0)
	{
		// 牌堆为空时，将弃牌堆洗回牌堆
		if (DiscardPile.Num() > 0)
		{
			DeckCards = DiscardPile;
			DiscardPile.Empty();
			Shuffle();
			UE_LOG(LogTemp, Log, TEXT("Deck reshuffled from discard pile, %d cards"), DeckCards.Num());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Deck is empty, cannot draw!"));
			return nullptr;
		}
	}

	UTKCardBase* Card = DeckCards.Pop();
	if (Card)
	{
		Card->ZoneType = ETKCardZone::Hand;
	}
	return Card;
}

TArray<UTKCardBase*> UTKDeckComponent::DrawMultipleCards(int32 Count)
{
	TArray<UTKCardBase*> Drawn;
	for (int32 i = 0; i < Count; i++)
	{
		UTKCardBase* Card = DrawCard();
		if (Card)
		{
			Drawn.Add(Card);
		}
		else
		{
			break;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Drew %d cards from deck"), Drawn.Num());
	return Drawn;
}

void UTKDeckComponent::DiscardCard(UTKCardBase* Card)
{
	if (Card == nullptr) return;
	Card->ZoneType = ETKCardZone::DiscardPile;
	DiscardPile.Add(Card);
	UE_LOG(LogTemp, Log, TEXT("Card [%s] discarded to pile"), *Card->CardDefId.ToString());
}
