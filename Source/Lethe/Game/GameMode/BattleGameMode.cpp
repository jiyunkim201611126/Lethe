// Copyright JETBLU, Inc. All Rights Reserved.

#include "BattleGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Lethe/Util.h"
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
#include "Lethe/Manager/World/BattleStateSaveSubsystem.h"

struct FRoomRoleSelectionContext
{
	TMap<ERoomRole, FRoomRolePlacementCandidate> AssignedCandidatesByRole;
};

const FRoomRolePlacementCandidate* ABattleGameMode::SelectRoomRoleCandidate(const UObject* WorldContextObject, const URoomRoleAssignmentRuleData* RoomRoleAssignmentRuleData, const TArray<FRoomRolePlacementCandidate>& Candidates, const FRoomRoleSelectionContext& SelectionContext) const
{
	if (!WorldContextObject || !RoomRoleAssignmentRuleData)
	{
		return nullptr;
	}

	switch (RoomRoleAssignmentRuleData->RoomRole)
	{
	case ERoomRole::PlayerSpawn:
		return SelectLargestRoom(Candidates);
	case ERoomRole::StageEnd:
		if (const FRoomRolePlacementCandidate* Candidate = SelectionContext.AssignedCandidatesByRole.Find(ERoomRole::PlayerSpawn))
		{
			return SelectFarthestRoom(WorldContextObject, Candidates, Candidate->RoomId);
		}
		checkf(false, TEXT("StageEnd가 PlayerSpawn보다 먼저 생성 시도되었습니다. DT에서 순서를 바꿔주세요."))
	default:
		return SelectRandomRoom(Candidates);
	}
}

const FRoomRolePlacementCandidate* ABattleGameMode::SelectLargestRoom(const TArray<FRoomRolePlacementCandidate>& Candidates) const
{
	if (Candidates.IsEmpty())
	{
		return nullptr;
	}

	int32 LargestRoomSize = 0;
	for (const FRoomRolePlacementCandidate& Candidate : Candidates)
	{
		LargestRoomSize = FMath::Max(LargestRoomSize, Candidate.RoomSize);
	}

	TArray<int32> CandidateIndexes;
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		if (Candidates[Index].RoomSize == LargestRoomSize)
		{
			CandidateIndexes.Add(Index);
		}
	}

	const int32 SelectedIndex = CandidateIndexes[FMath::RandRange(0, CandidateIndexes.Num() - 1)];
	return &Candidates[SelectedIndex];
}

const FRoomRolePlacementCandidate* ABattleGameMode::SelectFarthestRoom(const UObject* WorldContextObject, const TArray<FRoomRolePlacementCandidate>& Candidates, const int32 StartRoomId) const
{
	if (!WorldContextObject || Candidates.IsEmpty())
	{
		return nullptr;
	}

	const URoomManagerSubsystem* RoomManagerSubsystem = WorldContextObject->GetWorld()->GetSubsystem<URoomManagerSubsystem>();
	if (!RoomManagerSubsystem)
	{
		return nullptr;
	}

	// Room 기준 거리 순으로 정렬된 RoomId 배열을 가져옵니다.
	TArray<int32> OutRoomIds;
	RoomManagerSubsystem->TryGetDistantRoomIds(StartRoomId, OutRoomIds);

	// 먼 거리 순(역순)으로 순회합니다.
	for (int32 Index = OutRoomIds.Num() - 1; Index >= 0; --Index)
	{
		const int32 RoomId = OutRoomIds[Index];

		// 이번 RoomId에 해당하는 후보들을 모두 가져옵니다.
		TArray<int32> CandidateIndices;
		for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
		{
			if (Candidates[CandidateIndex].RoomId == RoomId)
			{
				CandidateIndices.Add(CandidateIndex);
			}
		}

		// 후보가 존재한다면 그 중 랜덤하게 하나 선택해 반환합니다.
		if (!CandidateIndices.IsEmpty())
		{
			const int32 SelectedIndex = CandidateIndices[FMath::RandRange(0, CandidateIndices.Num() - 1)];
			return &Candidates[SelectedIndex];
		}
	}
	
	return nullptr;
}

