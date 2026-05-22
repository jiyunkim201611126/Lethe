// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "LetheAbilitySystemComponent.generated.h"

class UCardDefinitionData;
class UCharacterDefinitionData;
class ULetheGameplayAbility;
struct FLoadedCardInfo;
struct FSavedCard;

DECLARE_DELEGATE_FourParams(FOnAbilityGivenSignature, ULetheAbilitySystemComponent* /* this */, const UCharacterDefinitionData*, const UCardDefinitionData*, const FSavedCard&);

UCLASS(NotBlueprintable)
class LETHE_API ULetheAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** Ability를 부여하는 함수입니다. */
	void AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& InAbilities);
	
	/** Ability를 부여함과 동시에 한 번 발동하는 함수입니다. 보통 Passive 구현에 사용합니다. */
	void AddCharacterAbilitiesWithActive(const TArray<TSubclassOf<UGameplayAbility>>& InAbilities);

	void AddCharacterAbilities(const TArray<FSavedCard>& InSavedCards);

private:
	void OnAllCardsLoaded(const FGameplayTag& CharacterTag, const FLoadedCardInfo& LoadedCardInfo, bool bEquipped);

public:
	FOnAbilityGivenSignature OnAbilityGivenDelegate;
};
