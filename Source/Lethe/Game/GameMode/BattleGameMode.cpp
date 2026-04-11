// Copyright JETBLU, Inc. All Rights Reserved.

#include "BattleGameMode.h"

#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Character/EnemyCharacterBase.h"
#include "Lethe/Character/PlayerCharacterBase.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Manager/DataLoadManagerSubsystem.h"
#include "Lethe/Manager/DeckManagerSubsystem.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

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
		CharacterTags.Reserve(PLAYER_CHARACTER_NUMBER);
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
		FCubeCoord MostLeftTileCoord(0, 0, 0);
		
		TSet<FCubeCoord> SelectedCoord;
		TileManagerSubsystem->TileBFS(MostLeftTileCoord, 3, EBFSType::Through, SelectedCoord,
			[](const FTileData* CurrentTileData, const FTileData* NextTileData)
			{
				return true;
			},
			[](const FCubeCoord CurrentCoord, const FTileData* TileData, int32 Depth)
			{
				if (TileData && TileData->TileActor.IsValid())
				{
					// 중앙 가로 일렬 타일만 가져옵니다.
					const FCubeCoord& CubeCoord = TileData->TileActor->GetCubeCoord();
					return CubeCoord.R == 0;
				}
				return false;
			});

		if (!SelectedCoord.IsEmpty())
		{
			for (const FCubeCoord& CurrentCoord : SelectedCoord)
			{
				if (CurrentCoord.Q < MostLeftTileCoord.Q)
				{
					MostLeftTileCoord = CurrentCoord;
				}
			}
		}

		for (int32 CharacterIndex = 0; CharacterIndex < CharacterDefinitionDatas.Num(); CharacterIndex++)
		{
			const UCharacterDefinitionData* CharacterDefinitionData = CharacterDefinitionDatas[CharacterIndex];
			
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			const FCubeCoord TargetCoord = CharacterIndex == 0 ? MostLeftTileCoord : MostLeftTileCoord + FCubeCoord::GetDirection(6 - CharacterIndex);
			if (ATile* TileActor = TileManagerSubsystem->GetTile(TargetCoord))
			{
				FVector SpawnLocation = TileActor->GetActorLocation();
				if (APlayerCharacterBase* SpawnedCharacter = GetWorld()->SpawnActor<APlayerCharacterBase>(CharacterDefinitionData->CharacterClass, SpawnLocation, TileActor->GetActorRotation(), SpawnParameters))
				{
					TileManagerSubsystem->MapTileAndActor(TileActor, SpawnedCharacter);
					TileManagerSubsystem->OccupyPlayerMoveTile(SpawnedCharacter, TileActor);
					SpawnedCharacter->SetLocationOnTile(SpawnLocation);
					SpawnedCharacter->SetPersonalColor(CharacterDefinitionData->PersonalColor);
				}
			}
		}

		if (ALetheGameState* LetheGameState = GetGameState<ALetheGameState>())
		{
			int32 EnemyPriority = 0;
			for (const FCubeCoord& SpawnCoord : EnemySpawnCoords)
			{
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (ATile* Tile = TileManagerSubsystem->GetTile(SpawnCoord))
				{
					FVector SpawnLocation = Tile->GetActorLocation();
					if (AEnemyCharacterBase* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyCharacterBase>(TestEnemyClass, SpawnLocation, Tile->GetActorRotation(), SpawnParameters))
					{
						SpawnedEnemy->SetLocationOnTile(SpawnLocation);
						SpawnedEnemy->SetEnemyAbilityPriority(EnemyPriority);

						TileManagerSubsystem->MapTileAndActor(Tile, SpawnedEnemy);

						LetheGameState->RegisterEnemy(SpawnedEnemy);
						EnemyPriority += 100;
					}
				}
			}
			LetheGameState->GoEnemyPlanningPhase();
		}
	}
}
