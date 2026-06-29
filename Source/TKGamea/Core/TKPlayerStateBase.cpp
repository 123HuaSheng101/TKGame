// Fill out your copyright notice in the Description page of Project Settings.

#include "TKPlayerStateBase.h"
#include "TKGamea.h"
#include "TKGameStateBase.h"
#include "TKResponseComponent.h"
#include "TKGameModeBase.h"
#include "Game/TKCardBase.h"
#include "Cards/TKCardZoneComponent.h"
#include "Cards/TKDeckComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

// 延迟获取常用 Tag（避免 Unity Build 中 static 变量跨文件冲突）
namespace { static const FGameplayTag& Tag_Peach() { static FGameplayTag T = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Peach"), false); return T; } }

ATKPlayerStateBase::ATKPlayerStateBase(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	CardZone = CreateDefaultSubobject<UTKCardZoneComponent>("CardZone");
}

void ATKPlayerStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATKPlayerStateBase, SeatIndex);
	DOREPLIFETIME(ATKPlayerStateBase, bAlive);
	DOREPLIFETIME(ATKPlayerStateBase, Identity);
	DOREPLIFETIME(ATKPlayerStateBase, CurrentHealth);
	DOREPLIFETIME(ATKPlayerStateBase, MaxHealth);
	DOREPLIFETIME(ATKPlayerStateBase, HandCardCount);
}

void ATKPlayerStateBase::OnRep_Alive()
{
	UE_LOG(LogTKGame, Log, TEXT("Player [%s] alive status changed to: %s"), *GetPlayerName(), bAlive ? TEXT("Alive") : TEXT("Dead"));
}

void ATKPlayerStateBase::OnRep_Identity()
{
	UE_LOG(LogTKGame, Log, TEXT("Player [%s] identity revealed: %d"), *GetPlayerName(), (uint8)Identity);
}

void ATKPlayerStateBase::OnRep_Health()
{
	UE_LOG(LogTKGame, Log, TEXT("Player [%s] health: %d/%d"), *GetPlayerName(), CurrentHealth, MaxHealth);
}

void ATKPlayerStateBase::SetIdentity(ETKIdentity NewIdentity)
{
	Identity = NewIdentity;
}

bool ATKPlayerStateBase::ApplyDamage(int32 Damage,ATKPlayerStateBase* DamageSource)
{
	if (!bAlive || Damage <= 0)
		return false;
	CurrentHealth = FMath::Max(0, CurrentHealth - Damage);
	LastDamageSource = DamageSource;
	UE_LOG(LogTKGame, Log, TEXT("Player [%s] took %d damage, health now: %d"), *GetPlayerName(), Damage, CurrentHealth);

	if (CurrentHealth <= 0)
	{
		UE_LOG(LogTKGame, Log, TEXT("Player [%s] is dying! Triggering rescue response..."), *GetPlayerName());
		TriggerDyingResponse();
		return true; // dying
	}
	return false;
}

bool ATKPlayerStateBase::Heal(int32 HealAmount)
{
	if (!bAlive || HealAmount <= 0)
		return false;

	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + HealAmount);
	UE_LOG(LogTKGame, Log, TEXT("Player [%s] healed %d, health now: %d"), *GetPlayerName(), HealAmount, CurrentHealth);
	return true;
}

void ATKPlayerStateBase::TriggerDyingResponse()
{
	if (!bAlive) return;

	ATKGameStateBase* TKGS = Cast<ATKGameStateBase>(GetWorld()->GetGameState());
	if (!TKGS) { OnDyingResolved(FResponseResult()); return; }

	UTKResponseComponent* ResponseComp = TKGS->GetResponseComponent();
	if (!ResponseComp) { OnDyingResolved(FResponseResult()); return; }

	FResponseRequest Req;
	Req.Type = ETKResponseType::Sequential;
	Req.RequiredTag = Tag_Peach();  // 需要桃或酒
	Req.SourceCard = nullptr;
	Req.Initiator = nullptr;
	Req.PrimaryTarget = this;

	// 构建逆时针响应对列（从当前回合玩家起，直接用PlayerArray）
	Req.ResponderQueue = UTKResponseComponent::BuildResponderQueue(TKGS->PlayerArray, this, nullptr);

	UE_LOG(LogTKGame, Log, TEXT("Player [%s] dying: requesting rescue from %d players"), *GetPlayerName(), Req.ResponderQueue.Num());
	ResponseComp->RequestResponse(Req, FOnResponseResolved::CreateUObject(this, &ATKPlayerStateBase::OnDyingResolved));
}

