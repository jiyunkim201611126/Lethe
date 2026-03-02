// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "PreviewCoordinatorComponent.generated.h"

struct FGameplayAttribute;
class UAbilitySystemComponent;
class ULetheCardAbility;

USTRUCT()
struct FPreviewContext
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<const AActor*> CurrentTargetActors;

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

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPreviewDataUpdated, const FPreviewData&);

UCLASS()
class LETHE_API UPreviewCoordinatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPreviewCoordinatorComponent();
	
	void StartCalculatingPreviewData(const FPreviewContext& PreviewContext) const;

private:
	void ConvertAttributeToTag(const TMap<FGameplayAttribute, float>& InMap, TMap<FGameplayTag, float>& OutMap) const;

public:
	FOnPreviewDataUpdated OnPreviewDataUpdated;
};
