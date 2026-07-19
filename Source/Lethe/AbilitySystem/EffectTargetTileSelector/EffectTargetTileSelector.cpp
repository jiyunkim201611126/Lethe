// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectTargetTileSelector.h"

#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void FEffectTargetTileSelector::GetCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
}

void FEffectTargetTileSelector::GetTargetTiles(FEffectTargetTileSelectorContext& Context) const
{
}

void FEffectTargetTileSelector::GetSelectCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
}

void FEffectTargetTileSelector::GetTargetCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
}

void FEffectTargetTileSelector::FilterTargetTilesByTeamRelation(FEffectTargetTileSelectorContext& Context) const
{
	const ICombatInterface* InstigatorCombatInterface = Cast<ICombatInterface>(Context.AvatarActor);
	if (!InstigatorCombatInterface)
	{
		return;
	}

	for (FTargetTileResult& TargetTileResult : Context.OutTargetTileResults)
	{
		// 일단 모두 꺼내옵니다.
		TArray<TWeakObjectPtr<ATile>> TargetCandidateTiles = MoveTemp(TargetTileResult.TargetTiles);

		const ETeamSide InstigatorTeamSide = InstigatorCombatInterface->GetTeamSide();
		for (const auto& Tile : TargetCandidateTiles)
		{
			const ICombatInterface* TargetCombatInterface = Cast<ICombatInterface>(Context.TileManagerSubsystem->GetActorOnTile(Tile.Get()));
			if (!TargetCombatInterface)
			{
				// EffectTargetMappingPolicies에서 TargetActors의 인덱스를 기반으로 로직을 수행하기 때문에, nullptr도 추가해야 합니다.
				TargetTileResult.TargetTiles.Add(nullptr);
				continue;
			}

			// 시전자와 대상 후보의 팀 관계에 따라 타겟 타일에 추가합니다.
			const ETeamSide TargetTeamSide = TargetCombatInterface->GetTeamSide();
			switch (TargetTeamRelation)
			{
			case ETargetTeamRelation::AllSides:
				TargetTileResult.TargetTiles.Add(Tile);
				break;
			case ETargetTeamRelation::SameTeam:
				if (InstigatorTeamSide == TargetTeamSide)
				{
					TargetTileResult.TargetTiles.Add(Tile);
				}
				break;
			case ETargetTeamRelation::OpposingTeam:
				if (InstigatorTeamSide != TargetTeamSide)
				{
					TargetTileResult.TargetTiles.Add(Tile);
				}
				break;
			default:
				TargetTileResult.TargetTiles.Add(nullptr);
				break;
			}
		}
	}
}
