// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheWidgetController.h"

#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/PlayerAttributeSet.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"

void ULetheWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams)
{
	PlayerController = WidgetControllerParams.PlayerController;

	AbilitySystemReferences.SetNum(PLAYER_CHARACTER_NUMBER);
	ULetheAbilitySystemComponent* AbilitySystemComponent = CastChecked<ULetheAbilitySystemComponent>(WidgetControllerParams.AbilitySystemComponent);
	ULetheAttributeSet* AttributeSet = CastChecked<ULetheAttributeSet>(WidgetControllerParams.AttributeSet);

	// Enemy의 경우 PlayerAttributeSet이 nullptr이기 때문에 CastChecked 대신 Cast를 사용합니다.
	UPlayerAttributeSet* PlayerAttributeSet = Cast<UPlayerAttributeSet>(WidgetControllerParams.PlayerAttributeSet);
	
	FAbilitySystemReference AbilitySystemReference;
	AbilitySystemReference.AbilitySystemComponent = AbilitySystemComponent;
	AbilitySystemReference.AttributeSet = AttributeSet;
	AbilitySystemReference.PlayerAttributeSet = PlayerAttributeSet;

	if (const IPlayerCharacterInterface* PlayerCharacter = Cast<IPlayerCharacterInterface>(AbilitySystemComponent->GetAvatarActor()))
	{
		const int32 OrderIndex = PlayerCharacter->GetPlayerOrderIndex();
		if (AbilitySystemReferences.IsValidIndex(OrderIndex))
		{
			AbilitySystemReferences[OrderIndex] = AbilitySystemReference;
		}
	}
}

void ULetheWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS, UPlayerAttributeSet* PAS)
{
}

void ULetheWidgetController::BroadcastInitialValue()
{
}

APlayerController* ULetheWidgetController::GetPC()
{
	return PlayerController;
}

const TArray<FAbilitySystemReference>& ULetheWidgetController::GetAbilitySystemReferences()
{
	return AbilitySystemReferences;
}
