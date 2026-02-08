// Copyright JETBLU, Inc. All Rights Reserved.

#include "AttributeWidgetController.h"

#include "AbilitySystemInterface.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Player/PlayerController/LethePlayerController.h"

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
	
	if (ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
	{
		LethePlayerController->OnOtherTileDetectedDelegate.AddUObject(this, &ThisClass::OnOtherTileDetected);
	}
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

void UAttributeWidgetController::OnOtherTileDetected(const AActor* LastActor, const AActor* CurrentActor, const UAbilitySystemComponent* SourceASC, const ULetheGameplayAbility* CardAbility)
{
	if (AbilitySystemReferences.IsEmpty())
	{
		return;
	}

	UAbilitySystemComponent* ThisASC = AbilitySystemReferences[0].AbilitySystemComponent;
	const IAbilitySystemInterface* LastAbilitySystemInterface = Cast<IAbilitySystemInterface>(LastActor);
	const IAbilitySystemInterface* CurrentAbilitySystemInterface = Cast<IAbilitySystemInterface>(CurrentActor);

	// LastActor의 ASC가 이 WidgetController가 관찰 중인 ASC면 들어가는 분기입니다.
	if (LastAbilitySystemInterface && LastAbilitySystemInterface->GetAbilitySystemComponent() == ThisASC)
	{
		// 먼저 현재 작동 중이었던 Preview를 모두 중단하고, Ability의 Target으로서의 Preview Data를 모두 제거한 뒤 다시 Preview를 진행합니다.
		// 자식 클래스에서 Ability의 Target으로서의 Preview만이 아닌 Cost 등 다른 Preview도 진행할 수 있기 때문입니다.
		StopAllPreview();
		CachedAbilityEffectPreviewData.Empty();
		StartAllPreview();
	}

	// CurrentActor의 ASC가 이 WidgetController가 관찰 중인 ASC면 들어가는 분기입니다.
	if (CurrentAbilitySystemInterface && CurrentAbilitySystemInterface->GetAbilitySystemComponent() == ThisASC)
	{
		// 마찬가지로 현재 작동 중이었던 Preview를 모두 중단한 뒤, Ability의 Target으로서의 Preview Data를 업데이트한 후 Preview를 진행합니다.
		StopAllPreview();
		CachedAbilityEffectPreviewData.Empty();
		TMap<FGameplayAttribute, float> TempAbilityEffectPreviewData;
		CardAbility->TryGetAbilityEffectsPreviewData(SourceASC, ThisASC, TempAbilityEffectPreviewData);
		for (const auto& Elem : TempAbilityEffectPreviewData)
		{
			UpdateCachedPreviewAttribute(Elem.Key, Elem.Value);
		}
		StartAllPreview();
	}
}

void UAttributeWidgetController::UpdateCachedAttribute(const FOnAttributeChangeData& AttributeData)
{
	if (!AbilitySystemReferences.IsEmpty() && AbilitySystemReferences[0].AttributeSet)
	{
		if (const FGameplayTag* AttributeTag = AbilitySystemReferences[0].AttributeSet->AttributesToTags.Find(AttributeData.Attribute))
		{
			CachedAttribute.Emplace(*AttributeTag, AttributeData.NewValue);
		}
	}
}

void UAttributeWidgetController::UpdateCachedPreviewAttribute(const FGameplayAttribute& Attribute, const float NewValue)
{
	if (!AbilitySystemReferences.IsEmpty() && AbilitySystemReferences[0].AttributeSet)
	{
		if (const FGameplayTag* AttributeTag = AbilitySystemReferences[0].AttributeSet->AttributesToTags.Find(Attribute))
		{
			const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
			if (AttributeTag->MatchesTagExact(LetheGameplayTags.Damage))
			{
				// Damage Attribute인 경우 Health Attribute로 바꾸고, 값도 음수로 바꿔서 넣어줍니다.
				CachedAbilityEffectPreviewData.FindOrAdd(LetheGameplayTags.Attributes_Vital_Health) -= NewValue;
			}
			else
			{
				// Cost와 마우스 Hovered 상태를 합산해서 보여주기 위해 Emplace가 아닌 FindOrAdd를 사용합니다.
				CachedAbilityEffectPreviewData.FindOrAdd(*AttributeTag) += NewValue;
			}
		}
	}
}

void UAttributeWidgetController::StartPreview(const FGameplayTag& CurrentTag, const FGameplayTag& MaxTag)
{
	bool bShouldPreview = false;

	// Preview 내역이 있는 경우에만 Preview를 표시할 수 있도록 합니다.
	float PreviewCurrentValue = CachedAttribute.FindRef(CurrentTag);
	const float* DeltaCurrentValue = CachedAbilityEffectPreviewData.Find(CurrentTag);
	if (DeltaCurrentValue)
	{
		PreviewCurrentValue += *DeltaCurrentValue;
		bShouldPreview = true;
	}
	
	float PreviewMaxValue = CachedAttribute.FindRef(MaxTag);
	const float* DeltaMaxValue = CachedAbilityEffectPreviewData.Find(MaxTag);
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
	// Preview 내역이 있는 경우에만 Preview를 취소하는 동작을 할 수 있도록 합니다.
	const bool bIsPreviewing = CachedAbilityEffectPreviewData.Contains(CurrentTag) || CachedAbilityEffectPreviewData.Contains(MaxTag);
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
