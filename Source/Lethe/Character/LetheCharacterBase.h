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
class UGameplayAbility;
class UGASManagerComponent;
class ULetheAbilitySystemComponent;
class ULetheAttributeSet;
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
	virtual void MoveToTile(TArray<ATile*>& PathTiles, const bool bTeleport) override;
	virtual int32 GetMoveDistance() const override;
	virtual int32 GetMaxMoveDistance() const override;
	virtual void OnDamageTaken() override;
	virtual void Die() override;
	virtual UAnimMontage* GetMoveAnimation() override;
	virtual ETeamSide GetTeamSide() const override;
	//~ End of ICombatInterface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
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

	/**
	 * Enemy만 재정의하는 함수로, 이동 중 밟고 있는 타일이 가시적으로 갱신되면 호출합니다.
	 * '가시적으로'라는 말은, 실제 게임 플레이 로직적으로는 큰 갱신이 없는 상황이란 뜻입니다.
	 */
	virtual void OnMoveTileChanged(const ATile* PreviousTile, const ATile* CurrentTile);

private:
	void OnCameraHeightChanged(const float InWidgetSize) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UGASManagerComponent> GASManagerComponent;

	UPROPERTY()
	TObjectPtr<ULetheAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<ULetheAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWidgetComponent> CharacterStatusWidgetComponent;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> MoveAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Team")
	ETeamSide TeamSide;

private:
	int32 OutlineColorTransparent = CUSTOM_DEPTH_YELLOW_TRANSPARENT;
	int32 OutlineColor = CUSTOM_DEPTH_YELLOW;

	UPROPERTY()
	TArray<TObjectPtr<UWidgetComponent>> AttributeWidgetComponents;

	UPROPERTY()
	TArray<TWeakObjectPtr<ATile>> MovePath;
	float MoveArriveTolerance = 5.f;
	float HiddenTolerance = 30.f;
};
