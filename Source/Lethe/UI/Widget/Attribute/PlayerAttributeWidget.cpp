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

		AttributeWidgetController->OnPreviewAttributeChangedMap.Emplace(LetheGameplayTags.Attributes_Vital_Mana).AddUObject(this, &ThisClass::StartPreviewMana);
		AttributeWidgetController->OnPreviewAttributeChangedMap.Emplace(LetheGameplayTags.Attributes_Vital_MaxMana).AddUObject(this, &ThisClass::StartPreviewMana);
		AttributeWidgetController->OnPreviewAttributeChangedMap.Emplace(LetheGameplayTags.Attributes_Vital_Cost).AddUObject(this, &ThisClass::StartPreviewCost);
		
		AttributeWidgetController->OnPreviewEndedMap.Emplace(LetheGameplayTags.Attributes_Vital_Mana).AddUObject(this, &ThisClass::StopPreviewMana);
		AttributeWidgetController->OnPreviewEndedMap.Emplace(LetheGameplayTags.Attributes_Vital_Cost).AddUObject(this, &ThisClass::StopPreviewCost);
	}
}

void UPlayerAttributeWidget::UpdateManaUI(const FAttributeData& NewData) const
{
	ManaBar->SetBarPercent(UKismetMathLibrary::SafeDivide(NewData.CurrentValue, NewData.MaxValue));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(NewData.CurrentValue), FMath::RoundToInt(NewData.MaxValue)));
}

void UPlayerAttributeWidget::UpdateCostUI(const FAttributeData& NewData)
{
	StopPreviewCostBlinking();
	CardCostText->SetText(FText::Format(INVTEXT("{0}"), FMath::RoundToInt(NewData.CurrentValue)));
}

void UPlayerAttributeWidget::StartPreviewMana(const FAttributeData& NewData) const
{
	ManaBar->SetPreviewBarPercent(UKismetMathLibrary::SafeDivide(NewData.CurrentValue, NewData.MaxValue));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(NewData.CurrentValue), FMath::RoundToInt(NewData.MaxValue)));
}

void UPlayerAttributeWidget::StartPreviewCost(const FAttributeData& NewData)
{
	PlayPreviewCostBlinking(GetWorld()->GetTimeSeconds());
	CardCostText->SetText(FText::Format(INVTEXT("{0}"), FMath::RoundToInt(NewData.CurrentValue)));
}

void UPlayerAttributeWidget::StopPreviewMana(const FAttributeData& NewData) const
{
	ManaBar->StopPreview(UKismetMathLibrary::SafeDivide(NewData.CurrentValue, NewData.MaxValue));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(NewData.CurrentValue), FMath::RoundToInt(NewData.MaxValue)));
}

void UPlayerAttributeWidget::StopPreviewCost(const FAttributeData& NewData)
{
	StopPreviewCostBlinking();
	CardCostText->SetText(FText::Format(INVTEXT("{0}"), FMath::RoundToInt(NewData.CurrentValue)));
}
