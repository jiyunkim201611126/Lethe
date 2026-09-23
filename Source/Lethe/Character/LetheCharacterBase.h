// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Interface/HighlightInterface.h"
#include "LetheCharacterBase.generated.h"

class ALethePawn;
class UGameplayAbility;
class UGASManagerComponent;
class ULetheAbilitySystemComponent;
class ULetheAttributeSet;
class UWidgetComponent;

DECLARE_MULTICAST_DELEGATE(FOnMoveFinished);

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
	virtual int32 GetMoveRange() const override;
	virtual int32 GetMaxMoveRange() const override;
	virtual void OnDamageTaken() override;
	virtual void Die() override;
	virtual bool IsDead() override;
	virtual UAnimMontage* GetMoveAnimation() override;
	virtual ETeamSide GetTeamSide() const override;
	//~ End of ICombatInterface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface

	//~ Begin IHighlightInterface
	virtual void Highlight_Implementation(const EHighlightReason Flag) override;
	virtual void Unhighlight_Implementation(const EHighlightReason Flag) override;
	//~ End of IHighlightInterface
	
	FGameplayTag GetCharacterTag();
	int64 GetCharacterId() const;

	/** MovePath를 따라 걷는 중인지 반환합니다. 순간이동은 즉시 끝나므로 해당하지 않습니다. */
	bool IsMoving() const;

	/** 현재 이동에 적용할 걷기 속도 배율입니다. 이동이 끝나면 1로 되돌아갑니다. */
	void SetMoveSpeedMultiplier(const float InMultiplier);

	/**
	 * 경로 이동이 끝나면 호출됩니다. 목적지 도착, 경로 무효화, 순간이동 모두 포함합니다.
	 * 이동 Ability는 이 신호로 종료 시점을 판단하므로, 이동을 끝내는 모든 경로에서 호출되어야 합니다.
	 */
	FOnMoveFinished OnMoveFinished;

protected:
	virtual void InitAbilityActorInfo() const;

	/**
	 * Enemy만 재정의하는 함수로, 이동 중 밟고 있는 타일이 가시적으로 갱신되면 호출합니다.
	 * '가시적으로'라는 말은, 실제 게임 플레이 로직적으로는 큰 갱신이 없는 상황이란 뜻입니다.
	 */
	virtual void OnMoveTileChanged(ATile* PreviousTile, ATile* CurrentTile);

private:
	void OnCameraHeightChanged(const float InWidgetSize) const;

	/** 이동 상태를 초기화한 뒤 OnMoveFinished를 호출합니다. 콜백 안에서 새 이동이 시작될 수 있으므로 초기화가 먼저입니다. */
	void FinishMove();

protected:
	/**
	 * 캐릭터 식별을 위해 사용하는 Id입니다.
	 * uint64는 Blueprint 미러링을 지원하지 않기 때문에 캐릭터만 예외로 int64를 사용합니다. 이 같은 이유로 양수여야만 합니다.
	 * ※!!  Id는 출시 이후 절대 변경되어선 안 됩니다  !!※
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LetheCharacter")
	int64 CharacterId;
	
	/**
	 * Id로 찾은 Tag가 런타임 중 동적으로 채워집니다.
	 * Id는 출시 이후 절대 변경되지 않으나, CharacterTag는 필요에 따라 변경될 수 있기 때문에 이와 같은 방법을 사용합니다.
	 */
	FGameplayTag CharacterTag;
	
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
	UPROPERTY()
	TArray<TObjectPtr<UWidgetComponent>> AttributeWidgetComponents;

	UPROPERTY()
	TArray<TWeakObjectPtr<ATile>> MovePath;
	float MoveArriveTolerance = 5.f;
	float HiddenTolerance = 30.f;
	float MoveSpeedMultiplier = 1.f;
};
