// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAttributeWidget.h"

#include "PlayerAttributeWidgetController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/UI/Core/LetheTextBlock.h"
#include "Lethe/UI/Widget/ProgressBar/LetheProgressBar.h"

void UPlayerAttributeWidget::WidgetControllerSet_Implementation()
{
	Super::WidgetControllerSet_Implementation();
	
	if (UPlayerAttributeWidgetController* AttributeWidgetController = Cast<UPlayerAttributeWidgetController>(WidgetController))
	{
		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		
		AttributeWidgetController->OnAttributeChangedMap.Emplace(LetheGameplayTags.Attributes_Vital_Mana).AddUObject(this, &ThisClass::UpdateManaUI);
		AttributeWidgetController->OnAttributeChangedMap.Emplace(LetheGameplayTags.Attributes_Vital_MaxMana).AddUObject(this, &ThisClass::UpdateManaUI);
		AttributeWidgetController->OnAttributeChangedMap.Emplace(LetheGameplayTags.Attributes_Vital_Cost).AddUObject(this, &ThisClass::UpdateCostUI);

		AttributeWidgetController->OnPreviewAttributeChangedMap.Emplace(LetheGameplayTags.Attributes_Vital_Mana).AddUObject(this, &ThisClass::UpdateManaUI);
		AttributeWidgetController->OnPreviewAttributeChangedMap.Emplace(LetheGameplayTags.Attributes_Vital_MaxMana).AddUObject(this, &ThisClass::UpdateManaUI);
		AttributeWidgetController->OnPreviewAttributeChangedMap.Emplace(LetheGameplayTags.Attributes_Vital_Cost).AddUObject(this, &ThisClass::UpdateCostUI);
		
		AttributeWidgetController->OnPreviewEndedMap.Emplace(LetheGameplayTags.Attributes_Vital_Mana).AddUObject(this, &ThisClass::StopPreviewMana);
		AttributeWidgetController->OnPreviewEndedMap.Emplace(LetheGameplayTags.Attributes_Vital_Cost).AddUObject(this, &ThisClass::StopPreviewCost);
	}
}

void UPlayerAttributeWidget::UpdateManaUI(const FAttributeData& NewData) const
{
	if (NewData.bIsPreview)
	{
		ManaBar->SetPreviewBarPercent(UKismetMathLibrary::SafeDivide(NewData.CurrentValue, NewData.MaxValue));
		ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(NewData.CurrentValue), FMath::RoundToInt(NewData.MaxValue)));
	}
	else
	{
		ManaBar->SetBarPercent(UKismetMathLibrary::SafeDivide(NewData.CurrentValue, NewData.MaxValue));
		ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(NewData.CurrentValue), FMath::RoundToInt(NewData.MaxValue)));
	}
}

void UPlayerAttributeWidget::UpdateCostUI(const FAttributeData& NewData)
{
	if (NewData.bIsPreview)
	{
		PlayAnimation(CostBlinkingAnimation, 0, 0);
		CardCostText->SetText(FText::Format(INVTEXT("{0}"), FMath::RoundToInt(NewData.CurrentValue)));
	}
	else
	{
		StopAnimation(CostBlinkingAnimation);
		CardCostText->SetText(FText::Format(INVTEXT("{0}"), FMath::RoundToInt(NewData.CurrentValue)));
	}
}

void UPlayerAttributeWidget::StopPreviewMana(const FAttributeData& NewData) const
{
	ManaBar->StopPreview(UKismetMathLibrary::SafeDivide(NewData.CurrentValue, NewData.MaxValue));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(NewData.CurrentValue), FMath::RoundToInt(NewData.MaxValue)));
}

void UPlayerAttributeWidget::StopPreviewCost(const FAttributeData& NewData)
{
	StopAnimation(CostBlinkingAnimation);
	CardCostText->SetText(FText::Format(INVTEXT("{0}"), FMath::RoundToInt(NewData.CurrentValue)));
}
