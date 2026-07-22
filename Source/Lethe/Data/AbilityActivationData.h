// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Lethe/AbilitySystem/EffectTargetTileSelector/TargetTileSelectData.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "AbilityActivationData.generated.h"

UCLASS(BlueprintType)
class LETHE_API UMoveAbilityPayload : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<ATile>> PathTiles;
};

USTRUCT()
struct FGameplayAbilityTargetData_TargetSelectResults : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FTargetSelectResult> TargetSelectResults;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	virtual FString ToString() const override
	{
		return TEXT("FGameplayAbilityTargetData_TargetSelectResults");
	}
};

USTRUCT()
struct FAbilityActivationData
{
	GENERATED_BODY()

	/** Player는 HandIndex로 사용하고, Enemy는 Priority로 사용하는 변수입니다. */
	int32 Index = INDEX_NONE;

	FGameplayAbilitySpecHandle AbilitySpecHandle;

	FGameplayTag AbilityTag;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilityOwnerASC;

	UPROPERTY()
	FTargetingIntent TargetingIntent;

	UPROPERTY()
	TArray<FTargetSelectResult> TargetSelectResults;

	UPROPERTY()
	TArray<TWeakObjectPtr<ATile>> PathTiles;

	/** 소음이 발생할 TargetTile입니다. */
	UPROPERTY()
	TWeakObjectPtr<ATile> NoiseTile;

	UPROPERTY()
	FGameplayEventData Payload;
};

UENUM(BlueprintType)
enum class ETeamSide : uint8
{
	None,
	Player,
	Enemy
};
