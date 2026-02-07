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
		// Attribute에 해당하는 Tag를 통해서 매핑된 Delegate를 가져와 함수를 바인드합니다.
		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		
		AttributeWidgetController->OnAttributeChangedDelegates.Emplace(LetheGameplayTags.Attributes_Vital_Mana).AddUObject(this, &ThisClass::OnManaChanged);
		AttributeWidgetController->OnAttributeChangedDelegates.Emplace(LetheGameplayTags.Attributes_Vital_MaxMana).AddUObject(this, &ThisClass::OnMaxManaChanged);
		AttributeWidgetController->OnAttributeChangedDelegates.Emplace(LetheGameplayTags.Attributes_Vital_Cost).AddUObject(this, &ThisClass::OnCostChanged);
		AttributeWidgetController->BroadcastInitialValue();

		AttributeWidgetController->OnAttributePreviewChangedDelegates.Emplace(LetheGameplayTags.Attributes_Vital_Mana).AddUObject(this, &ThisClass::OnManaPreviewValueChanged);
		AttributeWidgetController->OnAttributePreviewChangedDelegates.Emplace(LetheGameplayTags.Attributes_Vital_MaxMana).AddUObject(this, &ThisClass::OnMaxManaPreviewValueChanged);
		AttributeWidgetController->OnAttributePreviewChangedDelegates.Emplace(LetheGameplayTags.Attributes_Vital_Cost).AddUObject(this, &ThisClass::OnCardCostPreviewValueChanged);
	}
}

void UPlayerAttributeWidget::OnManaChanged(const float NewValue)
{
	Mana = NewValue;
	UpdateManaUI();
}

void UPlayerAttributeWidget::OnMaxManaChanged(const float NewValue)
{
	MaxMana = NewValue;
	UpdateManaUI();
}

void UPlayerAttributeWidget::UpdateManaUI() const
{
	ManaBar->SetBarPercent(UKismetMathLibrary::SafeDivide(Mana, MaxMana));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(Mana), FMath::RoundToInt(MaxMana)));
}

void UPlayerAttributeWidget::OnCostChanged(const float NewValue)
{
	StopAnimation(CostBlinkingAnimation);
	CardCost = NewValue;
	CardCostText->SetText(FText::Format(INVTEXT("{0}"), NewValue));
}

void UPlayerAttributeWidget::OnManaPreviewValueChanged(const float PreviewDeltaValue)
{
	PreviewDeltaMana = PreviewDeltaValue;
	IsPreviewing(PreviewDeltaMana) ? StartManaPreview() : StopManaPreview();
}

void UPlayerAttributeWidget::OnMaxManaPreviewValueChanged(const float PreviewDeltaValue)
{
	PreviewDeltaMaxMana = PreviewDeltaValue;
	IsPreviewing(PreviewDeltaMaxMana) ? StartManaPreview() : StopManaPreview();
}

void UPlayerAttributeWidget::OnCardCostPreviewValueChanged(const float PreviewDeltaValue)
{
	PreviewDeltaCardCost = PreviewDeltaValue;
	if (IsPreviewing(PreviewDeltaValue))
	{
		PlayAnimation(CostBlinkingAnimation, 0, 0);
		CardCostText->SetText(FText::Format(INVTEXT("{0}"), FMath::RoundToInt(CardCost + PreviewDeltaCardCost)));
	}
	else
	{
		StopAnimation(CostBlinkingAnimation);
		CardCostText->SetText(FText::Format(INVTEXT("{0}"), FMath::RoundToInt(CardCost)));
	}
}

void UPlayerAttributeWidget::StartManaPreview() const
{
	const float PreviewMana = Mana + PreviewDeltaMana;
	const float PreviewMaxMana = MaxMana + PreviewDeltaMaxMana;
	ManaBar->SetPreviewBarPercent(UKismetMathLibrary::SafeDivide(PreviewMana, PreviewMaxMana));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(PreviewMana), FMath::RoundToInt(PreviewMaxMana)));
}

void UPlayerAttributeWidget::StopManaPreview() const
{
	ManaBar->StopPreview(UKismetMathLibrary::SafeDivide(Mana, MaxMana));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(Mana), FMath::RoundToInt(MaxMana)));
}
