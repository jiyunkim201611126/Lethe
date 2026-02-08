// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Lethe/UI/Widget/LetheWidgetController.h"
#include "AttributeWidgetController.generated.h"

class ULetheGameplayAbility;
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

/**
 * AttributeSet을 관찰하며 AttributeWidget에게 값을 전달하는 역할의 VM입니다.
 * 이벤트 발생 시 Actual Value 혹은 Preview Value를 표시합니다.
 * 
 * AttributeWidget에 있는 ProgressBar와 Text의 특성상 CurrentValue와 MaxValue를 모두 요구합니다.
 * 때문에 예를 들어 Health만 Update되는 상황에도 MaxHealth를 함께 보내주어야 합니다.
 * 이로 인해 Attribute Value들을 캐싱해둘 필요가 있었고, 이를 직접 선언해 1:1로 대응하기엔 코드 양이 많아져 TMap을 활용했습니다.
 */
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
	
	void OnOtherTileDetected(const AActor* LastActor, const AActor* CurrentActor, const UAbilitySystemComponent* SourceASC, const ULetheGameplayAbility* CardAbility);

public:
	// AttributeTag를 Key로, AttributeWidget이 콜백을 걸어두는 델리게이트를 Value로 하는 TMap들입니다.
	TMap<FGameplayTag, FOnAttributeChanged> OnAttributeChangedMap;
	TMap<FGameplayTag, FOnAttributeChanged> OnPreviewAttributeChangedMap;
	TMap<FGameplayTag, FOnAttributeChanged> OnPreviewEndedMap;

protected:
	// 실제 AttributeSet이 갖고 있는 Attribute Value를 캐싱해두는 TMap입니다.
	TMap<FGameplayTag, float> CachedAttribute;
	// Ability가 갖고 있는 Effect가 적용될 경우 Attribute에 어떤 변화가 일어나는지 그 값을 캐싱해두는 TMap입니다.
	TMap<FGameplayTag, float> CachedAbilityEffectPreviewData;
};
