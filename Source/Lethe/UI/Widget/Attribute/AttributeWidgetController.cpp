// Copyright JETBLU, Inc. All Rights Reserved.

#include "AttributeWidgetController.h"

#include "AbilitySystemInterface.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/Abilities/LetheCardAbility.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/Controller/PlayerController/PreviewCoordinatorComponent.h"
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
	
	if (ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
	{
		if (UPreviewCoordinatorComponent* PreviewCoordinatorComponent = LethePlayerController->GetPreviewCoordinatorComponent())
		{
			PreviewCoordinatorComponent->OnPreviewDataUpdated.AddUObject(this, &ThisClass::OnPreviewDataUpdated);
		}
		LethePlayerController->OnCancelCardSelectDelegate.AddUObject(this, &ThisClass::OnCancelCardSelect);
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
	Data.CurrentValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_Health);
	Data.MaxValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_MaxHealth);
	if (const FOnAttributeChanged* OnHealthChanged = OnAttributeChangedMap.Find(LetheGameplayTags.Attributes_Vital_Health))
	{
		OnHealthChanged->Broadcast(Data);
	}
}


void UAttributeWidgetController::OnPreviewDataUpdated(const FPreviewContext& PreviewContext, const FPreviewData& PreviewData)
{
	if (AbilitySystemReferences.IsEmpty())
	{
		return;
	}

	const UAbilitySystemComponent* ThisASC = AbilitySystemReferences[0].AbilitySystemComponent;
	
	const IAbilitySystemInterface* LastTargetAbilitySystemInterface = Cast<IAbilitySystemInterface>(PreviewContext.LastTargetActor);
	const IAbilitySystemInterface* CurrentTargetAbilitySystemInterface = Cast<IAbilitySystemInterface>(PreviewContext.CurrentTargetActor);
	const UAbilitySystemComponent* LastTargetASC = LastTargetAbilitySystemInterface ? LastTargetAbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	const UAbilitySystemComponent* CurrentTargetASC = CurrentTargetAbilitySystemInterface ? CurrentTargetAbilitySystemInterface->GetAbilitySystemComponent() : nullptr;

	// 직전에 Target이었던 ASC라면 Preview를 중단합니다.
	if (LastTargetASC == ThisASC)
	{
		StopAllPreview();
	}

	// 이번에 Target으로 지정된 ASC라면 Preview를 시작합니다.
	if (CurrentTargetASC == ThisASC)
	{
		StopAllPreview();
		if (!PreviewData.OutPreviewDataForTargetActor.IsEmpty())
		{
			StartAllPreview(PreviewData.OutPreviewDataForTargetActor);
		}
	}
}

void UAttributeWidgetController::OnCancelCardSelect()
{
	StopAllPreview();
}

void UAttributeWidgetController::UpdateCachedAttribute(const FOnAttributeChangeData& AttributeData)
{
	if (const FGameplayTag* AttributeTag = ULetheAttributeSet::AttributesToTags.Find(AttributeData.Attribute))
	{
		CachedAttribute.Emplace(*AttributeTag, AttributeData.NewValue);
	}
}

void UAttributeWidgetController::StartPreview(const FGameplayTag& CurrentTag, const FGameplayTag& MaxTag, const TMap<FGameplayTag, float>& InPreviewData)
{
	bool bShouldPreview = false;

	// Preview 내역이 있는 경우에만 Preview를 표시할 수 있도록 합니다.
	float PreviewCurrentValue = CachedAttribute.FindRef(CurrentTag);
	const float* DeltaCurrentValue = InPreviewData.Find(CurrentTag);
	if (DeltaCurrentValue)
	{
		PreviewCurrentValue += *DeltaCurrentValue;
		bShouldPreview = true;
	}
	
	float PreviewMaxValue = CachedAttribute.FindRef(MaxTag);
	const float* DeltaMaxValue = InPreviewData.Find(MaxTag);
	if (DeltaMaxValue)
	{
		PreviewMaxValue += *DeltaMaxValue;
		bShouldPreview = true;
	}

	PreviewMaxValue = PreviewMaxValue <= 0.f ? 0.f : PreviewMaxValue;
	PreviewCurrentValue = FMath::Clamp(PreviewCurrentValue, 0.f, PreviewMaxValue);

	// Preview 여부를 AttributeWidget에게 알려줍니다.
	if (bShouldPreview)
	{
		NowPreviewAttributes.Emplace(CurrentTag);
		if (const FOnAttributeChanged* OnChanged = OnPreviewAttributeChangedMap.Find(CurrentTag))
		{
			FAttributeData Data;
			Data.CurrentValue = PreviewCurrentValue;
			Data.MaxValue = PreviewMaxValue;
			OnChanged->Broadcast(Data);
		}
	}
}

void UAttributeWidgetController::StopPreview(const FGameplayTag& CurrentTag, const FGameplayTag& MaxTag)
{
	if (!NowPreviewAttributes.Contains(CurrentTag))
	{
		return;
	}

	NowPreviewAttributes.Remove(CurrentTag);
	
	const float CurrentValue = CachedAttribute.FindRef(CurrentTag);
	const float PreviewMaxValue = CachedAttribute.FindRef(MaxTag);

	// Preview가 중단되도록 Widget에게 알려줍니다.
	if (const FOnAttributeChanged* OnChanged = OnPreviewEndedMap.Find(CurrentTag))
	{
		FAttributeData Data;
		Data.CurrentValue = CurrentValue;
		Data.MaxValue = PreviewMaxValue;
		OnChanged->Broadcast(Data);
	}
}

void UAttributeWidgetController::StartAllPreview(const TMap<FGameplayTag, float>& InPreviewData)
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	StartPreview(LetheGameplayTags.Attributes_Vital_Health, LetheGameplayTags.Attributes_Vital_MaxHealth, InPreviewData);
}

void UAttributeWidgetController::StopAllPreview()
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	StopPreview(LetheGameplayTags.Attributes_Vital_Health, LetheGameplayTags.Attributes_Vital_MaxHealth);
}
