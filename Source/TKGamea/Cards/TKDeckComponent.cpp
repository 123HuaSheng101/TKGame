#include "TKDeckComponent.h"
#include "TKGamea.h"
#include "Game/TKCardBase.h"
#include "Game/TKCard_Basic.h"
#include "Game/TKCard_Trick.h"
#include "Game/TKCard_DelayedTrick.h"
#include "Game/TKCard_Equipment.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"

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
	// 根据 ETKCardType 选择对应子类
	TSubclassOf<UTKCardBase> CardClass;
	switch (Entry.CardType)
	{
	case ETKCardType::Basic:         CardClass = UTKCard_Basic::StaticClass();         break;
	case ETKCardType::Trick:         CardClass = UTKCard_Trick::StaticClass();          break;
	case ETKCardType::DelayedTrick:  CardClass = UTKCard_DelayedTrick::StaticClass();  break;
	case ETKCardType::Equipment:     CardClass = UTKCard_Equipment::StaticClass();      break;
	default:                         CardClass = UTKCard_Basic::StaticClass();          break;
	}

	UTKCardBase* Card = NewObject<UTKCardBase>(this, CardClass);
	if (Card)
	{
		Card->CardDefId  = Entry.CardDefId;
		Card->CardName   = Entry.CardName;
		Card->CardType   = Entry.CardType;
		Card->Suit       = Entry.Suit;
		Card->Rank       = Entry.Rank;
		Card->ZoneType   = ETKCardZone::Deck;
		Card->EffectTags = Entry.EffectTags;

		NextInstanceId++;
	}
	return Card;
}

void UTKDeckComponent::InitFromDataTable(UDataTable* DataTable)
{
	if (!DataTable) return;

	DeckCards.Empty();
	DiscardPile.Empty();
	NextInstanceId = 0;

	static const FString ContextStr(TEXT("TKDeckComponent::InitFromDataTable"));
	TArray<FTKCardDef*> Rows;
	DataTable->GetAllRows<FTKCardDef>(ContextStr, Rows);

	for (const FTKCardDef* Row : Rows)
	{
		if (!Row) continue;
		for (int32 i = 0; i < Row->Count; i++)
		{
			// 构造临时 FTKDebugCardEntry 复用工厂方法
			FTKDebugCardEntry Entry;
			Entry.CardDefId  = Row->CardDefId;
			Entry.CardName   = Row->Name;
			Entry.CardType   = Row->CardType;
			Entry.Suit       = Row->Suit;
			Entry.Rank       = Row->Rank;
			Entry.EffectTags = Row->EffectTags;
			Entry.Count      = 1;

			UTKCardBase* Card = CreateCardInstance(Entry);
			if (Card) DeckCards.Add(Card);
		}
	}

	Shuffle();
	UE_LOG(LogTKGame, Log, TEXT("Deck initialized from DataTable with %d cards"), DeckCards.Num());
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
	UE_LOG(LogTKGame, Log, TEXT("Deck initialized with %d cards, shuffled"), DeckCards.Num());
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
			UE_LOG(LogTKGame, Log, TEXT("Deck reshuffled from discard pile, %d cards"), DeckCards.Num());
		}
		else
		{
			UE_LOG(LogTKGame, Warning, TEXT("Deck is empty, cannot draw!"));
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
	UE_LOG(LogTKGame, Log, TEXT("Drew %d cards from deck"), Drawn.Num());
	return Drawn;
}

void UTKDeckComponent::DiscardCard(UTKCardBase* Card)
{
	if (Card == nullptr) return;
	Card->ZoneType = ETKCardZone::DiscardPile;
	DiscardPile.Add(Card);
	UE_LOG(LogTKGame, Log, TEXT("Card [%s] discarded to pile"), *Card->CardDefId.ToString());
}

// ===== 判定系统 =====

bool UTKDeckComponent::ExecuteJudgement(const FGameplayTag& JudgeTag, ETKCardSuit& OutSuit, int32& OutRank, UTKCardBase*& OutCard)
{
	OutCard = DrawCard();
	if (!OutCard)
	{
		UE_LOG(LogTKGame, Warning, TEXT("ExecuteJudgement: DrawCard returned null for tag [%s]"), *JudgeTag.ToString());
		return false;
	}

	OutSuit = OutCard->Suit;
	OutRank = OutCard->Rank;

	// 判定牌进入弃牌堆
	DiscardCard(OutCard);

	bool bEffective = IsJudgementEffective(JudgeTag, OutSuit, OutRank);
	UE_LOG(LogTKGame, Log, TEXT("ExecuteJudgement: tag=[%s] card=[%s] suit=%d rank=%d -> %s"),
		*JudgeTag.ToString(), *OutCard->CardDefId.ToString(), (uint8)OutSuit, OutRank,
		bEffective ? TEXT("EFFECTIVE") : TEXT("PASSED"));
	return bEffective;
}

bool UTKDeckComponent::IsJudgementEffective(const FGameplayTag& JudgeTag, ETKCardSuit Suit, int32 Rank)
{
	static FGameplayTag Tag_SkipPlay  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Skip.Play"),  false);
	static FGameplayTag Tag_SkipDraw  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Skip.Draw"),  false);
	static FGameplayTag Tag_Lightning = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Lightning"),  false);

	if (JudgeTag == Tag_SkipPlay)
	{
		// 乐不思蜀：非红桃生效
		return Suit != ETKCardSuit::Heart;
	}
	if (JudgeTag == Tag_SkipDraw)
	{
		// 兵粮寸断：非梅花生效
		return Suit != ETKCardSuit::Club;
	}
	if (JudgeTag == Tag_Lightning)
	{
		// 闪电：黑桃 2~9 生效
		return Suit == ETKCardSuit::Spade && Rank >= 2 && Rank <= 9;
	}
	return false;
}
