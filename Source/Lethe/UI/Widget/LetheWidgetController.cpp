// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheWidgetController.h"

#include "GameFramework/PlayerState.h"
#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"

void ULetheWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams)
{
	PlayerController = Cast<APlayerController>(WidgetControllerParams.PlayerController);
	PlayerState = Cast<APlayerState>(WidgetControllerParams.PlayerState);

	AbilitySystemReferences.Reserve(PLAYABLE_CHARACTER_NUMBER);
	ULetheAbilitySystemComponent* AbilitySystemComponent = Cast<ULetheAbilitySystemComponent>(WidgetControllerParams.AbilitySystemComponent);
	ULetheAttributeSet* AttributeSet = Cast<ULetheAttributeSet>(WidgetControllerParams.AttributeSet);
	AbilitySystemReferences.Emplace(FAbilitySystemReference(AbilitySystemComponent, AttributeSet));
}

void ULetheWidgetController::BindCallbacksToDependencies(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
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
