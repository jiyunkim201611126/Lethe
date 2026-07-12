// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
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

/** TargetTileSelector의 동작 결과 데이터입니다. */
USTRUCT()
struct FTargetTileResult
{
	GENERATED_BODY()
	
	FGameplayTag TargetGroupTag;
	TArray<TWeakObjectPtr<ATile>> TargetTiles;
};

/** 위 TargetTileResult를 Ability가 사용 가능한 형태로 바꾼 데이터입니다. */
USTRUCT()
struct FTargetActorResult
{
	GENERATED_BODY()

	FGameplayTag TargetGroupTag;
	TArray<TWeakObjectPtr<AActor>> TargetActors;
};

USTRUCT()
struct FGameplayAbilityTargetData_TargetActorResults : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FTargetActorResult> TargetActorResults;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	virtual FString ToString() const override
	{
		return TEXT("FGameplayAbilityTargetData_TargetActorResults");
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
	TArray<FTargetTileResult> TargetTileResults;

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
