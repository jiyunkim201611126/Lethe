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
	void ProcessCommitPlan() const;
	
	//~ Begin ICombatInterface
	virtual void UpdateHiddenByTile_Implementation(const ATile* Tile) override;
	virtual void OnDamageTaken() override;
	virtual void Die() override;
	//~ End of ICombatInterface

	void NotifyNoiseHeard() const;

	const FBFSRange& GetAbilityRange() const;

protected:
	virtual void OnMoveTileChanged(ATile* OldTile, ATile* NewTile) override;

private:
	int32 AbilityPriority = INT_MAX;
};
