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

void FEffectTargetTileSelector::GetTargetTilesForAI(FEffectTargetTileSelectorContext& Context) const
{
	GetTargetTiles(Context);
}

void FEffectTargetTileSelector::GetSelectCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
}

void FEffectTargetTileSelector::GetTargetCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
}

void FEffectTargetTileSelector::ResolveTargetActors(FEffectTargetTileSelectorContext& Context) const
{
	const ICombatInterface* SourceCombatInterface = Cast<ICombatInterface>(Context.AvatarActor);
	if (!SourceCombatInterface)
	{
		return;
	}

	const ETeamSide SourceTeamSide = SourceCombatInterface->GetTeamSide();
	for (FTargetSelectionResult& TargetResult : Context.OutTargetResults)
	{
		for (auto TargetIt = TargetResult.Targets.CreateIterator(); TargetIt; ++TargetIt)
		{
			FSelectedTarget& Target = *TargetIt;
			
			Target.ActorOnTile.Reset();
			if (!Target.TargetTile.IsValid())
			{
				TargetIt.RemoveCurrentSwap();
				continue;
			}

			AActor* ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(Target.TargetTile.Get());
			const ICombatInterface* TargetCombatInterface = Cast<ICombatInterface>(ActorOnTile);
			if (!TargetCombatInterface)
			{
				TargetIt.RemoveCurrentSwap();
				continue;
			}

			// 시전자와 대상 후보의 팀 관계에 따라 타겟 타일에 추가합니다.
			const ETeamSide TargetTeamSide = TargetCombatInterface->GetTeamSide();
			bool bCanTarget = false;
			switch (TargetTeamRelation)
			{
			case ETargetTeamRelation::AllSides:
				bCanTarget = true;
				break;
			case ETargetTeamRelation::SameTeam:
				bCanTarget = SourceTeamSide == TargetTeamSide;
				break;
			case ETargetTeamRelation::OpposingTeam:
				bCanTarget = SourceTeamSide != TargetTeamSide;
				break;
			default:
				break;
			}

			if (!bCanTarget)
			{
				TargetIt.RemoveCurrentSwap();
				continue;
			}
			
			Target.ActorOnTile = ActorOnTile;
		}
	}
}
