// Copyright JETBLU, Inc. All Rights Reserved.

#include "AttributeWidget.h"

#include "AttributeWidgetController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/UI/Battle/ProgressBar/LetheProgressBar.h"
#include "Lethe/UI/Core/LetheTextBlock.h"

void UAttributeWidget::WidgetControllerSet_Implementation()
{
	UAttributeWidgetController* AttributeWidgetController = CastChecked<UAttributeWidgetController>(WidgetController);
	
	// Attribute에 해당하는 Tag를 통해서 매핑된 Delegate를 가져와 함수를 바인드합니다.
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	
	AttributeWidgetController->OnAttributeChangedMap.Emplace(LetheGameplayTags.Attributes_Vital_Health).AddUObject(this, &ThisClass::UpdateHealthUI);
	AttributeWidgetController->OnPreviewAttributeChangedMap.Emplace(LetheGameplayTags.Attributes_Vital_Health).AddUObject(this, &ThisClass::StartPreviewHealth);
	AttributeWidgetController->OnPreviewEndedMap.Emplace(LetheGameplayTags.Attributes_Vital_Health).AddUObject(this, &ThisClass::StopPreviewHealth);
}

void UAttributeWidget::UpdateHealthUI(const FAttributeData& NewData) const
{
	HealthBar->SetBarPercent(UKismetMathLibrary::SafeDivide(NewData.CurrentValue, NewData.MaxValue));
	HealthText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(NewData.CurrentValue), FMath::RoundToInt(NewData.MaxValue)));
}

void UAttributeWidget::StartPreviewHealth(const FAttributeData& NewData) const
{
	HealthBar->SetPreviewBarPercent(UKismetMathLibrary::SafeDivide(NewData.CurrentValue, NewData.MaxValue));
	HealthText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(NewData.CurrentValue), FMath::RoundToInt(NewData.MaxValue)));
}

void UAttributeWidget::StopPreviewHealth(const FAttributeData& NewData) const
{
	HealthBar->StopPreview(UKismetMathLibrary::SafeDivide(NewData.CurrentValue, NewData.MaxValue));
	HealthText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(NewData.CurrentValue), FMath::RoundToInt(NewData.MaxValue)));
}
