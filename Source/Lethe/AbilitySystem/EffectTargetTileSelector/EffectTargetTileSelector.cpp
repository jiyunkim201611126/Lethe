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

	for (FTargetSelectResult& TargetTileResult : Context.OutTargetTileResults)
	{
		// 일단 모두 꺼내옵니다.
		TArray<FSelectedTarget> TargetCandidates = MoveTemp(TargetTileResult.Targets);

		const ETeamSide InstigatorTeamSide = InstigatorCombatInterface->GetTeamSide();
		for (FSelectedTarget& Target : TargetCandidates)
		{
			if (!Target.TargetTile.IsValid())
			{
				TargetTileResult.Targets.AddDefaulted();
				continue;
			}

			AActor* ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(Target.TargetTile.Get());
			const ICombatInterface* TargetCombatInterface = Cast<ICombatInterface>(ActorOnTile);
			if (!TargetCombatInterface)
			{
				// EffectTargetMappingPolicies에서 Targets의 인덱스를 기반으로 로직을 수행하기 때문에, 타일은 보존하고 Actor만 비웁니다.
				Target.ActorOnTile.Reset();
				TargetTileResult.Targets.Add(Target);
				continue;
			}

			Target.ActorOnTile = ActorOnTile;

			// 시전자와 대상 후보의 팀 관계에 따라 타겟 타일에 추가합니다.
			const ETeamSide TargetTeamSide = TargetCombatInterface->GetTeamSide();
			switch (TargetTeamRelation)
			{
			case ETargetTeamRelation::AllSides:
				TargetTileResult.Targets.Add(Target);
				break;
			case ETargetTeamRelation::SameTeam:
				if (InstigatorTeamSide == TargetTeamSide)
				{
					TargetTileResult.Targets.Add(Target);
				}
				else
				{
					Target.ActorOnTile.Reset();
					TargetTileResult.Targets.Add(Target);
				}
				break;
			case ETargetTeamRelation::OpposingTeam:
				if (InstigatorTeamSide != TargetTeamSide)
				{
					TargetTileResult.Targets.Add(Target);
				}
				else
				{
					Target.ActorOnTile.Reset();
					TargetTileResult.Targets.Add(Target);
				}
				break;
			default:
				Target.ActorOnTile.Reset();
				TargetTileResult.Targets.Add(Target);
				break;
			}
		}
	}
}
