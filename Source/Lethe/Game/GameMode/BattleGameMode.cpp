// Copyright JETBLU, Inc. All Rights Reserved.

#include "BattleGameMode.h"

#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Character/EnemyCharacterBase.h"
#include "Lethe/Character/LethePawn.h"
#include "Lethe/Character/PlayerCharacterBase.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/Data/Stage/StageData.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Manager/DeckManagerSubsystem.h"
#include "Lethe/Manager/EngineSystem/LetheAssetManager.h"
#include "Lethe/Manager/Tile/RoomManagerSubsystem.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"
#include "Lethe/Manager/World/StageRuntimeSubsystem.h"

void ABattleGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	Controller = NewPlayer;

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
		TileManagerSubsystem->MakeNewTileMap(StageType);

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

void ABattleGameMode::OnCharacterDefinitionDataLoaded(const TArray<UPrimaryDataAsset*>& CharacterDefinitions)
{
	InitRoomRoles(CharacterDefinitions);
	
	if (Controller.IsValid())
	{
		if (ALethePawn* LethePawn = Controller->GetPawn<ALethePawn>())
		{
			LethePawn->SetPawnStartLocation();
		}
	}
	
	if (ALetheGameState* LetheGameState = GetGameState<ALetheGameState>())
	{
		LetheGameState->GoEnemyPlanningPhase();
	}
}

void ABattleGameMode::OnFloorTransitionStarted() const
{
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}
	
	TileManagerSubsystem->MakeNewTileMap(StageType);
	InitRoomRoles();
	
	if (Controller.IsValid())
	{
		if (ALethePawn* LethePawn = Controller->GetPawn<ALethePawn>())
		{
			LethePawn->SetPawnStartLocation();
		}
	}
}

