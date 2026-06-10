#include "TKIdentityRuleComponent.h"
#include "Core/TKPlayerStateBase.h"
#include "Core/TKGameModeBase.h"
#include "Cards/TKCardZoneComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"

UTKIdentityRuleComponent::UTKIdentityRuleComponent(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

// 三国杀标准身份分配
// 4人: 1主 1忠 1反 1内
// 5人: 1主 1忠 2反 1内
// 6人: 1主 1忠 3反 1内
// 7人: 1主 2忠 3反 1内
// 8人: 1主 2忠 4反 1内
// 9人: 1主 3忠 4反 1内
// 10人: 1主 3忠 5反 1内

int32 UTKIdentityRuleComponent::GetRebelCountForPlayers(int32 TotalPlayers)
{
	switch (TotalPlayers)
	{
	case 4: return 1;
	case 5: return 2;
	case 6: return 3;
	case 7: return 3;
	case 8: return 4;
	case 9: return 4;
	case 10: return 5;
	default: return 1;
	}
}

int32 UTKIdentityRuleComponent::GetLoyalistCountForPlayers(int32 TotalPlayers)
{
	switch (TotalPlayers)
	{
	case 4: return 1;
	case 5: return 1;
	case 6: return 1;
	case 7: return 2;
	case 8: return 2;
	case 9: return 3;
	case 10: return 3;
	default: return 1;
	}
}

ETKIdentity UTKIdentityRuleComponent::DetermineIdentityForPlayer(int32 Index, int32 TotalPlayers)
{
	if (Index == 0) return ETKIdentity::Lord;

	// 第 1 个是主公，后面依次分配
	int32 LoyalistCount = GetLoyalistCountForPlayers(TotalPlayers);
	int32 RebelCount = GetRebelCountForPlayers(TotalPlayers);

	// 从第 1 个（Index=1）开始分配
	// 顺序：忠→反→内，循环填充
	TArray<ETKIdentity> Order;
	for (int32 i = 0; i < LoyalistCount; i++) Order.Add(ETKIdentity::Loyalist);
	for (int32 i = 0; i < RebelCount; i++) Order.Add(ETKIdentity::Rebel);
	Order.Add(ETKIdentity::Renegade);

	// 随机打乱分配顺序以实现随机分配
	int32 RandomOffset = FMath::RandRange(0, Order.Num() - 1);
	return Order[(Index - 1 + RandomOffset) % Order.Num()];
}

void UTKIdentityRuleComponent::AssignIdentities(const TArray<APlayerState*>& Players)
{
	int32 TotalPlayers = Players.Num();
	if (TotalPlayers < 4 || TotalPlayers > 10) return;

	UE_LOG(LogTemp, Log, TEXT("=== Assigning Identities for %d players ==="), TotalPlayers);

	// 先分配主公（第一个玩家）
	for (int32 i = 0; i < TotalPlayers; i++)
	{
		ATKPlayerStateBase* TKPlayer = Cast<ATKPlayerStateBase>(Players[i]);
		if (TKPlayer == nullptr) continue;

		ETKIdentity Identity = DetermineIdentityForPlayer(i, TotalPlayers);
		TKPlayer->SetIdentity(Identity);

		UE_LOG(LogTemp, Log, TEXT("  Player [%s] -> Seat %d -> Identity: %d"),
			*TKPlayer->GetPlayerName(), i, (uint8)Identity);
	}
}

ETKGameResult UTKIdentityRuleComponent::EvaluateVictory() const
{
	// 安全获取 GameState
	AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const TArray<TObjectPtr<APlayerState>>& PlayerArray = GS ? GS->PlayerArray : TArray<TObjectPtr<APlayerState>>();
	TArray<ATKPlayerStateBase*> AlivePlayers = GetAlivePlayers(PlayerArray);

	bool bLordAlive = false;
	bool bLoyalistAlive = false;
	bool bRebelAlive = false;
	bool bRenegadeAlive = false;

	for (const ATKPlayerStateBase* Player : AlivePlayers)
	{
		if (!Player || !Player->IsAlive()) continue;
		switch (Player->Identity)
		{
		case ETKIdentity::Lord:		bLordAlive = true; break;
		case ETKIdentity::Loyalist:	bLoyalistAlive = true; break;
		case ETKIdentity::Rebel:	bRebelAlive = true; break;
		case ETKIdentity::Renegade:	bRenegadeAlive = true; break;
		case ETKIdentity::Unknown:	/* 不应出现 */	break;
		}
	}

	// 主公死亡
	if (!bLordAlive)
	{
		// 内奸单独存活 -> 内奸胜利
		if (bRenegadeAlive && !bLoyalistAlive && !bRebelAlive)
			return ETKGameResult::RenegadeWin;
		// 有反贼存活 -> 反贼胜利
		if (bRebelAlive)
			return ETKGameResult::RebelWin;
	}

	// 反贼和内奸全部死亡 -> 主忠胜利
	if (!bRebelAlive && !bRenegadeAlive && bLordAlive)
		return ETKGameResult::LordLoyalistWin;

	return ETKGameResult::None;
}

void UTKIdentityRuleComponent::ResolveDeath(ATKPlayerStateBase* DeadPlayer)
{
	if (DeadPlayer == nullptr || DeadPlayer->IsAlive()) return;

	UE_LOG(LogTemp, Log, TEXT("Player [%s] died, resolving..."), *DeadPlayer->GetPlayerName());

	// 清空所有牌区
	UTKCardZoneComponent* CardZone = DeadPlayer->FindComponentByClass<UTKCardZoneComponent>();
	if (CardZone)
	{
		// 将手牌放入弃牌堆（通过 GameMode 访问 DeckComponent）
		CardZone->ClearAllZones();
	}

	// 胜负判定
	ETKGameResult Result = EvaluateVictory();
	if (Result != ETKGameResult::None)
	{
		// 收集胜利玩家
		UWorld* World = GetWorld();
		AGameStateBase* GS = World ? World->GetGameState() : nullptr;
		if (GS == nullptr) return;

		TArray<APlayerState*> Winners;
		for (APlayerState* PS : GS->PlayerArray)
		{
			ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
			if (TKPS == nullptr || !TKPS->IsAlive()) continue;

			bool bIsWinner = false;
			switch (Result)
			{
			case ETKGameResult::LordLoyalistWin:
				bIsWinner = (TKPS->Identity == ETKIdentity::Lord || TKPS->Identity == ETKIdentity::Loyalist);
				break;
			case ETKGameResult::RebelWin:
				bIsWinner = (TKPS->Identity == ETKIdentity::Rebel);
				break;
			case ETKGameResult::RenegadeWin:
				bIsWinner = (TKPS->Identity == ETKIdentity::Renegade);
				break;
			}
			if (bIsWinner)
			{
				Winners.Add(TKPS);
			}
		}

		// 通知 GameMode 结束游戏
		ATKGameModeBase* GameMode = World ? World->GetAuthGameMode<ATKGameModeBase>() : nullptr;
		if (GameMode)
		{
			GameMode->EndGame(Result, Winners);
		}
	}
}

// ===== 统计工具 =====

int32 UTKIdentityRuleComponent::GetLordCount(const TArray<ATKPlayerStateBase*>& Players)
{
	int32 Count = 0;
	for (const auto& P : Players) { if (P && P->Identity == ETKIdentity::Lord) Count++; }
	return Count;
}

int32 UTKIdentityRuleComponent::GetLoyalistCount(const TArray<ATKPlayerStateBase*>& Players)
{
	int32 Count = 0;
	for (const auto& P : Players) { if (P && P->Identity == ETKIdentity::Loyalist) Count++; }
	return Count;
}

int32 UTKIdentityRuleComponent::GetRebelCount(const TArray<ATKPlayerStateBase*>& Players)
{
	int32 Count = 0;
	for (const auto& P : Players) { if (P && P->Identity == ETKIdentity::Rebel) Count++; }
	return Count;
}

int32 UTKIdentityRuleComponent::GetRenegadeCount(const TArray<ATKPlayerStateBase*>& Players)
{
	int32 Count = 0;
	for (const auto& P : Players) { if (P && P->Identity == ETKIdentity::Renegade) Count++; }
	return Count;
}

int32 UTKIdentityRuleComponent::GetAliveCount(const TArray<ATKPlayerStateBase*>& Players)
{
	int32 Count = 0;
	for (const auto& P : Players) { if (P && P->IsAlive()) Count++; }
	return Count;
}

TArray<ATKPlayerStateBase*> UTKIdentityRuleComponent::GetAlivePlayers(const TArray<TObjectPtr<APlayerState>>& Players)
{
	TArray<ATKPlayerStateBase*> Alive;
	for (const auto& PS : Players)
	{
		ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
		if (TKPS && TKPS->IsAlive())
		{
			Alive.Add(TKPS);
		}
	}
	return Alive;
}
