// Fill out your copyright notice in the Description page of Project Settings.

#include "TKCardBase.h"

UTKCardBase::UTKCardBase()
{
}

void UTKCardBase::ActiveCard()
{
	// M1 阶段暂为空实现，后续 M2 补全完整使用效果
	UE_LOG(LogTemp, Log, TEXT("UTKCardBase::ActiveCard called for card [%s]"), *CardDefId.ToString());
}

