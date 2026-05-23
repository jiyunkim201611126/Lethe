// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAttributeWidgetController.h"

#include "Components/SlateWrapperTypes.h"
#include "Lethe/AbilitySystem/Ability/LetheCardAbility.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/PlayerAttributeSet.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Manager/LetheGameplayTags.h"

void UPlayerAttributeWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS, UPlayerAttributeSet* PAS)
{
	check(PAS);
	Super::BindCallbacks(ASC, AS, PAS);
	
	ASC->GetGameplayAttributeValueChangeDelegate(PAS->GetManaAttribute()).AddUObject(this, &ThisClass::OnManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(PAS->GetMaxManaAttribute()).AddUObject(this, &ThisClass::OnManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(PAS->GetCostAttribute()).AddUObject(this, &ThisClass::OnCostChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(PAS->GetMaxCostAttribute()).AddUObject(this, &ThisClass::OnCostChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMoveRangeAttribute()).AddUObject(this, &ThisClass::OnMoveRangeChanged);
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnChangePhaseState.AddUObject(this, &ThisClass::OnPhaseStateChanged);
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
	Data.CurrentValue = CachedAttribute.FindRef(LetheGameplayTags.Attribute_Vital_Mana);
	Data.MaxValue = CachedAttribute.FindRef(LetheGameplayTags.Attribute_Vital_MaxMana);
	if (const FOnAttributeChanged* OnManaChanged = OnAttributeChangedMap.Find(LetheGameplayTags.Attribute_Vital_Mana))
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
	Data.CurrentValue = CachedAttribute.FindRef(LetheGameplayTags.Attribute_Vital_Cost);
	Data.MaxValue = CachedAttribute.FindRef(LetheGameplayTags.Attribute_Vital_MaxCost);
	if (const FOnAttributeChanged* OnCostChanged = OnAttributeChangedMap.Find(LetheGameplayTags.Attribute_Vital_Cost))
	{
		OnCostChanged->Broadcast(Data);
	}
}

void UPlayerAttributeWidgetController::StartAllPreview(const TMap<FGameplayTag, float>& InPreviewData)
{
	Super::StartAllPreview(InPreviewData);
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	StartPreview(LetheGameplayTags.Attribute_Vital_Mana, LetheGameplayTags.Attribute_Vital_MaxMana, InPreviewData);
	StartPreview(LetheGameplayTags.Attribute_Vital_Cost, LetheGameplayTags.Attribute_Vital_MaxCost, InPreviewData);
}

void UPlayerAttributeWidgetController::StopAllPreview()
{
	Super::StopAllPreview();

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	StopPreview(LetheGameplayTags.Attribute_Vital_Mana, LetheGameplayTags.Attribute_Vital_MaxMana);
	StopPreview(LetheGameplayTags.Attribute_Vital_Cost, LetheGameplayTags.Attribute_Vital_MaxCost);
}

void UPlayerAttributeWidgetController::OnMoveRangeChanged(const FOnAttributeChangeData& AttributeData)
{
	bHasRemainingMoveRange = 0 < AttributeData.NewValue;
	BroadcastMarkerVisibilityChanged();
}

void UPlayerAttributeWidgetController::OnPhaseStateChanged(const EPhaseState OldPhase, const EPhaseState NewPhase)
{
	CurrentPhaseState = NewPhase;
	BroadcastMarkerVisibilityChanged();
}

void UPlayerAttributeWidgetController::BroadcastMarkerVisibilityChanged() const
{
	const bool bShouldShow = CurrentPhaseState == EPhaseState::PlayerMovePhase && bHasRemainingMoveRange;
	const ESlateVisibility Visibility = bShouldShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	OnMarkerVisibilityChanged.Broadcast(Visibility);
}
