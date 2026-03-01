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
};
