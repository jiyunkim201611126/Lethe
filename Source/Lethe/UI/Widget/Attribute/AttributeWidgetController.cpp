// Copyright JETBLU, Inc. All Rights Reserved.

#include "AttributeWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Manager/LetheGameplayTags.h"

void UAttributeWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams)
{
	PlayerController = WidgetControllerParams.PlayerController;
	PlayerState = WidgetControllerParams.PlayerState;

	// AttributeWidget과 1:1 대응되는 WidgetController이므로, 1개만 있으면 됩니다.
	// 부모 클래스가 PlayerCharacter의 용도로도 사용될 수 있도록 Array로 선언했기 때문에 이처럼 구현합니다.
	AbilitySystemReferences.Reserve(1);
	ULetheAbilitySystemComponent* AbilitySystemComponent = Cast<ULetheAbilitySystemComponent>(WidgetControllerParams.AbilitySystemComponent);
	ULetheAttributeSet* AttributeSet = Cast<ULetheAttributeSet>(WidgetControllerParams.AttributeSet);
	
	FAbilitySystemReference AbilitySystemReference;
	AbilitySystemReference.AbilitySystemComponent = AbilitySystemComponent;
	AbilitySystemReference.AttributeSet = AttributeSet;
	AbilitySystemReferences.Emplace(AbilitySystemReference);
}

void UAttributeWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMaxHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void UAttributeWidgetController::OnHealthChanged(const FOnAttributeChangeData& AttributeData)
{
	UpdateCachedAttribute(AttributeData);
	BroadcastHealthChanged();
}

void UAttributeWidgetController::BroadcastHealthChanged() const
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	FAttributeData Data;
	Data.bIsPreview = false;
	Data.CurrentValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_Health);
	Data.MaxValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_MaxHealth);
	if (const FOnAttributeChanged* OnHealthChanged = OnAttributeChangedMap.Find(LetheGameplayTags.Attributes_Vital_Health))
	{
		OnHealthChanged->Broadcast(Data);
	}
}

void UAttributeWidgetController::UpdateCachedAttribute(const FOnAttributeChangeData& AttributeData)
{
	if (!AbilitySystemReferences.IsEmpty() && AbilitySystemReferences[0].AttributeSet)
	{
		if (FGameplayTag* AttributeTag = AbilitySystemReferences[0].AttributeSet->AttributesToTags.Find(AttributeData.Attribute))
		{
			CachedAttribute.Emplace(*AttributeTag, AttributeData.NewValue);
		}
	}
}

void UAttributeWidgetController::UpdateCachedPreviewAttribute(const FGameplayAttribute& Attribute, const float NewValue)
{
	if (!AbilitySystemReferences.IsEmpty() && AbilitySystemReferences[0].AttributeSet)
	{
		if (FGameplayTag* AttributeTag = AbilitySystemReferences[0].AttributeSet->AttributesToTags.Find(Attribute))
		{
			CachedPreviewAttribute.Emplace(*AttributeTag, NewValue);
		}
	}
}

void UAttributeWidgetController::StartPreview(const FGameplayTag& CurrentTag, const FGameplayTag& MaxTag)
{
	if (CachedPreviewAttribute.IsEmpty())
	{
		return;
	}

	bool bShouldPreview = false;

	// Preview 내역이 있는 경우에만 Preview를 표시할 수 있도록 합니다.
	float PreviewCurrentValue = CachedAttribute.FindRef(CurrentTag);
	const float* DeltaCurrentValue = CachedPreviewAttribute.Find(CurrentTag);
	if (DeltaCurrentValue)
	{
		PreviewCurrentValue += *DeltaCurrentValue;
		bShouldPreview = true;
	}
	
	float PreviewMaxValue = CachedAttribute.FindRef(MaxTag);
	const float* DeltaMaxValue = CachedPreviewAttribute.Find(MaxTag);
	if (DeltaMaxValue)
	{
		PreviewMaxValue += *DeltaMaxValue;
		bShouldPreview = true;
	}

	// Preview 여부를 AttributeWidget에게 알려줍니다.
	if (bShouldPreview)
	{
		if (const FOnAttributeChanged* OnChanged = OnPreviewAttributeChangedMap.Find(CurrentTag))
		{
			FAttributeData Data;
			Data.bIsPreview = true;
			Data.CurrentValue = PreviewCurrentValue;
			Data.MaxValue = PreviewMaxValue;
			OnChanged->Broadcast(Data);
		}
	}
}

void UAttributeWidgetController::StopPreview(const FGameplayTag& CurrentTag, const FGameplayTag& MaxTag)
{
	if (CachedPreviewAttribute.IsEmpty())
	{
		return;
	}

	// Preview 내역이 있는 경우에만 Preview를 취소하는 동작을 할 수 있도록 합니다.
	const bool bIsPreviewing = CachedPreviewAttribute.Contains(CurrentTag) || CachedPreviewAttribute.Contains(MaxTag);
	if (bIsPreviewing)
	{
		const float CurrentValue = CachedAttribute.FindRef(CurrentTag);
		const float PreviewMaxValue = CachedAttribute.FindRef(MaxTag);

		// Preview가 중단되도록 Widget에게 알려줍니다.
		if (const FOnAttributeChanged* OnChanged = OnPreviewEndedMap.Find(CurrentTag))
		{
			FAttributeData Data;
			Data.bIsPreview = false;
			Data.CurrentValue = CurrentValue;
			Data.MaxValue = PreviewMaxValue;
			OnChanged->Broadcast(Data);
		}
	}
}

void UAttributeWidgetController::StartAllPreview()
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	StartPreview(LetheGameplayTags.Attributes_Vital_Health, LetheGameplayTags.Attributes_Vital_MaxHealth);
}

void UAttributeWidgetController::StopAllPreview()
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	StopPreview(LetheGameplayTags.Attributes_Vital_Health, LetheGameplayTags.Attributes_Vital_MaxHealth);
}
