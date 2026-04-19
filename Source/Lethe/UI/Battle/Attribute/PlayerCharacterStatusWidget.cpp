// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerCharacterStatusWidget.h"

#include "PlayerAttributeWidgetController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/UI/Battle/ProgressBar/LetheProgressBar.h"
#include "Lethe/UI/Core/LetheTextBlock.h"

void UPlayerCharacterStatusWidget::WidgetControllerSet_Implementation()
{
	Super::WidgetControllerSet_Implementation();
	
	UPlayerAttributeWidgetController* AttributeWidgetController = CastChecked<UPlayerAttributeWidgetController>(WidgetController);
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	
	AttributeWidgetController->OnAttributeChangedMap.Emplace(LetheGameplayTags.Attribute_Vital_Mana).AddUObject(this, &ThisClass::UpdateManaUI);
	AttributeWidgetController->OnAttributeChangedMap.Emplace(LetheGameplayTags.Attribute_Vital_MaxMana).AddUObject(this, &ThisClass::UpdateManaUI);
	AttributeWidgetController->OnAttributeChangedMap.Emplace(LetheGameplayTags.Attribute_Vital_Cost).AddUObject(this, &ThisClass::UpdateCostUI);

	AttributeWidgetController->OnPreviewAttributeChangedMap.Emplace(LetheGameplayTags.Attribute_Vital_Mana).AddUObject(this, &ThisClass::StartPreviewMana);
	AttributeWidgetController->OnPreviewAttributeChangedMap.Emplace(LetheGameplayTags.Attribute_Vital_MaxMana).AddUObject(this, &ThisClass::StartPreviewMana);
	AttributeWidgetController->OnPreviewAttributeChangedMap.Emplace(LetheGameplayTags.Attribute_Vital_Cost).AddUObject(this, &ThisClass::StartPreviewCost);
	
	AttributeWidgetController->OnPreviewEndedMap.Emplace(LetheGameplayTags.Attribute_Vital_Mana).AddUObject(this, &ThisClass::StopPreviewMana);
	AttributeWidgetController->OnPreviewEndedMap.Emplace(LetheGameplayTags.Attribute_Vital_Cost).AddUObject(this, &ThisClass::StopPreviewCost);
}

void UPlayerCharacterStatusWidget::UpdateManaUI(const FAttributeData& NewData) const
{
	ManaBar->SetBarPercent(UKismetMathLibrary::SafeDivide(NewData.CurrentValue, NewData.MaxValue));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(NewData.CurrentValue), FMath::RoundToInt(NewData.MaxValue)));
}

void UPlayerCharacterStatusWidget::UpdateCostUI(const FAttributeData& NewData)
{
	StopPreviewCostBlinking();
	CardCostText->SetText(FText::Format(INVTEXT("{0}"), FMath::RoundToInt(NewData.CurrentValue)));
}

void UPlayerCharacterStatusWidget::StartPreviewMana(const FAttributeData& NewData) const
{
	ManaBar->SetPreviewBarPercent(UKismetMathLibrary::SafeDivide(NewData.CurrentValue, NewData.MaxValue));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(NewData.CurrentValue), FMath::RoundToInt(NewData.MaxValue)));
}

void UPlayerCharacterStatusWidget::StartPreviewCost(const FAttributeData& NewData)
{
	PlayPreviewCostBlinking(GetWorld()->GetTimeSeconds());
	CardCostText->SetText(FText::Format(INVTEXT("{0}"), FMath::RoundToInt(NewData.CurrentValue)));
}

void UPlayerCharacterStatusWidget::StopPreviewMana(const FAttributeData& NewData) const
{
	ManaBar->StopPreview(UKismetMathLibrary::SafeDivide(NewData.CurrentValue, NewData.MaxValue));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(NewData.CurrentValue), FMath::RoundToInt(NewData.MaxValue)));
}

void UPlayerCharacterStatusWidget::StopPreviewCost(const FAttributeData& NewData)
{
	StopPreviewCostBlinking();
	CardCostText->SetText(FText::Format(INVTEXT("{0}"), FMath::RoundToInt(NewData.CurrentValue)));
}
