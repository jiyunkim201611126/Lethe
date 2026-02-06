// Copyright JETBLU, Inc. All Rights Reserved.

#include "AttributeWidget.h"

#include "AttributeWidgetController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/UI/Core/LetheTextBlock.h"
#include "Lethe/UI/Widget/ProgressBar/LetheProgressBar.h"

void UAttributeWidget::WidgetControllerSet_Implementation()
{
	if (UAttributeWidgetController* AttributeWidgetController = Cast<UAttributeWidgetController>(WidgetController))
	{
		// Attribute에 해당하는 Tag를 통해서 매핑된 Delegate를 가져와 함수를 바인드합니다.
		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		
		AttributeWidgetController->OnAttributeChangedDelegates.FindOrAdd(LetheGameplayTags.Attributes_Vital_Health).AddUObject(this, &ThisClass::OnHealthChanged);
		AttributeWidgetController->OnAttributeChangedDelegates.FindOrAdd(LetheGameplayTags.Attributes_Vital_MaxHealth).AddUObject(this, &ThisClass::OnMaxHealthChanged);
		AttributeWidgetController->BroadcastInitialValue();
		
		AttributeWidgetController->OnAttributePreviewChangedDelegates.FindOrAdd(LetheGameplayTags.Attributes_Vital_Health).AddUObject(this, &ThisClass::OnHealthPreviewValueChanged);
		AttributeWidgetController->OnAttributePreviewChangedDelegates.FindOrAdd(LetheGameplayTags.Attributes_Vital_MaxHealth).AddUObject(this, &ThisClass::OnMaxHealthPreviewValueChanged);
	}
}

void UAttributeWidget::OnHealthChanged(const float NewValue)
{
	Health = NewValue;
	HealthBar->SetBarPercent(UKismetMathLibrary::SafeDivide(Health, MaxHealth));
	HealthText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth)));
}

void UAttributeWidget::OnMaxHealthChanged(const float NewValue)
{
	MaxHealth = NewValue;
	HealthBar->SetBarPercent(UKismetMathLibrary::SafeDivide(Health, MaxHealth));
	HealthText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth)));
}

void UAttributeWidget::OnHealthPreviewValueChanged(const float PreviewDeltaValue)
{
	PreviewDeltaHealth = PreviewDeltaValue;
	if (FMath::IsNearlyEqual(PreviewDeltaValue, 0.f, 0.001f))
	{
		HealthBar->StopPreview(UKismetMathLibrary::SafeDivide(Health, MaxHealth));
		HealthText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth)));
	}
	else
	{
		StartHealthPreview();
	}
}

void UAttributeWidget::OnMaxHealthPreviewValueChanged(const float PreviewDeltaValue)
{
	PreviewDeltaMaxHealth = PreviewDeltaValue;
	if (FMath::IsNearlyEqual(PreviewDeltaValue, 0.f, 0.001f))
	{
		HealthBar->StopPreview(UKismetMathLibrary::SafeDivide(Health, MaxHealth));
		HealthText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth)));
	}
	else
	{
		StartHealthPreview();
	}
}

void UAttributeWidget::StartHealthPreview() const
{
	const float PreviewHealth = Health + PreviewDeltaHealth;
	const float PreviewMaxHealth = MaxHealth + PreviewDeltaMaxHealth;
	HealthBar->SetPreviewBarPercent(UKismetMathLibrary::SafeDivide(PreviewHealth, PreviewMaxHealth));
	HealthText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(PreviewHealth), FMath::RoundToInt(PreviewMaxHealth)));
}
