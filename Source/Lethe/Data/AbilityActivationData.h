// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "AbilityActivationData.generated.h"

USTRUCT()
struct FAbilityActivationData
{
	GENERATED_BODY()

	// Player는 HandIndex로 사용하고, Enemy는 Priority로 사용하는 변수입니다.
	int32 Index = INDEX_NONE;
	
	FGameplayAbilitySpecHandle AbilitySpecHandle;
	
	FGameplayTag AbilityTag;

	UPROPERTY()
	FGameplayEventData Payload;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilityOwnerASC;

	UPROPERTY()
	TWeakObjectPtr<ATile> TargetTile;
};

UENUM(BlueprintType)
enum class ETeamSide : uint8
{
	None,
	Player,
	Enemy
};
