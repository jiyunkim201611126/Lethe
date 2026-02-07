// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheWidgetController.h"

#include "GameFramework/PlayerState.h"
#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"

void ULetheWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams)
{
	PlayerController = WidgetControllerParams.PlayerController;
	PlayerState = WidgetControllerParams.PlayerState;

	AbilitySystemReferences.Reserve(PLAYABLE_CHARACTER_NUMBER);
	ULetheAbilitySystemComponent* AbilitySystemComponent = Cast<ULetheAbilitySystemComponent>(WidgetControllerParams.AbilitySystemComponent);
	ULetheAttributeSet* AttributeSet = Cast<ULetheAttributeSet>(WidgetControllerParams.AttributeSet);
	
	FAbilitySystemReference AbilitySystemReference;
	AbilitySystemReference.AbilitySystemComponent = AbilitySystemComponent;
	AbilitySystemReference.AttributeSet = AttributeSet;
	AbilitySystemReferences.Emplace(AbilitySystemReference);
}

void ULetheWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
}

void ULetheWidgetController::BroadcastInitialValue()
{
}

APlayerController* ULetheWidgetController::GetPC()
{
	return PlayerController;
}

APlayerState* ULetheWidgetController::GetPS()
{
	return PlayerState;
}

const TArray<FAbilitySystemReference>& ULetheWidgetController::GetAbilitySystemReferences()
{
	return AbilitySystemReferences;
}
