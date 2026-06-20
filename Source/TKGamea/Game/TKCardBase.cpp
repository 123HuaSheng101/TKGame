#include "TKGamea.h"
// Fill out your copyright notice in the Description page of Project Settings.

#include "TKCardBase.h"
#include "Core/TKPlayerControllerBase.h"
#include "Core/TKPlayerStateBase.h"
#include "Core/TKGameStateBase.h"
#include "Core/TKTurnComponentBase.h"
#include "Core/TKEventComponentBase.h"
#include "Core/TKResponseComponent.h"
#include "TKGameTypes.h"

UTKCardBase::UTKCardBase()
{
}

// ===== 模板方法 =====

bool UTKCardBase::Use(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target)
{
	if (!User || !Target)
	{
		UE_LOG(LogTKGame, Warning, TEXT("UTKCardBase::Use - Invalid User or Target"));
		return false;
	}

	// 1. 前置校验
	if (!PreUseCheck(User))
	{
		UE_LOG(LogTKGame, Warning, TEXT("UTKCardBase::Use - PreUseCheck failed for card [%s]"), *CardDefId.ToString());
		return false;
	}

	// 2. 目标合法性校验
	if (!CanUse(User, Target))
	{
		UE_LOG(LogTKGame, Warning, TEXT("UTKCardBase::Use - CanUse failed for card [%s] on target [%s]"),
			*CardDefId.ToString(), *Target->GetPlayerName());
		return false;
	}

	// 3. 检查是否需要等待响应（杀需要目标出闪）
	if (NeedsResponse())
	{
		ATKGameStateBase* TKGS = Cast<ATKGameStateBase>(User->GetWorld()->GetGameState());
		if (!TKGS)
		{
			UE_LOG(LogTKGame, Error, TEXT("UTKCardBase::Use - GameState not found for response"));
			return false;
		}

		UTKResponseComponent* ResponseComp = TKGS->GetResponseComponent();
		if (!ResponseComp)
		{
			UE_LOG(LogTKGame, Error, TEXT("UTKCardBase::Use - ResponseComponent not found"));
			return false;
		}

		// 暂存 User/Target
		PendingUser = User;
		PendingTarget = Target;

		// 构建响应请求
		FResponseRequest Req;
		Req.Type = ETKResponseType::SingleTarget;
		Req.RequiredTag = GetResponseRequiredTag();
		Req.SourceCard = this;
		Req.Initiator = User->PlayerState;
		Req.PrimaryTarget = Target;

		// 发起异步请求，绑定回调
		ResponseComp->RequestResponse(Req, FOnResponseResolved::CreateUObject(this, &UTKCardBase::ContinueUseAfterResponse));

		UE_LOG(LogTKGame, Log, TEXT("UTKCardBase::Use - Card [%s] waiting for response (tag=[%s])"),
			*CardDefId.ToString(), *GetResponseRequiredTag().GetTagName().ToString());
		return true;
	}

	// 4. 不需要响应 → 直接同步结算
	OnUse(User, Target);
	OnAfterUse(User, Target);

	UE_LOG(LogTKGame, Log, TEXT("UTKCardBase::Use - Card [%s] used by [%s] on [%s]"),
		*CardDefId.ToString(), *User->GetName(), *Target->GetPlayerName());
	return true;
}

void UTKCardBase::ContinueUseAfterResponse(const FResponseResult& Result)
{
	ATKPlayerControllerBase* User = PendingUser.Get();
	ATKPlayerStateBase* Target = PendingTarget.Get();

	if (!User || !Target)
	{
		UE_LOG(LogTKGame, Error, TEXT("UTKCardBase::ContinueUseAfterResponse - Pending User/Target is null"));
		return;
	}

	UE_LOG(LogTKGame, Log, TEXT("UTKCardBase::ContinueUseAfterResponse - Card [%s], result=%d, bNegated=%d"),
		*CardDefId.ToString(), (uint8)Result.Result, Result.bNegated);

	// 如果没被抵消（没人出闪/无懈），执行卡牌效果
	if (!Result.bNegated)
	{
		OnUse(User, Target);
	}
	else
	{
		UE_LOG(LogTKGame, Log, TEXT("UTKCardBase::ContinueUseAfterResponse - Card [%s] negated by response"), *CardDefId.ToString());
	}

	// 后置处理（无论是否被抵消，主动牌都进弃牌堆）
	OnAfterUse(User, Target);

	PendingUser = nullptr;
	PendingTarget = nullptr;
}

bool UTKCardBase::PreUseCheck(ATKPlayerControllerBase* User) const
{
	if (!User) return false;

	// 获取 GameState 和回合组件
	ATKPlayerStateBase* TKPS = Cast<ATKPlayerStateBase>(User->PlayerState);
	if (!TKPS || !TKPS->IsAlive()) return false;

	ATKGameStateBase* TKGS = Cast<ATKGameStateBase>(User->GetWorld()->GetGameState());
	if (!TKGS) return false;

	UTKTurnComponentBase* TurnComp = TKGS->GetTurnComponent();
	if (!TurnComp) return false;

	// 校验：是否为自己的回合（主动出牌需要）
	APlayerState* CurrentTurnPlayer = TurnComp->GetCurrentPlayer();
	ETKTurnPhase CurrentPhase = TurnComp->GetCurrentPhase();

	// 基本牌/锦囊/装备：需要在出牌阶段且是自己的回合
	if (CardType == ETKCardType::Basic || CardType == ETKCardType::Trick || CardType == ETKCardType::Equipment)
	{
		// 仅能在出牌阶段使用
		if (CurrentPhase != ETKTurnPhase::Play)
		{
			UE_LOG(LogTKGame, Warning, TEXT("PreUseCheck: Not in Play phase"));
			return false;
		}
		// 仅能自己回合主动使用
		if (CurrentTurnPlayer != User->PlayerState)
		{
			UE_LOG(LogTKGame, Warning, TEXT("PreUseCheck: Not user's turn"));
			return false;
		}
	}

	// TODO: 闪/桃/无懈的响应时机不受此限制，由 ResponseComponent 单独处理

	return true;
}

bool UTKCardBase::CanUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) const
{
	if (!User || !Target) return false;

	if (!Target->IsAlive()) return false;

	return true;
}

void UTKCardBase::OnAfterUse(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target)
{
	// 广播事件
	FEventContext Ctx = BuildUseEvent(User, Target);
	UTKEventComponentBase* EventComp = UTKEventComponentBase::Get(User);
	if (EventComp)
	{
		EventComp->PostEvent(Ctx);
	}

	// 默认：非装备牌使用后进入弃牌堆
	if (CardType != ETKCardType::Equipment && CardType != ETKCardType::DelayedTrick)
	{
		UE_LOG(LogTKGame, Log, TEXT("UTKCardBase::OnAfterUse - Card [%s] will be discarded"), *CardDefId.ToString());
	}
}

FEventContext UTKCardBase::BuildUseEvent(ATKPlayerControllerBase* User, ATKPlayerStateBase* Target) const
{
	FEventContext Ctx;
	Ctx.EventTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Use"), false);
	Ctx.EventInitiator = User;

	TArray<TObjectPtr<ATKPlayerControllerBase>> TargetList;
	if (Target)
	{
		APlayerController* PC = Target->GetPlayerController();
		if (ATKPlayerControllerBase* TKPC = Cast<ATKPlayerControllerBase>(PC))
		{
			TargetList.Add(TKPC);
		}
	}
	Ctx.Targets = TargetList;
	Ctx.Params_g = EffectTags;

	return Ctx;
}