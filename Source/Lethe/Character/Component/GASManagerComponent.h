// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Components/PawnComponent.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "GASManagerComponent.generated.h"

enum class EPhaseState : uint8;
class UAbilitySystemComponent;
class UAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
struct FSavedCard;

UCLASS(NotBlueprintable)
class LETHE_API UGASManagerComponent : public UPawnComponent, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	UGASManagerComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin AActorComponent Interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActorComponent Interface

	/** PlayerState가 아닌 Character가 ASC, AS를 직접 생성하기 때문에, 아래 함수로 넘겨받아 할당합니다. */
	void SetAbilitySystemComponent(UAbilitySystemComponent* InAbilitySystemComponent);
	void SetAttributeSet(UAttributeSet* InAttributeSet);
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	virtual void InitAbilityActorInfo(const TArray<UUserWidget*>& AttributeWidgets);
	
	virtual void AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& InAbilities) const;
	virtual void AddCharacterAbilities(const TArray<FSavedCard>& InCards) const;
	
	void OnDied() const;

protected:
	void ApplyEffectToSelf(const TSubclassOf<UGameplayEffect>& GameplayEffectClass, const float Level) const;

private:
	void OnPhaseStateChanged(const EPhaseState OldPhase, const EPhaseState NewPhase) const;
	void OnPlanPhaseStarted() const;

protected:
	/** 게임 시작 시 기본으로 적용되어 Attribute를 초기화하는 GameplayEffect입니다. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultAttributes;

	/** 턴 시작 시 Cost와 Mana를 회복하는 GameplayEffect입니다. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> TurnStartRecovery;

	/**
	 * MoveAbility를 포함, 시작과 동시에 부여되는 Ability입니다.
	 * 캐릭터의 Passive Ability처럼 카드로 사용하지 않는 능력을 구현할 때 활용할 수 있습니다.
	 */
	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<UGameplayAbility>> StartAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Team")
	ETeamSide TeamSide;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
};
