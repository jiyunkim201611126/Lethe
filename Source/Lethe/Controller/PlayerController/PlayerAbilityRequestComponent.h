// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerAbilityRequestComponent.generated.h"

class AActor;
class APlayerCharacterBase;
class ATile;
class UActorSelectorComponent;
class UAbilitySystemComponent;
class ULetheAbilitySystemComponent;
struct FSavedCard;

enum class EReservedMoveState : uint8
{
	/** ActivationData로 변경되어 큐에 들어가길 대기하는 상태입니다. */
	WaitingForQueue,

	/** 현재 캐릭터가 움직이는 상태임을 표시하며, 움직이고 난 후에도 잔여 MoveRange가 있다면 계속 Moving으로 기록합니다. */
	Moving,

	/** 경로상에 다른 캐릭터가 막고 있어, 재시도를 기다리는 상태입니다. */
	WaitingForUnblock,

	/** 모든 MoveRange를 소모했거나, 남은 경로가 없거나, 경로상의 캐릭터가 비켜주길 기다렸지만 아무도 움직이지 않았을 때 Finished 상태가 됩니다. */
	Finished
};

enum class EMoveActionType : uint8
{
	/** TargetTile에 무언가 올라간 상태에 Swap도 불가능한 상태입니다. */
	CantReach,

	/** GA_Move를 통해 도달할 수 있는 상태입니다. */
	MoveAbility,

	/** GA_Swap을 통해 도달할 수 있는 상태입니다. */
	SwapAbility,
};
	
struct FPlayerCharacterReservedMove
{
	TWeakObjectPtr<AActor> PlayerCharacter;
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	TArray<TWeakObjectPtr<ATile>> PathTiles;
	
	EReservedMoveState State = EReservedMoveState::Finished;

	uint8 bIsSwapTarget : 1 = false;

	bool IsValid() const
	{
		return PlayerCharacter.IsValid() && AbilitySystemComponent.IsValid();
	}
};

UCLASS()
class LETHE_API UPlayerAbilityRequestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerAbilityRequestComponent();

	//~ Begin UActorComponent Interface
	virtual void InitializeComponent() override;
	//~ End of UActorComponent Interface

#pragma region Reserve Move API
	/** PlayerMovePhase(비전투 페이즈)에 이동 예약을 시작합니다. */
	void ReserveMove(AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const ATile* TargetTile);

	/** 예약된 이동을 파기합니다. */
	void RemoveReservedMove(const AActor* SelectedCharacter);

	/** 예약된 이동 실행을 시작합니다. */
	void StartResolveMoves();

	/** PlayerMovePhase의 이동 완료 후 이벤트를 수신하는 함수입니다. */
	void OnPlayerReservedMoveResolved(AActor* MovedCharacter);

	/** 이동 완료 후 예약 경로와 상태를 갱신하고 다음 이동을 큐에 추가합니다. */
	void RefreshReservedMoveData(FPlayerCharacterReservedMove* ReservedMove) const;

	/** 비전투 페이즈 진입 시 호출되는 함수로, MoveRange가 회복되었기 때문에 모든 데이터를 WaitingForQueue 상태로 변경합니다. */
	void SetAllReservedMovesWaitingForQueue();
	
	void ResetReservedMoveData();

	/** 이동 예약된 모든 타일들의 Location을 반환합니다. */
	bool TryGetMovePathLocations(TMap<APlayerCharacterBase*, TArray<FVector>>& OutMovePathLocations) const;
#pragma endregion Reserve Move API

#pragma region Move API
	bool TryGetMovableTiles(AActor* SelectedCharacter, const UAbilitySystemComponent* AbilitySystemComponent, TArray<ATile*>& OutTilesInRange) const;
	
	/** 전투 페이즈 중 선택한 타일로 즉시 MoveAbility 사용을 요청합니다. */
	void RequestMove(const AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const TArray<ATile*>& TilesInRange, ATile* TargetTile) const;
#pragma endregion Move API

#pragma region Card
	bool RequestUseCard(const APlayerController* PlayerController, ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, int32 InHandIndex) const;
	
	void GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, FText& OutText) const;
#pragma endregion Card

private:
	/**
	 * 다음 예약 이동을 Ability 큐에 추가합니다.
	 * OnPlayerMoveResolved를 통해 들어온 경우 AbilityResolverComponent의 StartActivatePlayerAbility가 호출되기 직전 타이밍입니다.
	 */
	bool TryEnqueueNextReservedMoveActivationData();
	
	/** 내부적으로 GetActionType을 호출해 어떤 방식으로 도달할 수 있는지 반환하며, Swap을 통해 도달할 수 있는 경우 Out 인자들이 채워집니다. */
	EMoveActionType ResolveActionType(const FPlayerCharacterReservedMove* SourceReservedMove, TArray<TWeakObjectPtr<ATile>>& OutPathTiles, FPlayerCharacterReservedMove*& OutSwapTargetReservedMove);

	/** 해당 타일에 어떤 방식으로 도달할 수 있는지 반환하는 함수입니다. */
	EMoveActionType GetActionType(const FPlayerCharacterReservedMove* SourceReservedMove, const ATile* TargetTile, FPlayerCharacterReservedMove*& OutSwapTargetReservedMove);

	/**	예약 경로와 현재 MoveRange만 기준으로 TargetTile에 도달 가능한지 확인합니다. */
	bool CanReachReservedTile(const FPlayerCharacterReservedMove* SourceReservedMove, const ATile* TargetTile) const;

private:
	TWeakObjectPtr<UActorSelectorComponent> ActorSelector;

	TArray<FPlayerCharacterReservedMove> ReservedMoves;

	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AActor>> SwapSourceToTarget;
};
