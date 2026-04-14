// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "PlayerAbilityContextComponent.generated.h"

class AActor;
class APlayerCharacterBase;
class ATile;
class UActorSelectorComponent;
class UAbilitySystemComponent;
class ULetheAbilitySystemComponent;

enum class EReservedMoveState : uint8
{
	/** ActivationData로 변경되어 큐에 들어가길 대기하는 상태입니다. */
	WaitingForQueue,

	/** 현재 캐릭터가 움직이는 상태임을 표시하며, 움직이고 난 후에도 잔여 MoveDistance가 있다면 계속 Moving으로 기록합니다. */
	Moving,

	/** 경로상에 다른 캐릭터가 막고 있어, 재시도를 기다리는 상태입니다. */
	WaitingForUnblock,

	/** 모든 MoveDistance를 소모했거나, 남은 경로가 없거나, 경로상의 캐릭터가 비켜주길 기다렸지만 아무도 움직이지 않았을 때 Finished 상태가 됩니다. */
	Finished
};
	
struct FPlayerCharacterReservedMove
{
	TWeakObjectPtr<AActor> PlayerCharacter;
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	TArray<TWeakObjectPtr<ATile>> PathTiles;
	
	EReservedMoveState State = EReservedMoveState::Finished;

	bool IsValid() const
	{
		return PlayerCharacter.IsValid() && AbilitySystemComponent.IsValid();
	}
};

UCLASS()
class LETHE_API UPlayerAbilityContextComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerAbilityContextComponent();

	//~ Begin UActorComponent Interface
	virtual void InitializeComponent() override;
	//~ End of UActorComponent Interface

	bool TryGetMovableTiles(AActor* SelectedCharacter, const UAbilitySystemComponent* AbilitySystemComponent, TArray<ATile*>& OutTilesInRange) const;

	/** PlayerMovePhase(비전투 페이즈)에 이동 예약을 시작합니다. */
	void ReserveMove(AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const ATile* TargetTile);

	/** 예약된 이동 실행을 시작합니다. */
	void StartResolveMoves();

	/** 이동 완료 후 예약 경로와 상태를 갱신하고 다음 이동을 큐에 추가합니다. */
	void OnPlayerMoveResolved(const AActor* MovedCharacter);
	
	void ResetReservedMoveData();
	
	/** 전투 페이즈 중 선택한 타일로 즉시 MoveAbility 사용을 요청합니다. */
	void RequestMove(const AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const TArray<ATile*>& TilesInRange, ATile* TargetTile) const;

	bool RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, int32 InHandIndex) const;
	
	void GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, FText& OutText) const;

	bool TryGetMovePathLocations(TMap<APlayerCharacterBase*, TArray<FVector>>& OutMovePathLocations) const;

private:
	/**
	 * 다음 예약 이동을 Ability 큐에 추가합니다.
	 * OnPlayerMoveResolved를 통해 들어온 경우 AbilityResolverComponent의 StartActivatePlayerAbility가 호출되기 직전 타이밍입니다.
	 */
	bool AddMoveActivationData();
	
	/** 경로상에서 가장 멀리 도달할 수 있는 타일을 반환하는 함수입니다. */
	ATile* GetNextReserveTile(FPlayerCharacterReservedMove* ReservedMove) const;

private:
	TWeakObjectPtr<UActorSelectorComponent> ActorSelector;

	TArray<FPlayerCharacterReservedMove> ReservedMoves;
};
