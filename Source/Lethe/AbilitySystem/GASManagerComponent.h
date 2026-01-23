// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Components/PawnComponent.h"
#include "GASManagerComponent.generated.h"

struct FGameplayTag;
class UGameplayAbility;
class UGameplayEffect;
class UAttributeSet;
class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class ETeamSide : uint8
{
	Player,
	Enemy
};

UCLASS(NotBlueprintable)
class LETHE_API UGASManagerComponent : public UPawnComponent, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	UGASManagerComponent(const FObjectInitializer& ObjectInitializer);

	// 해당 프로젝트는 PlayerState가 아닌 Character가 두 객체를 직접 생성하기 때문에, 아래 함수로 넘겨받아 할당합니다.
	void SetAbilitySystemComponent(UAbilitySystemComponent* InAbilitySystemComponent);
	void SetAttributeSet(UAttributeSet* InAttributeSet);
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	virtual void InitAbilityActorInfo();
	
	virtual void AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& InAbilities) const;
	virtual void AddCharacterAbilities(const TArray<FGameplayTag>& InCardTags) const;

protected:
	// GameplayEffect를 본인에게 적용하는 함수입니다.
	void ApplyEffectToSelf(const TSubclassOf<UGameplayEffect>& GameplayEffectClass, const float Level) const;

public:
	// 게임 시작 시 기본으로 적용되어 Attribute를 초기화하는 GameplayEffect입니다.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultAttributes;

protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "Team")
	ETeamSide TeamSide;
};
