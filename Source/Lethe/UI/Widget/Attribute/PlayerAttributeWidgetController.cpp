// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAttributeWidgetController.h"

#include "AbilitySystemInterface.h"
#include "Lethe/AbilitySystem/Abilities/LetheCardAbility.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Player/PlayerController/LethePlayerController.h"

void UPlayerAttributeWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	Super::BindCallbacks(ASC, AS);
	
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetManaAttribute()).AddUObject(this, &ThisClass::OnManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMaxManaAttribute()).AddUObject(this, &ThisClass::OnManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetCostAttribute()).AddUObject(this, &ThisClass::OnCostChanged);
	
	if (ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
	{
		LethePlayerController->OnCardSelectedDelegate.AddUObject(this, &ThisClass::OnCardSelected);
	}
}

void UPlayerAttributeWidgetController::OnManaChanged(const FOnAttributeChangeData& AttributeData)
{
	UpdateCachedAttribute(AttributeData);
	BroadcastManaChanged();
}

void UPlayerAttributeWidgetController::BroadcastManaChanged() const
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	FAttributeData Data;
	Data.CurrentValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_Mana);
	Data.MaxValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_MaxMana);
	if (const FOnAttributeChanged* OnManaChanged = OnAttributeChangedMap.Find(LetheGameplayTags.Attributes_Vital_Mana))
	{
		OnManaChanged->Broadcast(Data);
	}
}

void UPlayerAttributeWidgetController::OnCostChanged(const FOnAttributeChangeData& AttributeData)
{
	UpdateCachedAttribute(AttributeData);
	BroadcastCostChanged();
}

void UPlayerAttributeWidgetController::BroadcastCostChanged() const
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	FAttributeData Data;
	Data.CurrentValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_Cost);
	Data.MaxValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_MaxCost);
	if (const FOnAttributeChanged* OnCostChanged = OnAttributeChangedMap.Find(LetheGameplayTags.Attributes_Vital_Cost))
	{
		OnCostChanged->Broadcast(Data);
	}
}

void UPlayerAttributeWidgetController::OnCardSelected(ULetheAbilitySystemComponent* CardOwnerASC, const ULetheCardAbility* CardAbility)
{
	if (!AbilitySystemReferences.IsEmpty() && AbilitySystemReferences[0].AbilitySystemComponent == CardOwnerASC)
	{
		// 해당 WidgetController와 관련 있는 ASC의 카드가 선택된 경우 들어오는 분기입니다.
		if (CardAbility && !AbilitySystemReferences.IsEmpty())
		{
			// Owner의 카드가 선택된 경우 Cost에 대한 Preview를 수행합니다.
			TMap<FGameplayAttribute, float> OutAbilityCostPreviewData;
			TMap<FGameplayTag, float> OutPreviewData;
			if (CardAbility->TryGetAbilityCostEffectPreviewData(CardOwnerASC, OutAbilityCostPreviewData))
			{
				ConvertAttributeToTag(OutAbilityCostPreviewData, OutPreviewData);
			}
			StartAllPreview(OutPreviewData);
		}
	}
}

void UPlayerAttributeWidgetController::OnOtherTileDetected(const AActor* LastActor, const AActor* CurrentActor, UAbilitySystemComponent* SourceASC, const ULetheCardAbility* CardAbility)
{
	if (AbilitySystemReferences.IsEmpty())
	{
		return;
	}

	UAbilitySystemComponent* ThisASC = AbilitySystemReferences[0].AbilitySystemComponent;
	const IAbilitySystemInterface* LastAbilitySystemInterface = Cast<IAbilitySystemInterface>(LastActor);
	const IAbilitySystemInterface* CurrentAbilitySystemInterface = Cast<IAbilitySystemInterface>(CurrentActor);

	TMap<FGameplayTag, float> OutPreviewData;

	// 선택된 Card의 Owner가 이 WidgetController와 관련이 있으면 들어가는 분기입니다.
	if (SourceASC == ThisASC)
	{
		// Target 유무에 따라 Preview 데이터가 결정되는 경우가 있습니다.
		// 이전에 Preview가 있다가 이번에 없는 경우, 해당 함수 최하단 분기를 통과하지 못 하므로 여기서 명시적으로 Preview를 중단합니다.
		StopAllPreview();
		TMap<FGameplayAttribute, float> OutAbilityCostPreviewData;
		if (CardAbility->TryGetAbilityCostEffectPreviewData(ThisASC, OutAbilityCostPreviewData))
		{
			ConvertAttributeToTag(OutAbilityCostPreviewData, OutPreviewData);
		}
		TMap<FGameplayAttribute, float> OutAbilityEffectPreviewData;
		if (CurrentAbilitySystemInterface)
		{
			if (UAbilitySystemComponent* TargetASC = CurrentAbilitySystemInterface->GetAbilitySystemComponent())
			{
				if (CardAbility->TryGetAbilityEffectsForSourcePreviewData(SourceASC, TargetASC, OutAbilityEffectPreviewData))
				{
					ConvertAttributeToTag(OutAbilityEffectPreviewData, OutPreviewData);
				}
			}
		}
	}
	
	// LastActor의 ASC가 이 WidgetController가 관찰 중인 ASC면 들어가는 분기입니다.
	if (LastAbilitySystemInterface && LastAbilitySystemInterface->GetAbilitySystemComponent() == ThisASC)
	{
		StopAllPreview();
	}

	// CurrentActor의 ASC가 이 WidgetController가 관찰 중인 ASC면 들어가는 분기입니다.
	if (CurrentAbilitySystemInterface && CurrentAbilitySystemInterface->GetAbilitySystemComponent() == ThisASC)
	{
		TMap<FGameplayAttribute, float> OutAbilityEffectPreviewData;
		if (CardAbility->TryGetAbilityEffectsForTargetPreviewData(SourceASC, ThisASC, OutAbilityEffectPreviewData))
		{
			ConvertAttributeToTag(OutAbilityEffectPreviewData, OutPreviewData);
		}
	}

	if (!OutPreviewData.IsEmpty())
	{
		StopAllPreview();
		StartAllPreview(OutPreviewData);
	}
}

void UPlayerAttributeWidgetController::StartAllPreview(const TMap<FGameplayTag, float>& InPreviewData)
{
	Super::StartAllPreview(InPreviewData);
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	StartPreview(LetheGameplayTags.Attributes_Vital_Mana, LetheGameplayTags.Attributes_Vital_MaxMana, InPreviewData);
	StartPreview(LetheGameplayTags.Attributes_Vital_Cost, LetheGameplayTags.Attributes_Vital_MaxCost, InPreviewData);
}

void UPlayerAttributeWidgetController::StopAllPreview()
{
	Super::StopAllPreview();

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	StopPreview(LetheGameplayTags.Attributes_Vital_Mana, LetheGameplayTags.Attributes_Vital_MaxMana);
	StopPreview(LetheGameplayTags.Attributes_Vital_Cost, LetheGameplayTags.Attributes_Vital_MaxCost);
}
