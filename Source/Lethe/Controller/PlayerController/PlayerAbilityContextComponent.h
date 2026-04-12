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
	
struct FPlayerCharacterReservedMove
{
	TWeakObjectPtr<AActor> PlayerCharacter;
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	TArray<TWeakObjectPtr<ATile>> PathTiles;
	TWeakObjectPtr<ATile> TargetTile;

	bool IsValid() const
	{
		// 목적지에 도착했다면 PathTiles는 비어있을 수 있고, 첫 예약이라면 TargetTile이 nullptr이므로 검사하지 않습니다.
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

	/** 비전투 페이즈에 MoveAbility를 사용하는 함수입니다. */
	void ReserveMove(AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const ATile* TargetTile);
	void ProcessAllMoves();
	void OnPlayerMoveResolved(const AActor* MovedCharacter);
	void ResetReservedMoveData();

	/** 전투 페이즈에 MoveAbility를 사용하는 함수입니다. */
	void RequestMove(AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const TArray<ATile*>& TilesInRange, ATile* TargetTile) const;
	
	bool RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, int32 InHandIndex) const;
	void GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, FText& OutText) const;

	bool TryGetMovePathLocations(TMap<APlayerCharacterBase*, TArray<FVector>>& OutMovePathLocations) const;

private:
	bool ReserveNextMoveTile(FPlayerCharacterReservedMove* ReservedMove, const bool bUseCurrentMoveDistance) const;

	/** 경로상에서 가장 멀리 도달할 수 있는 타일을 반환하는 함수입니다. */
	ATile* GetNextReserveTile(FPlayerCharacterReservedMove* ReservedMove, const int32 MoveDistance) const;

private:
	TWeakObjectPtr<UActorSelectorComponent> ActorSelector;

	TArray<FPlayerCharacterReservedMove> ReservedMoves;
};
