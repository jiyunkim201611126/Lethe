// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Lethe/AbilitySystem/EffectTargetTileSelector/TargetSelectionData.h"
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
struct FGameplayAbilityTargetData_TargetSelectionResults : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FTargetSelectionResult> TargetSelectionResults;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	virtual FString ToString() const override
	{
		return TEXT("FGameplayAbilityTargetData_TargetSelectionResults");
	}
};

USTRUCT()
struct FAbilityActivationContext
{
	GENERATED_BODY()

	/** Player는 HandIndex로 사용하고, Enemy는 Priority로 사용하는 변수입니다. */
	int32 Index = INDEX_NONE;

	FGameplayAbilitySpecHandle AbilitySpecHandle;

	/** Enemy가 카드를 사용하는 시점에만 할당 및 사용되며, 시전 직전에 TargetTiles를 갱신하기 위해 사용합니다. */
	FTargetingIntent TargetingIntent;

	FGameplayTag AbilityTag;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilityOwnerASC;

	UPROPERTY()
	TArray<FTargetSelectionResult> TargetSelectionResults;

	UPROPERTY()
	TArray<TWeakObjectPtr<ATile>> PathTiles;

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
