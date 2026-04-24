// Fill out your copyright notice in the Description page of Project Settings.

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
	TArray<TWeakObjectPtr<AActor>> CurrentTargetActors;

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
