// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Lethe/Interface/CombatInterface.h"
#include "LetheCharacterBase.generated.h"

class ALethePawn;
class UAttributeSet;
class UGameplayAbility;
class UGASManagerComponent;
class UWidgetComponent;

UCLASS()
class LETHE_API ALetheCharacterBase : public ACharacter, public IAbilitySystemInterface, public ICombatInterface
{
	GENERATED_BODY()

public:
	ALetheCharacterBase(const FObjectInitializer& ObjectInitializer);
	
	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End of IAbilitySystemInterface
	
	//~ Begin ICombatInterface
	virtual void SetLocationOnTile(FVector InTileLocation) override;
	virtual void Die() override;
	//~ End of ICombatInterface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface

	UFUNCTION()
	void BindCameraHeightChanged(APawn* OldPawn, APawn* NewPawn);
	void UnbindCameraHeightChanged(APawn* OldPawn) const;

protected:
	void InitAbilityActorInfo() const;

private:
	void OnCameraHeightChanged(const float InWidgetSize) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UGASManagerComponent> GASManagerComponent;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWidgetComponent> AttributeWidgetComponent;

	/**
	 * MoveAbility를 포함, 시작과 동시에 부여되는 Ability입니다.
	 * 캐릭터의 Passive Ability처럼 카드로 발동하지 않는 능력을 구현할 때 활용할 수 있습니다.
	 */
	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<UGameplayAbility>> StartAbilities;

private:
	FDelegateHandle OnCameraHeightChangedDelegateHandle;
};
