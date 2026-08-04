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

void FEffectTargetTileSelector::ResolveTargets(FEffectTargetTileSelectorContext& Context) const
{
	Context.OutTargetResults.Reset();
	Context.bHasValidActorTarget = false;

	const ICombatInterface* SourceCombatInterface = Cast<ICombatInterface>(Context.AvatarActor);
	if (!SourceCombatInterface)
	{
		return;
	}

	const ETeamSide SourceTeamSide = SourceCombatInterface->GetTeamSide();
	TSet<TWeakObjectPtr<AActor>> SelectedActors;

	auto ResolveTargetResult = [&Context, this, SourceTeamSide, &SelectedActors](const FTargetSelectionResult& CandidateResult)
	{
		FTargetSelectionResult* ResolvedResult = nullptr;
		for (const FSelectedTarget& CandidateTarget : CandidateResult.Targets)
		{
			if (!CandidateTarget.TargetTile.IsValid())
			{
				continue;
			}

			AActor* ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(CandidateTarget.TargetTile.Get());
			const ICombatInterface* TargetCombatInterface = Cast<ICombatInterface>(ActorOnTile);
			if (!TargetCombatInterface)
			{
				continue;
			}

			// 이미 선택한 대상이라면 생략합니다.
			if (SelectedActors.Contains(ActorOnTile))
			{
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
				continue;
			}

			if (!ResolvedResult)
			{
				ResolvedResult = &Context.OutTargetResults.AddDefaulted_GetRef();
				ResolvedResult->TargetGroupTag = CandidateResult.TargetGroupTag;
			}

			FSelectedTarget& ResolvedTarget = ResolvedResult->Targets.AddDefaulted_GetRef();
			ResolvedTarget.TargetTile = CandidateTarget.TargetTile;
			ResolvedTarget.ActorOnTile = ActorOnTile;
			Context.bHasValidActorTarget = true;
			SelectedActors.Add(ActorOnTile);
		}
	};
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();

	// Primary Target들은 우선적으로 남겨야 하므로, 먼저 처리합니다.
	for (const FTargetSelectionResult& CandidateResult : Context.OutTargetCandidates)
	{
		if (!CandidateResult.TargetGroupTag.MatchesTagExact(LetheGameplayTags.TargetGroup_Primary))
		{
			continue;
		}

		ResolveTargetResult(CandidateResult);
	}

	// 나머지 그룹은 Primary에서 선택되지 않은 대상만 처리합니다.
	for (const FTargetSelectionResult& CandidateResult : Context.OutTargetCandidates)
	{
		if (CandidateResult.TargetGroupTag.MatchesTagExact(LetheGameplayTags.TargetGroup_Primary))
		{
			continue;
		}

		ResolveTargetResult(CandidateResult);
	}
}
