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
	const AActor* LastTargetActor = nullptr;

	UPROPERTY()
	const AActor* CurrentTargetActor = nullptr;

	UPROPERTY()
	UAbilitySystemComponent* SourceASC = nullptr;

	UPROPERTY()
	const ULetheCardAbility* SelectedCardAbility = nullptr;
};

USTRUCT()
struct FPreviewData
{
	GENERATED_BODY()

	TMap<FGameplayTag, float> OutPreviewDataForSourceActor;
	TMap<FGameplayTag, float> OutPreviewDataForTargetActor;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPreviewDataUpdated, const FPreviewContext&, const FPreviewData&);

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
