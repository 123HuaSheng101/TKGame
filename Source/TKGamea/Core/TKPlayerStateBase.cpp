#include "TKGamea.h"
// Fill out your copyright notice in the Description page of Project Settings.

#include "TKPlayerStateBase.h"
#include "Cards/TKCardZoneComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

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
		bAlive = false;
		UE_LOG(LogTKGame, Log, TEXT("Player [%s] has died!"), *GetPlayerName());
		return true; // died
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
