// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Lethe/SaveGame/SavedCardTypes.h"
#include "LetheAbilitySystemComponent.generated.h"

class UCardDefinitionData;
class UCharacterDefinitionData;
class ULetheGameplayAbility;
struct FLoadedCardInfo;

USTRUCT()
struct FGrantedCardAbilityInfo
{
	GENERATED_BODY()

	UPROPERTY()
	ULetheAbilitySystemComponent* CardOwnerASC = nullptr;

	UPROPERTY()
	const UCharacterDefinitionData* CharacterDefinitionData = nullptr;

	UPROPERTY()
	const UCardDefinitionData* CardDefinitionData = nullptr;

	FSavedCard SavedCard;

	FGameplayAbilitySpecHandle AbilitySpecHandle;
};

DECLARE_DELEGATE_OneParam(FOnAbilityGivenSignature, const FGrantedCardAbilityInfo&);

UCLASS(NotBlueprintable)
class LETHE_API ULetheAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** Ability를 부여하는 함수입니다. */
	void GiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& InAbilities);
	void GiveAbilities(const TArray<FSavedCard>& InSavedCards);
	
	/** Ability를 부여함과 동시에 한 번 발동하는 함수입니다. 보통 Passive 구현에 사용합니다. */
	void GiveAbilitiesAndActiveOnce(const TArray<TSubclassOf<UGameplayAbility>>& InAbilities);

private:
	void OnAllCardsLoaded(const FGameplayTag& CharacterTag, const FLoadedCardInfo& LoadedCardInfo, bool bEquipped);

public:
	FOnAbilityGivenSignature OnAbilityGivenDelegate;
};