const FRoomRolePlacementCandidate* ABattleGameMode::SelectRandomRoom(const TArray<FRoomRolePlacementCandidate>& Candidates) const
{
	if (Candidates.IsEmpty())
	{
		return nullptr;
	}

	const int32 SelectedIndex = FMath::RandRange(0, Candidates.Num() - 1);
	return &Candidates[SelectedIndex];
}

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

void ABattleGameMode::OnCharacterDefinitionDataLoaded(const TArray<UPrimaryDataAsset*>& CharacterDefinitions) const
{
	InitRoomRoles(CharacterDefinitions);

	if (UGameplayStatics::HasOption(OptionsString, TEXT("FloorTransition")))
	{
		const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
		const UBattleStateSaveSubsystem* BattleStateSaveSubsystem = GetWorld()->GetSubsystem<UBattleStateSaveSubsystem>();

		FBattleStateSaveContext Context;
		for (AActor* PlayerCharacterActor : LetheGameState->GetPlayerCharacters())
		{
			if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(PlayerCharacterActor))
			{
				Context.PlayerCharacters.Add(PlayerCharacter);
			}
		}
		BattleStateSaveSubsystem->LoadBattleState(Context);
	}
	
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

void ABattleGameMode::InitRoomRoles(const TArray<UPrimaryDataAsset*>& CharacterDefinitions) const
{
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>();
	ALetheGameState* LetheGameState = GetGameState<ALetheGameState>();
	
	if (!TileManagerSubsystem || !RoomManagerSubsystem || !LetheGameState)
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
	FRoomRoleSelectionContext RoomRoleSelectionContext;

	// 스테이지의 모든 RoomAssignmentRule을 순회합니다.
	for (const URoomRoleAssignmentRuleData* RoomRoleAssignmentRuleData : StageData->RoomAssignmentRules)
	{
		// Rule을 만족하는 모든 좌표를 가져옵니다.
		TArray<FRoomRolePlacementCandidate> RoomRoleAssignmentCandidates;
		if (RoomManagerSubsystem->TryFindRoomRoleCandidates(RoomRoleAssignmentRuleData, RoomRoleAssignmentCandidates))
		{
			ERoomRole CurrentRoomRole = RoomRoleAssignmentRuleData->RoomRole;
			
			// 좌표들 중에서 추가적인 조건을 만족하는 좌표를 하나 선택합니다.
			const FRoomRolePlacementCandidate* SelectedCandidate = SelectRoomRoleCandidate(this, RoomRoleAssignmentRuleData, RoomRoleAssignmentCandidates, RoomRoleSelectionContext);
			if (!SelectedCandidate)
			{
				if (CurrentRoomRole == ERoomRole::PlayerSpawn || CurrentRoomRole == ERoomRole::Boss || CurrentRoomRole == ERoomRole::StageEnd)
				{
					checkf(false, TEXT("필수 RoomRole이 정상적으로 부여되지 않았습니다. RoomRole: %s"), *LogHelper::EnumToString(CurrentRoomRole));
				}
				continue;
			}

			// 선택된 Room에게 Role이 Assign되었음을 기록합니다.
			RoomManagerSubsystem->MarkRoomRoleAssigned(*SelectedCandidate);
			RoomRoleSelectionContext.AssignedCandidatesByRole.Add(CurrentRoomRole, *SelectedCandidate);

			const TArray<FRoomCoordSlot>& SelectedSlots = SelectedCandidate->CoordSlots;

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
					break;
				case ERoomCoordSlotType::EnemySpawn:
					if (AEnemyCharacterBase* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyCharacterBase>(TestEnemyClass, SpawnLocation, Tile->GetActorRotation(), SpawnParams))
					{
						SpawnedEnemy->SetEnemyAbilityPriority(EnemyPriority);
						EnemyPriority += 100;
						
						LetheGameState->RegisterEnemy(SpawnedEnemy);
						
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
							TileManagerSubsystem->MapTileAndActor(Tile, SpawnedActor);
							if (SpawnedActor->Implements<UTileVisionAffectedInterface>())
							{
								ITileVisionAffectedInterface::Execute_UpdateHiddenByTile(SpawnedActor, Tile);
							}
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
