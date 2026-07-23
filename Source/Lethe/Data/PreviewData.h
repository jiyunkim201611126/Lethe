// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Ability/LetheCardAbility.h"
#include "PreviewData.generated.h"

USTRUCT()
struct FPreviewContext
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FTargetSelectionResult> TargetSelectionResults;

	UPROPERTY()
	UAbilitySystemComponent* SourceASC = nullptr;

	UPROPERTY()
	const ULetheCardAbility* SelectedCardAbility = nullptr;
};

USTRUCT()
struct FAttributePreviewDelta
{
	GENERATED_BODY()
	
	TMap<FGameplayTag, float> AttributePreviewDelta;
};

USTRUCT()
struct FPreviewData
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<UAbilitySystemComponent*, FAttributePreviewDelta> ASCToPreviewData;
};
