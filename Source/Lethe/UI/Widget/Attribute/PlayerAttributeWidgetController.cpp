// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAttributeWidgetController.h"

#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Player/PlayerController/LethePlayerController.h"

void UPlayerAttributeWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	Super::BindCallbacks(ASC, AS);
	
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetManaAttribute()).AddUObject(this, &ThisClass::OnAttributeChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMaxManaAttribute()).AddUObject(this, &ThisClass::OnAttributeChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetCostAttribute()).AddUObject(this, &ThisClass::OnAttributeChanged);

	if (ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
	{
		LethePlayerController->OnCardSelectedDelegate.AddUObject(this, &ThisClass::OnCardSelected);
		LethePlayerController->OnCancelCardSelectDelegate.AddUObject(this, &ThisClass::OnCancelCardSelect);
	}
}

void UPlayerAttributeWidgetController::OnCardSelected(const ULetheAbilitySystemComponent* CardOwnerASC, const ULetheGameplayAbility* CardAbility)
{
	if (!AbilitySystemReferences.IsEmpty() && AbilitySystemReferences[0].AbilitySystemComponent == CardOwnerASC)
	{
		// 해당 WidgetController와 관련 있는 ASC의 카드가 선택된 경우 들어오는 분기입니다.
		SelectedCardAbility = CardAbility;

		TMap<FGameplayAttribute, float> TempAbilityCostPreviewData;
		if (SelectedCardAbility->TryGetAbilityCostEffectPreviewData(TempAbilityCostPreviewData))
		{
			for (const auto& AbilityCostPreview : TempAbilityCostPreviewData)
			{
				// 카드 Ability의 Cost Effect가 적용됐을 때 수정되는 Attribute를 멤버변수에 캐싱합니다.
				AbilityCostPreviewData.Emplace(AbilityCostPreview.Key, AbilityCostPreview.Value);
				OnAttributePreviewChanged(AbilityCostPreview.Key, AbilityCostPreview.Value);
			}
		}
	}
}

void UPlayerAttributeWidgetController::OnCancelCardSelect()
{
	// 캐싱했던 값을 기준으로 Preview를 취소합니다.
	for (const auto& Data : AbilityCostPreviewData)
	{
		OnAttributePreviewChanged(Data.Key, 0.f);
	}
	
	// 캐싱했던 값을 비워줍니다.
	AbilityCostPreviewData.Empty();
}
