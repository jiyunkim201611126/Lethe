// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectTargetTileSelector.h"

#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
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
			Context.bHasValidActorTarget = true;
		}
	}
	
	// 중복으로 선택된 대상 제거를 시작합니다.
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	TSet<TWeakObjectPtr<AActor>> SelectedActors;
	
	// Primary Target들은 우선적으로 남겨야 하므로, 먼저 SelectedActors에 채워줍니다.
	for (FTargetSelectionResult& TargetResult : Context.OutTargetResults)
	{
		if (!TargetResult.TargetGroupTag.MatchesTagExact(LetheGameplayTags.TargetGroup_Primary))
		{
			continue;
		}
		
		for (auto TargetIt = TargetResult.Targets.CreateIterator(); TargetIt; ++TargetIt)
		{
			// Primary 그룹은 동일한 대상이 수집될 일이 없지만 검증을 위해 확인 후 제거합니다.
			if (!TargetIt->ActorOnTile.IsValid() || SelectedActors.Contains(TargetIt->ActorOnTile))
			{
				TargetIt.RemoveCurrentSwap();
				continue;
			}
			
			SelectedActors.Add(TargetIt->ActorOnTile);
		}
	}
	
	// 나머지 그룹도 동일한 작업을 수행합니다.
	for (FTargetSelectionResult& TargetResult : Context.OutTargetResults)
	{
		if (TargetResult.TargetGroupTag.MatchesTagExact(LetheGameplayTags.TargetGroup_Primary))
		{
			continue;
		}
		
		for (auto TargetIt = TargetResult.Targets.CreateIterator(); TargetIt; ++TargetIt)
		{
			if (!TargetIt->ActorOnTile.IsValid() || SelectedActors.Contains(TargetIt->ActorOnTile))
			{
				TargetIt.RemoveCurrentSwap();
				continue;
			}
			
			SelectedActors.Add(TargetIt->ActorOnTile);
		}
	}
}
