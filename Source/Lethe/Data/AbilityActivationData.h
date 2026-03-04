// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "AbilityActivationData.generated.h"

USTRUCT()
struct FAbilityActivationData
{
	GENERATED_BODY()

	// HandIndex는 Player 전용 변수입니다.
	int32 HandIndex = INDEX_NONE;
	
	FGameplayAbilitySpecHandle AbilitySpecHandle;
	
	FGameplayTag AbilityTag;

	UPROPERTY()
	FGameplayEventData Payload;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilityOwnerASC;
};
