// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheWidgetController.h"

#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"

void ULetheWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams)
{
	PlayerController = WidgetControllerParams.PlayerController;
	PlayerState = WidgetControllerParams.PlayerState;

	AbilitySystemReferences.SetNum(PLAYER_CHARACTER_NUMBER);
	ULetheAbilitySystemComponent* AbilitySystemComponent = CastChecked<ULetheAbilitySystemComponent>(WidgetControllerParams.AbilitySystemComponent);
	ULetheAttributeSet* AttributeSet = CastChecked<ULetheAttributeSet>(WidgetControllerParams.AttributeSet);
	
	FAbilitySystemReference AbilitySystemReference;
	AbilitySystemReference.AbilitySystemComponent = AbilitySystemComponent;
	AbilitySystemReference.AttributeSet = AttributeSet;

	const IPlayerCharacterInterface* PlayerCharacter = CastChecked<IPlayerCharacterInterface>(AbilitySystemComponent->GetAvatarActor());
	const int32 OrderIndex = PlayerCharacter->GetPlayerOrderIndex();
	if (AbilitySystemReferences.IsValidIndex(OrderIndex))
	{
		AbilitySystemReferences[OrderIndex] = AbilitySystemReference;
	}
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
