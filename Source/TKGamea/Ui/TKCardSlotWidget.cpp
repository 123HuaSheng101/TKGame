// TKCardSlotWidget.cpp

#include "TKCardSlotWidget.h"
#include "Game/TKCardBase.h"

void UTKCardSlotWidget::SetCardData(UTKCardBase* Card, int32 Index)
{
	CardData = Card;
	CardIndex = Index;
	OnCardDataUpdated(Card);
}
