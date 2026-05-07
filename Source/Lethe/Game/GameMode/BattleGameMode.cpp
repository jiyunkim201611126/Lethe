// Copyright JETBLU, Inc. All Rights Reserved.

#include "BattleGameMode.h"

#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Character/EnemyCharacterBase.h"
#include "Lethe/Character/PlayerCharacterBase.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Manager/DeckManagerSubsystem.h"
#include "Lethe/Manager/EngineSystem/LetheAssetManager.h"
#include "Lethe/Manager/Tile/RoomManagerSubsystem.h"
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

	if (TileManagerSubsystem && DeckManagerSubsystem)
	{
		// 타일부터 생성합니다.
		TileManagerSubsystem->MakeNewTileMap();

		// 전투에 참여할 캐릭터들의 CharacterTag를 가져옵니다.
		TArray<FGameplayTag> CharacterTags;
		CharacterTags.Reserve(PLAYER_CHARACTER_NUMBER);
		for (const auto& EquippedDeck : DeckManagerSubsystem->GetEquippedDecks())
		{
			CharacterTags.Add(EquippedDeck.Key);
		}

		// 콜백을 붙여놓고 Data Asset 로드를 요청합니다.
		const FOnPrimaryDataAssetsLoaded OnComplete = FOnPrimaryDataAssetsLoaded::CreateUObject(this, &ThisClass::OnCharacterDefinitionDataLoaded);
		ULetheAssetManager::Get().LoadPrimaryDataAssets(CharacterTags, OnComplete);
	}
}

int32 ABattleGameMode::GetStartRoomId() const
{
	return StartRoomId;
}

FVector ABattleGameMode::GetStartLocation() const
{
	const URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>();
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!RoomManagerSubsystem || !TileManagerSubsystem)
	{
		return FVector::ZeroVector;
	}
	
	if (const FRoomData* StartRoomData = RoomManagerSubsystem->GetRoomData(StartRoomId))
	{
		if (const ATile* Tile = TileManagerSubsystem->GetTile(StartRoomData->CenterCoord))
		{
			return Tile->GetActorLocation();
		}
	}
	return FVector::ZeroVector;
}

void ABattleGameMode::OnCharacterDefinitionDataLoaded(const TArray<UPrimaryDataAsset*>& CharacterDefinitionDatas) const
{
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	const URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>();
	const FRoomData* StartRoomData = RoomManagerSubsystem->GetRoomData(StartRoomId);
	ALetheGameState* LetheGameState = GetGameState<ALetheGameState>();
	
	if (!TileManagerSubsystem || !RoomManagerSubsystem || !StartRoomData || !LetheGameState)
	{
		return;
	}

	const int32 CharacterNumber = CharacterDefinitionDatas.Num();

	// StartRoom의 CenterCoords를 시작으로 주변 총 CharacterNumber 개수만큼의 타일을 가져옵니다.
	TSet<FCubeCoord> OutCoords;
	const bool bCanSelectOtherRoom = StartRoomData->RoomSize < CharacterNumber;
	TileManagerSubsystem->TileBFS(StartRoomData->CenterCoord, 10, EBFSType::Connection, OutCoords,
		[&OutCoords, CharacterNumber](const FTileData* CurrentTileData, const FTileData* NextTileData)
		{
			return OutCoords.Num() < CharacterNumber;
		},
		[this, bCanSelectOtherRoom, &OutCoords, CharacterNumber](const FCubeCoord CurrentCoord, const FTileData* TileData, const int32 Depth)
		{
			return (bCanSelectOtherRoom ? true : TileData->RoomId == StartRoomId) && OutCoords.Num() < CharacterNumber;
		});

	TArray<FCubeCoord> SpawnCoords = OutCoords.Array();
	for (int32 CharacterIndex = 0; CharacterIndex < CharacterNumber; ++CharacterIndex)
	{
		const UCharacterDefinitionData* CharacterDefinitionData = Cast<UCharacterDefinitionData>(CharacterDefinitionDatas[CharacterIndex]);
		if (!CharacterDefinitionData)
		{
			continue;
		}

		if (ATile* Tile = TileManagerSubsystem->GetTile(SpawnCoords[CharacterIndex]))
		{
			FTransform SpawnTransform;
			FVector SpawnLocation = Tile->GetActorLocation();
			SpawnTransform.SetLocation(SpawnLocation);
			if (APlayerCharacterBase* SpawnedCharacter = GetWorld()->SpawnActorDeferred<APlayerCharacterBase>(CharacterDefinitionData->CharacterClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
			{
				TArray<ATile*> SpawnTileArray;
				SpawnTileArray.Add(Tile);
				SpawnedCharacter->MoveToTile(SpawnTileArray, true);
				
				TileManagerSubsystem->MapTileAndActor(Tile, SpawnedCharacter);
				
				SpawnedCharacter->SetPersonalColor(CharacterDefinitionData->PersonalColor);
				SpawnedCharacter->SetPlayerOrderIndex(CharacterIndex);
				
				LetheGameState->RegisterPlayerCharacter(SpawnedCharacter);
				SpawnedCharacter->FinishSpawning(SpawnTransform);
			}
		}
	}
	
	int32 EnemyPriority = 0;
	for (const FCubeCoord& SpawnCoord : EnemySpawnCoords)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ATile* Tile = TileManagerSubsystem->GetTile(SpawnCoord))
		{
			if (!TileManagerSubsystem->CanEnemyAIMoveToTile(Tile))
			{
				continue;
			}
			
			FVector SpawnLocation = Tile->GetActorLocation();
			if (AEnemyCharacterBase* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyCharacterBase>(TestEnemyClass, SpawnLocation, Tile->GetActorRotation(), SpawnParameters))
			{
				TArray<ATile*> SpawnTileArray;
				SpawnTileArray.Add(Tile);
				SpawnedEnemy->MoveToTile(SpawnTileArray, true);
				
				TileManagerSubsystem->MapTileAndActor(Tile, SpawnedEnemy);
				
				SpawnedEnemy->SetEnemyAbilityPriority(EnemyPriority);
				EnemyPriority += 100;
				
				LetheGameState->RegisterEnemy(SpawnedEnemy);
			}
		}
	}
	LetheGameState->GoEnemyPlanningPhase();
}
