// Fill out your copyright notice in the Description page of Project Settings.

#include "TileManagerSubsystem.h"

#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Data/Stage/StageData.h"

void UTileManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	TileDataMap.Empty();
}

void UTileManagerSubsystem::MakeNewTileMap()
{
	//절차적 생성 맵 생성을 시작하는 함수 (GameMode에서 블루프린트 호출)
	//1. 맵 데이터를 초기화한다.
	//2. 타일에 층고를 만들기 위한 알고리즘을 실행한다.
	//3. 타일에 이벤트를 생성하기 위한 알고리즘을 실행한다.
	//4. 데이터를 기반으로 실제 액터를 생성한다.
	if (const FStageData* StageData = GetStageData(FName("Forest")))
	{
		if (const UStageInitData* StageInitData = StageData->StageInitData.LoadSynchronous())
		{
			//맵 시드 이용해서 디버깅 용이하게 만듬
			FRandomStream RandomStream;

			if (StageInitData->MapSeed == 0)
			{
				const int32 RandomSeed = FMath::RandRange(0, 10000);
				UE_LOG(LogTemp, Warning, TEXT("Random Seed : %d"), RandomSeed);
				RandomStream.Initialize(RandomSeed);
			}		
			else
			{
				RandomStream.Initialize(StageInitData->MapSeed);
				UE_LOG(LogTemp, Warning, TEXT("Map Seed : %d"), StageInitData->MapSeed);
			}
	
			InitMapData(StageInitData);
			MakeFloorData(&RandomStream, StageInitData);
			MakeEventData(StageData, StageInitData);
			MakeTileActor(StageData, StageInitData);
		}
	}
}

const FStageData* UTileManagerSubsystem::GetStageData(const FName& StageName) const
{
	if (const UDataTable* LoadedStageDataTable = StageDataTable.LoadSynchronous())
	{
		return LoadedStageDataTable->FindRow<FStageData>(StageName, TEXT(""));
	}
	return nullptr;
}

void UTileManagerSubsystem::InitMapData(const UStageInitData* StageInitData)
{
	TileDataMap.Reserve(StageInitData->MapWidth * StageInitData->MapHeight); //공간 미리 잡는 함수
	
	TArray<FCubeCoord> CoordList;
	GetCoordFromRange(FCubeCoord(0, 0, 0), CoordList, StageInitData->MapWidth, StageInitData->MapHeight);

	for (auto& Coord : CoordList)
	{
		TileDataMap.Emplace(Coord, FTileData());
	}
}

