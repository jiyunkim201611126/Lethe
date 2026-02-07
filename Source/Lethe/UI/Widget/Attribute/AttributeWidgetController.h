// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Lethe/UI/Widget/LetheWidgetController.h"
#include "AttributeWidgetController.generated.h"

struct FGameplayAttribute;
struct FOnAttributeChangeData;

USTRUCT()
struct FAttributeData
{
	GENERATED_BODY()

	bool bIsPreview = false;

	float CurrentValue = 0.f;
	float MaxValue = 0.f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAttributeChanged, const FAttributeData&);

UCLASS(Abstract, Blueprintable)
class LETHE_API UAttributeWidgetController : public ULetheWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin LetheWidgetController Interface
	virtual void SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams) override;
	virtual void BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS) override;
	//~ End LetheWidgetController Interface

protected:
	void UpdateCachedAttribute(const FOnAttributeChangeData& AttributeData);
	void UpdateCachedPreviewAttribute(const FGameplayAttribute& Attribute, const float NewValue);

	void StartPreview(const FGameplayTag& CurrentTag, const FGameplayTag& MaxTag);
	void StopPreview(const FGameplayTag& CurrentTag, const FGameplayTag& MaxTag);
	
	virtual void StartAllPreview();
	virtual void StopAllPreview();

private:
	void OnHealthChanged(const FOnAttributeChangeData& AttributeData);
	void BroadcastHealthChanged() const;

public:
	TMap<FGameplayTag, FOnAttributeChanged> OnAttributeChangedMap;
	TMap<FGameplayTag, FOnAttributeChanged> OnPreviewAttributeChangedMap;
	TMap<FGameplayTag, FOnAttributeChanged> OnPreviewEndedMap;

protected:
	TMap<FGameplayTag, float> CachedAttribute;
	TMap<FGameplayTag, float> CachedPreviewAttribute;
};
