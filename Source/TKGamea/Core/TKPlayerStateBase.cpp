#include "TKGamea.h"
// Fill out your copyright notice in the Description page of Project Settings.

#include "TKPlayerStateBase.h"
#include "TKGameStateBase.h"
#include "TKResponseComponent.h"
#include "Game/TKCardBase.h"
#include "Cards/TKCardZoneComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

static FGameplayTag Tag_Card_Peach = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Peach"), false);
static FGameplayTag Tag_Card_Wine  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Wine"),  false);

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

bool ATKPlayerStateBase::ApplyDamage(int32 Damage)
{
	if (!bAlive || Damage <= 0)
		return false;

	CurrentHealth = FMath::Max(0, CurrentHealth - Damage);
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
	Req.RequiredTag = Tag_Card_Peach;  // 需要桃或酒
	Req.SourceCard = nullptr;
	Req.Initiator = nullptr;
	Req.PrimaryTarget = this;

	// 构建逆时针响应对列（从当前回合玩家起）
	TArray<APlayerState*> AllPlayers;
	for (const TObjectPtr<APlayerState>& PS : TKGS->PlayerArray)
	{
		ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(PS);
		if (TKPS && TKPS->IsAlive())
			AllPlayers.Add(PS);
	}
	Req.ResponderQueue = UTKResponseComponent::BuildResponderQueue(AllPlayers, this, nullptr);

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
		UE_LOG(LogTKGame, Log, TEXT("Player [%s] has died! No one responded."), *GetPlayerName());
		// TODO: 死亡结算（弃牌、身份翻牌、胜负判定）
	}
}