void ABattleGameMode::InitRoomRoles(const TArray<UPrimaryDataAsset*>& CharacterDefinitions) const
{
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>();
	UStageRuntimeSubsystem* StageRuntimeSubsystem = GetWorld()->GetSubsystem<UStageRuntimeSubsystem>();
	ALetheGameState* LetheGameState = GetGameState<ALetheGameState>();
	
	if (!TileManagerSubsystem || !RoomManagerSubsystem || !StageRuntimeSubsystem || !LetheGameState)
	{
		return;
	}

	const FStageData* StageData = TileManagerSubsystem->GetStageData(StageType);
	if (!StageData)
	{
		return;
	}

	FCubeCoord LastPlayerCharacterSpawnedCoord;
	
	int32 EnemyPriority = 0;
	for (const URoomRoleAssignmentRuleData* RoomRoleAssignmentRuleData : StageData->RoomAssignmentRules)
	{
		TArray<TArray<FRoomCoordSlot>> OutCoordSlotArrays;
		if (RoomManagerSubsystem->TryAssignRoomRole(RoomRoleAssignmentRuleData, OutCoordSlotArrays))
		{
			// Room 내에 선택할 수 있는 지점이 여러 군데 있다면, 그 중 하나를 랜덤하게 선택합니다.
			const TArray<FRoomCoordSlot>& SelectedSlots = OutCoordSlotArrays[FMath::RandRange(0, OutCoordSlotArrays.Num() - 1)];
			
			int32 PlayerCharacterIndex = 0;
			for (const FRoomCoordSlot& Slot : SelectedSlots)
			{
				ATile* Tile = TileManagerSubsystem->GetTile(Slot.SlotCoord);
				if (!Tile)
				{
					continue;
				}
				
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				
				FTransform SpawnTransform;
				FVector SpawnLocation = Tile->GetActorLocation();
				SpawnTransform.SetLocation(SpawnLocation);
				
				switch (Slot.SlotType)
				{
				case ERoomCoordSlotType::PlayerSpawn:
					if (!CharacterDefinitions.IsEmpty())
					{
						if (const UCharacterDefinitionData* CharacterDefinition = Cast<UCharacterDefinitionData>(CharacterDefinitions[PlayerCharacterIndex]))
						{
							if (APlayerCharacterBase* SpawnedCharacter = GetWorld()->SpawnActorDeferred<APlayerCharacterBase>(CharacterDefinition->CharacterClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
							{
								SpawnedCharacter->SetPersonalColor(CharacterDefinition->PersonalColor);
								SpawnedCharacter->SetPlayerOrderIndex(PlayerCharacterIndex);

								LetheGameState->RegisterPlayerCharacter(SpawnedCharacter);
								SpawnedCharacter->FinishSpawning(SpawnTransform);
								
								TileManagerSubsystem->MapTileAndActor(Tile, SpawnedCharacter);
								TArray<ATile*> SpawnTileArray;
								SpawnTileArray.Add(Tile);
								SpawnedCharacter->MoveToTile(SpawnTileArray, true);
								
								++PlayerCharacterIndex;

								// 테스트 용도로 바로 옆에 몬스터를 스폰시키기 위해 위치를 기록합니다.
								LastPlayerCharacterSpawnedCoord = Slot.SlotCoord;
							}
						}
					}
					else
					{
						TArray<AActor*> PlayerCharacters = LetheGameState->GetPlayerCharacters();
						if (PlayerCharacters.IsValidIndex(PlayerCharacterIndex))
						{
							if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(PlayerCharacters[PlayerCharacterIndex]))
							{
								TileManagerSubsystem->MapTileAndActor(Tile, PlayerCharacter);
								TArray<ATile*> SpawnTileArray;
								SpawnTileArray.Add(Tile);
								PlayerCharacter->MoveToTile(SpawnTileArray, true);
							
								++PlayerCharacterIndex;
							
								// 테스트 용도로 바로 옆에 몬스터를 스폰시키기 위해 위치를 기록합니다.
								LastPlayerCharacterSpawnedCoord = Slot.SlotCoord;
							}
						}
					}
					break;
				case ERoomCoordSlotType::EnemySpawn:
					if (AEnemyCharacterBase* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyCharacterBase>(TestEnemyClass, SpawnLocation, Tile->GetActorRotation(), SpawnParams))
					{
						SpawnedEnemy->SetEnemyAbilityPriority(EnemyPriority);
						EnemyPriority += 100;
						
						LetheGameState->RegisterEnemy(SpawnedEnemy);
						StageRuntimeSubsystem->RegisterFloorActor(SpawnedEnemy);
						
						TileManagerSubsystem->MapTileAndActor(Tile, SpawnedEnemy);
						TArray<ATile*> SpawnTileArray;
						SpawnTileArray.Add(Tile);
						SpawnedEnemy->MoveToTile(SpawnTileArray, true);
					}
					break;
				default:
					if (Slot.SpawnActorClass)
					{
						if (AActor* SpawnedActor = GetWorld()->SpawnActor(Slot.SpawnActorClass, &SpawnTransform, SpawnParams))
						{
							StageRuntimeSubsystem->RegisterFloorActor(SpawnedActor);
							TileManagerSubsystem->MapTileAndActor(Tile, SpawnedActor);
						}
					}
					break;
				}
			}
		}
	}
	
#if WITH_EDITOR
	// 테스트용도로 작성된 구문으로, 플레이어 캐릭터 근처에 바로 적을 하나 스폰합니다.
	if (bSpawnEnemyNearly)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		int32 Direction = 0;
		while (true)
		{
			ATile* Tile = TileManagerSubsystem->GetTile(LastPlayerCharacterSpawnedCoord);
			if (!Tile || !TileManagerSubsystem->CanEnemyAIMoveToTile(Tile))
			{
				LastPlayerCharacterSpawnedCoord = LastPlayerCharacterSpawnedCoord + FCubeCoord::GetDirection(++Direction);
				if (Direction == 18)
				{
					break;
				}
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
				break;
			}
		}
	}
#endif
}

AController* ABattleGameMode::GetController() const
{
	return Controller.Get();
}
