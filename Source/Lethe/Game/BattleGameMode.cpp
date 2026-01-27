// Copyright JETBLU, Inc. All Rights Reserved.

#include "BattleGameMode.h"

#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Character/PlayerCharacterBase.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/Manager/DataLoadManagerSubsystem.h"
#include "Lethe/Manager/DeckManagerSubsystem.h"
#include "Lethe/Manager/TileManagerSubsystem.h"

void ABattleGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	/**
	 * 현재는 Battle 레벨에 진입하면 DeckManagerSubsystem에 있는 Equipped Deck을 가져와 Key(CharacterTag)를 기준으로 캐릭터를 생성합니다.
	 * 이는 임시 로직이며, 추후 '실제 참전 중인 캐릭터'만 골라서 가져올 수 있는 로직이 필요합니다.
	 * 예시) GameInstance에 캐싱해둔 CharacterTags
	 */
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	UDeckManagerSubsystem* DeckManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDeckManagerSubsystem>();
	UDataLoadManagerSubsystem* DataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDataLoadManagerSubsystem>();

	if (TileManagerSubsystem && DeckManagerSubsystem && DataLoadManagerSubsystem)
	{
		// 타일부터 생성합니다.
		TileManagerSubsystem->MakeNewTileMap();

		// 전투에 참여할 캐릭터들의 CharacterTag를 가져옵니다.
		TArray<FGameplayTag> CharacterTags;
		CharacterTags.Reserve(PLAYABLE_CHARACTER_NUMBER);
		for (const auto& EquippedDeck : DeckManagerSubsystem->GetEquippedDecks())
		{
			CharacterTags.Emplace(EquippedDeck.Key);
		}

		// 콜백을 붙여놓고 Data Asset 로드를 요청합니다.
		const FOnCharacterDefinitionsLoaded OnComplete = FOnCharacterDefinitionsLoaded::CreateUObject(this, &ThisClass::OnCharacterDefinitionDataLoaded);
		DataLoadManagerSubsystem->LoadCharacterDefinitionData(CharacterTags, OnComplete);
	}
}

void ABattleGameMode::OnCharacterDefinitionDataLoaded(const TArray<UCharacterDefinitionData*>& CharacterDefinitionDatas) const
{
	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		// 가장 왼쪽에 있는 타일을 가져오는 과정입니다.
		FCubeCoord MostLeftTileCoord(0, 0, 0);
		
		const TSet<FCubeCoord> ComponentCoord = TileManagerSubsystem->TileBFS(MostLeftTileCoord, INT32_MAX, EBFSType::Through,
			[]()
			{
				return true;
			},
			[](const FTileData* TileData, int32 Depth)
			{
				if (TileData && TileData->TileActor)
				{
					// 중앙 일렬 타일 중 좌측에 있는 것만 가져옵니다.
					return TileData->TileActor->GetCubeCoord().R == 0 && TileData->TileActor->GetCubeCoord().Q < 0;
				}
				return false;
			});

		if (!ComponentCoord.IsEmpty())
		{
			for (const FCubeCoord& CurrentCoord : ComponentCoord)
			{
				if (CurrentCoord.Q < MostLeftTileCoord.Q)
				{
					MostLeftTileCoord = CurrentCoord;
				}
			}
		}

		// 캐릭터 순서대로 가장 왼쪽 타일, 우상단, 우측, 우하단에 스폰합니다.
		for (int32 CharacterIndex = 0; CharacterIndex < CharacterDefinitionDatas.Num(); CharacterIndex++)
		{
			const UCharacterDefinitionData* CharacterDefinitionData = CharacterDefinitionDatas[CharacterIndex];
			
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			const FCubeCoord TargetCoord = CharacterIndex == 0 ? MostLeftTileCoord : MostLeftTileCoord + FCubeCoord::GetDirection(6 - CharacterIndex);
			ATile* TileActor = TileManagerSubsystem->GetTileActor(TargetCoord);

			APlayerCharacterBase* SpawnedCharacter = GetWorld()->SpawnActor<APlayerCharacterBase>(CharacterDefinitionData->CharacterClass, TileActor->GetActorLocation(), TileActor->GetActorRotation(), SpawnParameters);
			TileActor->SetActorOnTile(SpawnedCharacter);
		}

		// 테스트용으로 중앙에 적을 하나 스폰합니다.
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATile* TileActor = TileManagerSubsystem->GetTileActor(FCubeCoord(0, 0, 0));
		ALetheCharacterBase* SpawnedEnemy = GetWorld()->SpawnActor<ALetheCharacterBase>(TestEnemy, TileActor->GetActorLocation(), TileActor->GetActorRotation(), SpawnParameters);
		TileActor->SetActorOnTile(SpawnedEnemy);
	}
}
