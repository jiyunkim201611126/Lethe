// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "PreviewCoordinatorComponent.generated.h"

struct FGameplayAttribute;
struct FPreviewContext;
struct FPreviewData;
class UAbilitySystemComponent;
class ULetheCardAbility;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnUpdatePreviewData, const FPreviewData&);

UCLASS()
class LETHE_API UPreviewCoordinatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPreviewCoordinatorComponent();
	
	void StartCalculatingPreviewData(const FPreviewContext& PreviewContext) const;
	void StopAllPreview() const;

private:
	void ConvertAttributeToTag(const TMap<FGameplayAttribute, float>& InMap, TMap<FGameplayTag, float>& OutMap) const;

public:
	FOnUpdatePreviewData OnUpdatePreviewData;
};
