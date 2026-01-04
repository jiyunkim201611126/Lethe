// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Components/PawnComponent.h"
#include "GASManagerComponent.generated.h"

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

	// 테스트용 변수입니다.
	// Ability 부여 후 해당하는 Card Widget이 생성되는 로직을 거쳐야 하기 때문에, 모든 객체 생성 완료 시점에 Ability를 부여해야 합니다.
	// 해당 Ability 정보는 덱빌딩 시점에 SaveGame에서 들고 있다가, Character가 불러들여와 부여하는 것으로 예상 중입니다.
	// 현재는 Character에서 직접 수행합니다.
	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<UGameplayAbility>> TestAbilities;
};
