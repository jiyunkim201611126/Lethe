// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Lethe/Lethe.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Interface/HighlightInterface.h"
#include "LetheCharacterBase.generated.h"

class ALethePawn;
class UAttributeSet;
class UGameplayAbility;
class UGASManagerComponent;
class UWidgetComponent;

UCLASS()
class LETHE_API ALetheCharacterBase : public ACharacter, public IAbilitySystemInterface, public ICombatInterface, public IHighlightInterface
{
	GENERATED_BODY()

public:
	ALetheCharacterBase(const FObjectInitializer& ObjectInitializer);
	
	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End of IAbilitySystemInterface
	
	//~ Begin ICombatInterface
	virtual void SetLocationOnTile(FVector InTileLocation) override;
	virtual int32 GetMoveDistance() const override;
	virtual void OnDamageTaken() override;
	virtual void Die() override;
	//~ End of ICombatInterface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface

	//~ Begin IHighlightInterface
	virtual void HighlightActorTransparentByMouse_Implementation() override;
	virtual void HighlightActorByMouse_Implementation() override;
	virtual void UnhighlightActorByMouse_Implementation() override;
	virtual void HighlightActorByAbility_Implementation(const int32 InOutlineColor) override;
	virtual void UnhighlightActorByAbility_Implementation() override;
	//~ End of IHighlightInterface

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

	// 캐릭터의 최대 이동 거리입니다.
	// TODO: 현재로선 간단하게 구현하기 위해 이곳에 선언되었으나, 추후 Attribute로 옮길 필요가 있습니다.
	UPROPERTY(EditDefaultsOnly, Category = "Status")
	int32 MoveDistance = 2;

private:
	int32 OutlineColorTransparent = CUSTOM_DEPTH_YELLOW_TRANSPARENT;
	int32 OutlineColor = CUSTOM_DEPTH_YELLOW;
};
