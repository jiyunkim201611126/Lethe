// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheCharacterBase.h"
#include "Lethe/Data/Stage/TileData.h"
#include "Lethe/Interface/TileVisionAffectedInterface.h"
#include "EnemyCharacterBase.generated.h"

UCLASS()
class LETHE_API AEnemyCharacterBase : public ALetheCharacterBase, public ITileVisionAffectedInterface
{
	GENERATED_BODY()

public:
	void SetEnemyAbilityPriority(const int32 InPriority);
	int32 GetEnemyAbilityPriority() const;

	void ProcessPlanPhase() const;
	void ProcessTelegraphPlan() const;
	
	//~ Begin ICombatInterface
	virtual void UpdateHiddenByTile_Implementation(const ATile* Tile) override;
	virtual void OnDamageTaken() override;
	virtual void Die() override;
	//~ End of ICombatInterface

	void NotifyNoiseHeard() const;

	const FBFSRange& GetAbilityRange() const;

protected:
	virtual void OnMoveTileChanged(ATile* OldTile, ATile* NewTile) override;

protected:
	/** 캐릭터의 사정거리입니다. */
	// TODO: 현재로선 간단하게 구현하기 위해 이곳에 선언되었으나, 추후 Ability에 선언된 AbilityRange를 사용하도록 구현할 필요가 있습니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status")
	FBFSRange AbilityRange;

private:
	int32 AbilityPriority = INT_MAX;
};
