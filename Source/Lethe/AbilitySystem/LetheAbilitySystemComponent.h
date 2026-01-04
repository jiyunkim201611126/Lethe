// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "LetheAbilitySystemComponent.generated.h"

DECLARE_DELEGATE_TwoParams(FOnAbilityGivenSignature, UAbilitySystemComponent* OwnerASC, const FGameplayTag&);

UCLASS()
class LETHE_API ULetheAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	// Ability를 부여하는 함수입니다.
	void AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& InAbilities);
	
	// Ability를 부여함과 동시에 한 번 발동하는 함수입니다. 보통 Passive 구현에 사용합니다.
	void AddCharacterAbilitiesWithActive(const TArray<TSubclassOf<UGameplayAbility>>& InAbilities);

protected:
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;

public:
	FOnAbilityGivenSignature OnAbilityGivenDelegate;
};
