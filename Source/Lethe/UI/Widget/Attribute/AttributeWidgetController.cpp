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
	// Attribute들에게 변동사항이 있는 경우 Widget Controller가 알 수 있도록 각 AttributeSet에게 함수를 바인드합니다.
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetHealthAttribute()).AddUObject(this, &ThisClass::OnAttributeChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMaxHealthAttribute()).AddUObject(this, &ThisClass::OnAttributeChanged);
}

void UAttributeWidgetController::OnAttributeChanged(const FOnAttributeChangeData& AttributeData)
{
	if (!AbilitySystemReferences.IsEmpty())
	{
		if (const ULetheAttributeSet* LetheAttributeSet = AbilitySystemReferences[0].AttributeSet)
		{
			if (const FGameplayTag* AttributeTag = LetheAttributeSet->AttributesToTags.Find(AttributeData.Attribute))
			{
				if (const FOnAttributeChanged* AttributeChangedDelegate = OnAttributeChangedDelegates.Find(*AttributeTag))
				{
					AttributeChangedDelegate->Broadcast(AttributeData.NewValue);
				}
			}
		}
	}
}

void UAttributeWidgetController::OnAttributePreviewChanged(const FGameplayAttribute& Attribute, const float PreviewDeltaValue)
{
	if (!AbilitySystemReferences.IsEmpty())
	{
		if (const ULetheAttributeSet* LetheAttributeSet = AbilitySystemReferences[0].AttributeSet)
		{
			if (const FGameplayTag* AttributeTag = LetheAttributeSet->AttributesToTags.Find(Attribute))
			{
				if (const FOnAttributeChanged* AttributeChangedDelegate = OnAttributePreviewChangedDelegates.Find(*AttributeTag))
				{
					AttributeChangedDelegate->Broadcast(PreviewDeltaValue);
				}
			}
		}
	}
}
