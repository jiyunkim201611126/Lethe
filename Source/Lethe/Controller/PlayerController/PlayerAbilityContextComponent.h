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

	// 비전투 페이즈에 MoveAbility를 사용하는 함수입니다.
	void ReserveMove(AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const ATile* TargetTile);
	void ProcessAllMoves();
	void OnPlayerMoveResolved(const AActor* MovedCharacter);
	void ResetReservedMoveData();

	// 전투 페이즈에 MoveAbility를 사용하는 함수입니다.
	void RequestMove(const AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const TArray<ATile*>& TilesInRange, ATile* TargetTile) const;
	
	bool RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, int32 InHandIndex) const;
	void GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, FText& OutText) const;

	bool TryGetMovePathLocations(TMap<APlayerCharacterBase*, TArray<FVector>>& OutMovePathLocations) const;

private:
	TWeakObjectPtr<UActorSelectorComponent> ActorSelector;
	
	struct FPlayerCharacterReservedMove
	{
		TWeakObjectPtr<AActor> PlayerCharacter;
		TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
		TArray<TWeakObjectPtr<ATile>> PathTiles;
		TWeakObjectPtr<ATile> TargetTile;

		bool IsValid() const
		{
			return PlayerCharacter.IsValid() && AbilitySystemComponent.IsValid() && !PathTiles.IsEmpty() && TargetTile.IsValid();
		}
	};

	TArray<FPlayerCharacterReservedMove> ReservedMoves;
};
