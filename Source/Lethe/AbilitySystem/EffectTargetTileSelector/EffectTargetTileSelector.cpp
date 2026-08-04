// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectTargetTileSelector.h"

#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void FEffectTargetTileSelector::GetCandidateTiles(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const
{
}

void FEffectTargetTileSelector::GetTargetTiles(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const
{
}

void FEffectTargetTileSelector::GetTargetTilesForAI(FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const
{
	GetTargetTiles(Context, OutResult);
}

void FEffectTargetTileSelector::GetSelectCandidateTiles(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const
{
}

void FEffectTargetTileSelector::GetTargetCandidateTiles(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const
{
}

void FEffectTargetTileSelector::ResolveTargets(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const
{
	OutResult.OutTargetResults.Reset();
	OutResult.bHasValidActorTarget = false;

	const ICombatInterface* SourceCombatInterface = Cast<ICombatInterface>(Context.AvatarActor);
	if (!SourceCombatInterface)
	{
		return;
	}

	const ETeamSide SourceTeamSide = SourceCombatInterface->GetTeamSide();
	TSet<TWeakObjectPtr<AActor>> SelectedActors;

	auto ResolveTargetResult = [&Context, &OutResult, this, SourceTeamSide, &SelectedActors](const FTargetSelectionResult& CandidateResult)
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
				ResolvedResult = &OutResult.OutTargetResults.AddDefaulted_GetRef();
				ResolvedResult->TargetGroupTag = CandidateResult.TargetGroupTag;
			}

			FSelectedTarget& ResolvedTarget = ResolvedResult->Targets.AddDefaulted_GetRef();
			ResolvedTarget.TargetTile = CandidateTarget.TargetTile;
			ResolvedTarget.ActorOnTile = ActorOnTile;
			OutResult.bHasValidActorTarget = true;
			SelectedActors.Add(ActorOnTile);
		}
	};
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();

	// Primary Target들은 우선적으로 남겨야 하므로, 먼저 처리합니다.
	for (const FTargetSelectionResult& CandidateResult : OutResult.OutTargetCandidates)
	{
		if (!CandidateResult.TargetGroupTag.MatchesTagExact(LetheGameplayTags.TargetGroup_Primary))
		{
			continue;
		}

		ResolveTargetResult(CandidateResult);
	}

	// 나머지 그룹은 Primary에서 선택되지 않은 대상만 처리합니다.
	for (const FTargetSelectionResult& CandidateResult : OutResult.OutTargetCandidates)
	{
		if (CandidateResult.TargetGroupTag.MatchesTagExact(LetheGameplayTags.TargetGroup_Primary))
		{
			continue;
		}

		ResolveTargetResult(CandidateResult);
	}
}