void UTileManagerSubsystem::MakeFloorData(const FRandomStream* RandomStream, const UStageInitData* StageInitData)
{
	//현재는 층 수가 올라갈수록 타일 생성이 어려워지는 변수를 추가하지 않았음
	//맵 구성되는거 보고 최상단 층이 너무 넓다는 생각이 들면 추가하기
	TArray<FCubeCoord> CubeCoordList;
	TileDataMap.GetKeys(CubeCoordList);
	ShuffleArray(RandomStream, CubeCoordList); //Array 셔플
	const UCurveFloat* Curve = StageInitData->ProbabilityCurve.LoadSynchronous(); //확률 커브
	int32 RoomId = 0;
	
	for (FCubeCoord& Coord : CubeCoordList)
	{
		if (RoomId >= StageInitData->FloorIncrementTrialsCount)
		{
			break;
		}

		FTileData* RootTileData = TileDataMap.Find(Coord);
		
		if (RootTileData->Floor >= StageInitData->MaxFloor)
		{
			continue;
		}

		int32 ConsecutiveCount = 0; //연속 시행 횟수
		int32 PrevDepth = -1;
		int32 Probability = 0;
		
		TSet<FCubeCoord> SelectedCoords;
		TileBFS(Coord, 10, EBFSType::Connection, SelectedCoords,
			[]()
			{
				return true;
			},
			[&](const FTileData* CurrentTileData, const int32 CurrentDepth)
			{
				if (CurrentTileData->RoomID != RootTileData->RoomID)
				{
					return false;
				}

				//타일 생성의 랜덤성을 위해 확률 커브, 랜덤 값 등을 활용
				//단, 랜덤성 때문에 이가 빠진 듯이 타일이 삐죽하게 생성되는걸 원치 않아서
				//확률에 한 번 선정되었을 경우 연속 횟수만큼은 확률 계산 하지 않고 통과하도록 함
			
				if (PrevDepth != CurrentDepth) //Depth가 변경되는 페이즈에 잔여 연속 횟수 소진을 위해 필요함
				{
					PrevDepth = CurrentDepth;
					Probability = Curve->GetFloatValue(CurrentDepth * 0.1f) * 100;
					ConsecutiveCount = 0;
				}

				int32 Rand = 0;
			
				if (ConsecutiveCount == 0)
				{
					ConsecutiveCount = StageInitData->ConsecutiveTileCount;
					Rand = RandomStream->FRandRange(0, 100);
				}
				
				if (Rand < Probability)
				{
					--ConsecutiveCount;
					return true;
				}
			
				return false;
			});

		if (SelectedCoords.IsEmpty())
		{
			continue;
		}

		++RoomId;

		//선택된 영역들에 대해 값 변경
		for (auto& SelectedCoord : SelectedCoords)
		{
			FTileData* TileData = TileDataMap.Find(SelectedCoord);
			TileData->RoomID = RoomId;
			TileData->Floor++;
		}

		//테두리 검출 후 타일 연결 끊기
		TSet<FCubeCoord> BoundCoords;
		TileBFS(Coord, 15, EBFSType::Connection, BoundCoords,
			[]()
			{
				return true;
			},
			[&RoomId](const FTileData* CurrentTileData, const int32 CurrentDepth)
			{
				if (CurrentTileData->RoomID != RoomId) //이새끼가 문제임
				{
					return true;
				}

				return false;
			});

		for (FCubeCoord BoundCoord : BoundCoords)
		{
			for (int32 Dir = 0; Dir < 6; ++Dir)
			{
				FTileData* CurrentTileData = TileDataMap.Find(BoundCoord);
				FTileData* NextTileData = TileDataMap.Find(BoundCoord + FCubeCoord::GetDirection(Dir));

				if (NextTileData == nullptr)
				{
					continue;
				}

				if (CurrentTileData->RoomID != NextTileData->RoomID)
				{
					CurrentTileData->bConnections[Dir] = false;
					NextTileData->bConnections[(Dir + 3) % 6] = false;
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("BoundCoord : [%d, %d, %d]"), BoundCoord.Q, BoundCoord.R, BoundCoord.S);
		}
	}
}

void UTileManagerSubsystem::MakeEventData(const FStageData* StageData, const UStageInitData* StageInitData)
{
}

void UTileManagerSubsystem::MakeTileActor(const FStageData* StageData, const UStageInitData* StageInitData)
{
	for (auto& Pair : TileDataMap)
	{
		FVector WorldPosition = CubeCoordToWorldCoord(Pair.Key);
		//WorldPosition.Z = pair.Value.Floor * 40;

		TArray<ATile*> NonTopTiles;
		NonTopTiles.Reserve(Pair.Value.Floor);
		
		for (int32 Floor = 1; Floor <= Pair.Value.Floor; Floor++)
		{
			WorldPosition.Z += 40.f;
			
			ATile* TileActor = GetWorld()->SpawnActor<ATile>(StageData->TileBP, WorldPosition, FRotator(0, 0, 0));
	
			//타일의 중심 메쉬 결정을 위한 코드
			TArray<UStaticMesh*> TileMeshes;
			ETileMeshType Key = ETileMeshType::Main;
			
			//꼭대기 층이 아닌 모든 경우에, key + 1을 하여 해당 메쉬의 Under 버전으로 변경 
			if (Floor < Pair.Value.Floor)
			{
				Key = static_cast<ETileMeshType>(static_cast<int32>(Key) + 1);
			}

			TileMeshes.Emplace(StageData->TileMeshes[Key].LoadSynchronous());

			int32 Index = 0;
			//테두리 타일의 메쉬 결정을 위한 루프
			for (int32 Dir = 0; Dir < 6; ++Dir)
			{
				FCubeCoord Offset = FCubeCoord::GetDirection(Dir);
				
				Key = ETileMeshType::Side_Under3; //기본 선택은 막힌 메쉬로, 조건에 부합할 경우 해당 메쉬로 변경

				if (TileDataMap.Contains(Pair.Key + Offset) && TileDataMap[Pair.Key].bConnections[Index])
				{
					if (TileDataMap[Pair.Key + Offset].Floor > Pair.Value.Floor)
					{
						Key = ETileMeshType::Side_Upper;
					}
					else if (TileDataMap[Pair.Key + Offset].Floor < Pair.Value.Floor)
					{
						Key = ETileMeshType::Side_Lower;
					}
					else
					{
						if (TileDataMap[Pair.Key + Offset].RoomID == Pair.Value.RoomID) //같은 층 같은 룸이면 연결
						{
							Key = ETileMeshType::Side;
						}
					}
				}
				//꼭대기 층이 아닌 모든 경우에, key + 1을 하여 해당 메쉬의 Under 버전으로 변경 
				if (Floor < Pair.Value.Floor)
				{
					Key = static_cast<ETileMeshType>(static_cast<int32>(Key) + 1);
				}

				TileMeshes.Emplace(StageData->TileMeshes[Key].LoadSynchronous());
				++Index;
			}
			
			TileActor->Init(TileMeshes, Pair.Key, Pair.Value.RoomID, Floor == Pair.Value.Floor);

			if (Floor == Pair.Value.Floor)
			{
				// 좌표에 해당하는 TileData에 꼭대기 타일만 할당합니다.
				if (FTileData* FoundData = TileDataMap.Find(Pair.Key))
				{
					FoundData->TileActor = TileActor;
				}

				// 꼭대기 타일이 아닌 모든 타일에게 꼭대기 타일을 할당합니다.
				for (ATile* NonTopTile : NonTopTiles)
				{
					NonTopTile->SetTopTile(TileActor);
				}
				
				NonTopTiles.Empty();
			}
			else
			{
				NonTopTiles.Emplace(TileActor);
			}
		}
	}
}

void UTileManagerSubsystem::GetCoordFromRange(const FCubeCoord& CenterCoord, TArray<FCubeCoord>& OutCoordList, const int32 Width, const int32 Height) const
{
	OutCoordList.Reserve(Width * Height);
    
	const int32 HalfWidth = Width / 2;
	const int32 HalfHeight = Height / 2;

	for (int32 R = -HalfHeight; R <= HalfHeight; ++R)
	{
		const int32 QMin = FMath::Max(-HalfWidth, -R - HalfWidth);
		const int32 QMax = FMath::Min(HalfWidth, -R + HalfWidth);

		for (int32 Q = QMin; Q <= QMax; ++Q)
		{
			OutCoordList.Emplace(CenterCoord + FCubeCoord(Q, R));
		}
	}
}

FVector UTileManagerSubsystem::CubeCoordToWorldCoord(const FCubeCoord& Coord) const
{
	// 언리얼은 왼손 좌표계로, 검지가 X축, 중지가 Y축, 엄지가 Z축에 해당합니다.
	const float WorldX = TileHeightInterval * (-Coord.R);
	const float WorldY = TileWidthInterval * (Coord.Q + Coord.R * 0.5f);

	return FVector(WorldX, WorldY, 0.f);
}

void UTileManagerSubsystem::ShuffleArray(const FRandomStream* RandomStream, TArray<FCubeCoord>& Array) const
{
	const int32 LastIndex = Array.Num() - 1;
	for (int32 Index = LastIndex; Index > 0; --Index)
	{
		const int32 SwapIndex = RandomStream->FRandRange(0, Index);
		if (Index != SwapIndex)
		{
			Array.Swap(Index, SwapIndex);
		}
	}
}

ATile* UTileManagerSubsystem::GetTile(const FCubeCoord& InCubeCoord)
{
	if (const FTileData* TileData = TileDataMap.Find(InCubeCoord))
	{
		if (TileData->TileActor.IsValid())
		{
			return TileData->TileActor.Get();
		}
	}

	return nullptr;
}

bool UTileManagerSubsystem::MapTileAndActor(ATile* InTile, AActor* InActor)
{
	if (!InTile || !InActor)
	{
		return false;
	}

	UnmapByTile(InTile);
	UnmapByActor(InActor);
	
	TileToActorMap.Emplace(InTile, InActor);
	ActorToTileMap.Emplace(InActor, InTile);

	return true;
}

void UTileManagerSubsystem::UnmapByTile(ATile* InTile)
{
	if (!InTile)
	{
		return;
	}
	
	if (const TWeakObjectPtr<AActor>* Actor = TileToActorMap.Find(InTile))
	{
		if (Actor->IsValid())
		{
			ActorToTileMap.Remove(Actor->Get());
		}
	}
	TileToActorMap.Remove(InTile);
}

void UTileManagerSubsystem::UnmapByActor(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}
	
	if (const TWeakObjectPtr<ATile>* Tile = ActorToTileMap.Find(InActor))
	{
		if (Tile->IsValid())
		{
			TileToActorMap.Remove(Tile->Get());
		}
	}
	ActorToTileMap.Remove(InActor);
}

AActor* UTileManagerSubsystem::GetActorOnTile(const ATile* InTile) const
{
	if (TileToActorMap.Contains(InTile))
	{
		const TWeakObjectPtr<AActor> FoundActor = TileToActorMap.FindRef(InTile);
		if (FoundActor.IsValid())
		{
			return FoundActor.Get();
		}
	}

	return nullptr;
}

ATile* UTileManagerSubsystem::GetTileUnderActor(const AActor* InActor) const
{
	if (ActorToTileMap.Contains(InActor))
	{
		const TWeakObjectPtr<ATile> FoundTile = ActorToTileMap.FindRef(InActor);
		if (FoundTile.IsValid())
		{
			return FoundTile.Get();
		}
	}

	return nullptr;
}
