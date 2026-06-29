// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameplayAbility.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemLibrary.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Character/EnemyCharacterBase.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void ULetheGameplayAbility::ActivateNoise(const ATile* StandingTile, const ATile* TargetTile)
{
	if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		for (const FNoisePolicy& NoisePolicy : NoisePolicies)
		{
			// 소음이 시작될 타일을 선택합니다.
			const ATile* NoiseStartTile = nullptr;
			switch (NoisePolicy.NoiseStartTile)
			{
			case ENoiseStartTile::StandingTile:
				NoiseStartTile = StandingTile;
				break;
			case ENoiseStartTile::TargetTile:
				NoiseStartTile = TargetTile;
				break;
			}

			if (NoiseStartTile)
			{
				// 소음 범위 내의 모든 적을 가져옵니다.
				TSet<FCubeCoord> EnemyTileCoords;
				TArray<AEnemyCharacterBase*> CombatStartingEnemies;
				TileManagerSubsystem->TileBFS(NoiseStartTile->GetCubeCoord(), NoisePolicy.NoiseRange.Distance, NoisePolicy.NoiseRange.BFSType, EnemyTileCoords,
					[](const FTileData* CurrentTileData, const FTileData* NextTileData)
					{
						return true;
					},
					[TileManagerSubsystem, &CombatStartingEnemies](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
					{
						if (TileData && TileData->TopTile.IsValid())
						{
							if (AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(TileData->TopTile.Get()))
							{
								if (AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(ActorOnTile))
								{
									CombatStartingEnemies.Add(Enemy);
									return true;
								}
							}
						}
						return false;
					});

				// 소음 범위 내의 모든 적을 전투 상태로 변경합니다.
				for (const AEnemyCharacterBase* Enemy : CombatStartingEnemies)
				{
					Enemy->NotifyNoiseHeard();
				}
			}
		}
	}
}

void ULetheGameplayAbility::MakeEffectContextForCue(const FCueDataPayload& CueDataPayload, FGameplayEffectContextHandle& OutHandle)
{
	OutHandle = MakeEffectContext(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo());
	ULetheAbilitySystemLibrary::SetCueContextToEffectContext(CueDataPayload, OutHandle);
}

#if WITH_EDITOR
void ULetheGameplayAbility::PostInitProperties()
{
	Super::PostInitProperties();

	// 어떤 Ability든 사망 시엔 발동할 수 없습니다.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ActivationBlockedTags.AddTag(FLetheGameplayTags::Get().State_Character_Dead);
	}
}
#endif