void ATKPlayerStateBase::OnDyingResolved(const FResponseResult& Result)
{
	if (Result.bNegated)
	{
		// 有人出桃/酒救活
		CurrentHealth = FMath::Max(1, CurrentHealth); // 至少回1血
		UE_LOG(LogTKGame, Log, TEXT("Player [%s] was rescued from dying by [%s]!"),
			*GetPlayerName(), Result.Responder ? *Result.Responder->GetPlayerName() : TEXT("Unknown"));
	}
	else
	{
		// 无人救 → 死亡
		bAlive = false;
		bIdentityRevealed = true;
		UE_LOG(LogTKGame, Log, TEXT("Player [%s] has died! Identity=%d"), *GetPlayerName(), (uint8)Identity);

		// 弃掉所有手牌和装备到弃牌堆
		ATKGameModeBase* GameMode = Cast<ATKGameModeBase>(GetWorld()->GetAuthGameMode());
		if (CardZone && GameMode)
		{
			UTKDeckComponent* Deck = GameMode->GetDeckComponent();
			CardZone->ClearAndDiscardAll(Deck);
		}

		// ---- 死亡奖惩 ----
		ATKPlayerStateBase* Killer = LastDamageSource.Get();
		if (GameMode && Killer && Killer != this && Killer->IsAlive())
		{
			UTKDeckComponent* Deck = GameMode->GetDeckComponent();
			UTKCardZoneComponent* KillerZone = Killer->GetCardZone();

			if (Deck && KillerZone)
			{
				switch (Identity)
				{
				case ETKIdentity::Rebel:  // 杀反贼摸 3 张牌
				{
					TArray<UTKCardBase*> RewardCards = Deck->DrawMultipleCards(3);
					for (UTKCardBase* Card : RewardCards)
					{
						if (Card) KillerZone->AddCardToHand(Card);
					}
					UE_LOG(LogTKGame, Log, TEXT("Death Reward: [%s] killed Rebel → draws 3 cards"),
						*Killer->GetPlayerName());
					break;
				}

				case ETKIdentity::Loyalist:  // 主公杀忠臣：弃置所有牌
				{
					if (Killer->Identity == ETKIdentity::Lord)
					{
						KillerZone->ClearAndDiscardAll(Deck);
						UE_LOG(LogTKGame, Log, TEXT("Death Penalty: Lord [%s] killed Loyalist → discards all cards"),
							*Killer->GetPlayerName());
					}
					break;
				}

				default: break;
				}
			}
		}

		// 检查胜负
		CheckGameEnd();
	}
}

void ATKPlayerStateBase::CheckGameEnd()
{
	ATKGameStateBase* TKGS = Cast<ATKGameStateBase>(GetWorld()->GetGameState());
	if (!TKGS) return;

	// 统计各阵营存活
	int32 RebelAlive = 0;
	bool bLordDead = false;
	for (const TObjectPtr<APlayerState>& PS : TKGS->PlayerArray)
	{
		ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
		if (!TKPS) continue;
		if (TKPS->IsAlive() && TKPS->Identity == ETKIdentity::Rebel) RebelAlive++;
		if (!TKPS->IsAlive() && TKPS->Identity == ETKIdentity::Lord) bLordDead = true;
	}

	TArray<APlayerState*> Winners;
	ATKGameModeBase* GameMode = Cast<ATKGameModeBase>(GetWorld()->GetAuthGameMode());

	// 主公死亡 → 反贼胜
	if (bLordDead)
	{
		for (const TObjectPtr<APlayerState>& PS : TKGS->PlayerArray)
		{
			ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
			if (TKPS && TKPS->Identity == ETKIdentity::Rebel)
				Winners.Add(PS);
		}
		if (GameMode) GameMode->EndGame(ETKGameResult::RebelWin, Winners);
		return;
	}

	// 反贼全灭 → 主忠胜
	if (RebelAlive == 0)
	{
		for (const TObjectPtr<APlayerState>& PS : TKGS->PlayerArray)
		{
			ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
			if (TKPS && (TKPS->Identity == ETKIdentity::Lord || TKPS->Identity == ETKIdentity::Loyalist))
				Winners.Add(PS);
		}
		if (GameMode) GameMode->EndGame(ETKGameResult::LordLoyalistWin, Winners);
	}
}
