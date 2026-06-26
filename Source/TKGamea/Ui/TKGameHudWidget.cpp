// TKGameHudWidget.cpp

#include "TKGameHudWidget.h"
#include "Core/TKGameHUD.h"
#include "Game/TKCardBase.h"

void UTKGameHudWidget::RefreshDisplay(const FTKGameUIData& GameData, const TArray<FTKPlayerUIData>& PlayerData)
{
	OnGameDataUpdated(GameData, PlayerData);
}

void UTKGameHudWidget::RefreshHandCards()
{
	if (!OwningHUD) return;
	TArray<UTKCardBase*> Hand = OwningHUD->GetMyHandCards();
	OnHandCardsUpdated(Hand);
}

void UTKGameHudWidget::ShowResponsePanel()
{
	OnResponseShown(FTKResponseUIData());
}

void UTKGameHudWidget::HideResponsePanel()
{
	OnResponseHidden();
}

void UTKGameHudWidget::SetResponseUIData(const FTKResponseUIData& Data)
{
	OnResponseShown(Data);
}
