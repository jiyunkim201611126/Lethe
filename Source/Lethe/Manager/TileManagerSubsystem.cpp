// Fill out your copyright notice in the Description page of Project Settings.

#include "TileManagerSubsystem.h"

#include "Lethe/Util.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Data/Stage/StageData.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"


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
			MakeEventData(&RandomStream, StageInitData);
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
	ArrayShuffle::ShuffleWithSeed(CubeCoordList, *RandomStream); //Array 셔플
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
			[](const FTileData* CurrentTileData, const FTileData* NextTileData)
			{
				return true;
			},
			[&](const FCubeCoord CurrentCoord, const FTileData* CurrentTileData, const int32 CurrentDepth)
			{
				if (CurrentTileData->RoomId != RootTileData->RoomId)
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
			TileData->RoomId = RoomId;
			TileData->Floor++;
		}

		//테두리 검출 후 타일 연결 끊기
		TSet<FCubeCoord> BoundCoords;
		TileBFS(Coord, 15, EBFSType::Connection, BoundCoords,
			[](const FTileData* CurrentTileData, const FTileData* NextTileData)
			{
				return true;
			},
			[&RoomId](const FCubeCoord CurrentCoord, const FTileData* CurrentTileData, const int32 CurrentDepth)
			{
				if (CurrentTileData->RoomId != RoomId) //이새끼가 문제임
				{
					return true;
				}

				return false;
			});

		for (FCubeCoord BoundCoord : BoundCoords)
		{
			for (int32 Direction = 0; Direction < 6; ++Direction)
			{
				FTileData* CurrentTileData = TileDataMap.Find(BoundCoord);
				FTileData* NextTileData = TileDataMap.Find(BoundCoord + FCubeCoord::GetDirection(Direction));

				if (!CurrentTileData || !NextTileData)
				{
					continue;
				}

				if (CurrentTileData->RoomId != NextTileData->RoomId)
				{
					CurrentTileData->Connections[Direction] = false;
					NextTileData->Connections[(Direction + 3) % 6] = false;
				}
			}
		}
	}
}

void UTileManagerSubsystem::MakeEventData(const FRandomStream* RandomStream, const UStageInitData* StageInitData)
{
		//모든 Room 순회하면서 새로운 RoomId 부여
		//모든 Room은 초기에 고립되어 있으므로, 추가 조건 없이 고립 구역만 검출하면 Room 검출 가능
    	TSet<FCubeCoord> VisitedCoords;
    	int RoomId = 0;
    
    	for (auto& Elem : TileDataMap)
    	{
    		if (VisitedCoords.Contains(Elem.Key))
    		{
    			continue;			
    		}
    
    		TSet<FCubeCoord> CurrentCoords;
    		
    		TileBFS(Elem.Key, 999, EBFSType::Connection, CurrentCoords,
    			[](const FTileData* CurrentTileData, const FTileData* NextTileData)
    		{
    			return true;
    		},
    		[](const FCubeCoord CurrentCoord, const FTileData* CurrentTileData, const int32 CurrentDepth)
    		{
    			return true;
    		});

    		FCubeCoord TempCoord = FCubeCoord(0, 0, 0);
    		
    		for (FCubeCoord coord : CurrentCoords)
    		{
    			TempCoord = TempCoord + coord;
    			TileDataMap[coord].RoomId = RoomId;
    		}

    		{
    			int32 AverageQ = FMath::RoundToInt(static_cast<float>(TempCoord.Q) / CurrentCoords.Num());
    			int32 AverageR = FMath::RoundToInt(static_cast<float>(TempCoord.R) / CurrentCoords.Num());
    			int32 AverageS = FMath::RoundToInt(static_cast<float>(TempCoord.S) / CurrentCoords.Num());

    			TempCoord = FCubeCoord(AverageQ, AverageR, AverageS);

    			//최소값 탐색
    			const FCubeCoord* Closest = Algo::MinElementBy(CurrentCoords, [&](const FCubeCoord& Coord)
				{
					return FCubeCoord::Distance(Coord, TempCoord);
				});

    			if (Closest)
    			{
    				RoomDataMap.Emplace(RoomId, FRoomData(CurrentCoords.Num(), *Closest));
    			}    			
    		}
    		
    		VisitedCoords.Append(CurrentCoords);
    		RoomId++;
    	}

		//테두리 타일 검출 BFS, 통과 직후에는 각 Room마다 테두리 타일이 검출되므로, 양뱡향 연결시 기준으로 중복 값이 들어있음
		//Key : {RoomId(From), RoomId(To)}, Value : {CurrentTileCoord, Dir}
		TMap<TPair<int32, int32>, TArray<TPair<FCubeCoord, int32>>> BoundTiles;
	
		for (auto& Elem : RoomDataMap)
		{
			TSet<FCubeCoord> BoundCoords;
			
			TileBFS(Elem.Value.CenterCoords, 999, EBFSType::Connection, BoundCoords,
			[](const FTileData* CurrentTileData, const FTileData* NextTileData)
			{
				return true;
			},
			[this](const FCubeCoord CurrentCoord, const FTileData* CurrentTileData, const int32 CurrentDepth)
			{
				//6방향 검사해서 하나라도 다른 RoomId가 나오면, 테두리 타일로 검출
				for (int32 i = 0; i < 6; i++)
				{
					FCubeCoord NextCoord = CurrentCoord + FCubeCoord::GetDirection(i);
					FTileData* NextTileData = TileDataMap.Find(NextCoord);
	
					if (NextTileData && FMath::Abs(CurrentTileData->Floor - NextTileData->Floor) <= 1 && CurrentTileData->RoomId != NextTileData->RoomId)
					{
						return true;
					}
				}

				return false;
			});

			TMap<TPair<int32, int32>, TArray<TPair<FCubeCoord, int32>>> TempTiles;

			//각 테두리 타일의 Direction을 포함하여 연결될 수 있는 가능성을 가진 길 자체를 배열의 요소로 담음
			for (FCubeCoord Coord : BoundCoords)
			{				
				FTileData* CurrentTileData = TileDataMap.Find(Coord);

				if (CurrentTileData)
				{
					for (int Dir = 0; Dir < 6; Dir++)
					{
						FTileData* NextTileData = TileDataMap.Find(Coord + FCubeCoord::GetDirection(Dir));

						if (NextTileData && CurrentTileData->RoomId != NextTileData->RoomId && FMath::Abs(CurrentTileData->Floor - NextTileData->Floor) <= 1)
						{
							int32 KeyA = FMath::Min(CurrentTileData->RoomId, NextTileData->RoomId); //중복값 검출을 위해 RoomId(From)과 RoomId(To)를 오름차순 정렬
							int32 KeyB = FMath::Max(CurrentTileData->RoomId, NextTileData->RoomId);

							TempTiles.FindOrAdd({KeyA, KeyB}).Add({Coord, Dir}); //Add 대신 Emplace 안됨
						}
					}
				}
			}

			//양방향으로 연결했을 때, 건너편 Room의 테두리 타일 정보는 필요하지 않으므로 중복 값 삭제
			for (auto& temp : TempTiles)
			{
				if (!BoundTiles.Contains(temp.Key))
				{
					BoundTiles.Add(temp.Key, temp.Value);
				}				
			}
		}

		//낮은 확률이지만, 주변 모든 공간이 Floor 차이가 2 이상인 공간이 간혹 생김. 이걸 구덩이라고 생각하고 용납할건지? 당장은 미관상 나쁘지 않은듯 (몬스터 혹은 구조물 생성시 주의)
		for (auto& Elem : BoundTiles)
		{
			TArray<TPair<FCubeCoord, int32>> RandArray = Elem.Value;
			ArrayShuffle::ShuffleWithSeed(RandArray, *RandomStream); //Array 셔플

			int32 SelectCount = 6; //최소 1개의 길 보장

			for (auto& Pair : RandArray)
			{
				if (SelectCount / 6)
				{
					SelectCount -= 6;
					TileDataMap.FindChecked(Pair.Key).Connections[Pair.Value] = true;
					TileDataMap.FindChecked(Pair.Key + FCubeCoord::GetDirection(Pair.Value)).Connections[(Pair.Value + 3) % 6] = true; //양방향 연결
				}

				SelectCount = SelectCount + StageInitData->AverageConnectionPerSixWays;
			}
		}
}

void UTileManagerSubsystem::MakeTileActor(const FStageData* StageData, const UStageInitData* StageInitData)
{
	for (auto& Pair : TileDataMap)
	{
		FVector WorldPosition = CubeCoordToWorldCoord(Pair.Key);

		TArray<ATile*> NonTopTiles;
		NonTopTiles.Reserve(Pair.Value.Floor);
		
		for (int32 Floor = 1; Floor <= Pair.Value.Floor; Floor++)
		{
			WorldPosition.Z += 40.f;
			
			ATile* TileActor = GetWorld()->SpawnActor<ATile>(StageData->TileBP, WorldPosition, FRotator::ZeroRotator);
	
			//타일의 중심 메쉬 결정을 위한 코드
			TArray<UStaticMesh*> TileMeshes;
			TileMeshes.Reserve(7);
			ETileMeshType MeshKey = ETileMeshType::Main;
			
			//꼭대기 층이 아닌 모든 경우에, key + 1을 하여 해당 메쉬의 Under 버전으로 변경 
			if (Floor < Pair.Value.Floor)
			{
				MeshKey = static_cast<ETileMeshType>(static_cast<int32>(MeshKey) + 1);
			}

			TileMeshes.Emplace(StageData->TileMeshes[MeshKey].LoadSynchronous());

			int32 Index = 0;
			TArray<ETileConnectionState> UVOffsetType; // UV Offset 기능을 위해서 추가
			UVOffsetType.Init(ETileConnectionState::Block, 6);
			
			//테두리 타일의 메쉬 결정을 위한 루프
			for (int32 Direction = 0; Direction < 6; ++Direction)
			{
				FCubeCoord Offset = FCubeCoord::GetDirection(Direction);
				
				MeshKey = ETileMeshType::Side_Under3; //기본 선택은 막힌 메쉬로, 조건에 부합할 경우 해당 메쉬로 변경

				const FTileData* CurrentTileData = TileDataMap.Find(Pair.Key);
				const FTileData* NeighborTileData = TileDataMap.Find(Pair.Key + Offset);
				if (NeighborTileData && CurrentTileData && CurrentTileData->Connections[Index])
				{
					if (NeighborTileData->Floor > Pair.Value.Floor)
					{
						MeshKey = ETileMeshType::Side_Upper;
						UVOffsetType[Direction] = ETileConnectionState::VerticalConnected;
					}
					else if (NeighborTileData->Floor < Pair.Value.Floor)
					{
						MeshKey = ETileMeshType::Side_Lower;
						UVOffsetType[Direction] = ETileConnectionState::VerticalConnected;
					}
					else
					{
						MeshKey = ETileMeshType::Side;
						UVOffsetType[Direction] = ETileConnectionState::Connected;
					}
				}
				
				//꼭대기 층이 아닌 모든 경우에, key + 1을 하여 해당 메쉬의 Under 버전으로 변경 
				if (Floor < Pair.Value.Floor)
				{
					MeshKey = static_cast<ETileMeshType>(static_cast<int32>(MeshKey) + 1);
				}

				TileMeshes.Emplace(StageData->TileMeshes[MeshKey].LoadSynchronous());
				++Index;
			}

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
			
			TileActor->Init(TileMeshes, Pair.Key, Pair.Value.RoomId, UVOffsetType);
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

bool UTileManagerSubsystem::FindShortestPath(const ATile* StartTile, const ATile* EndTile, TArray<ATile*>& OutPathTiles)
{
	OutPathTiles.Reset();
	if (!StartTile || !EndTile)
	{
		return false;
	}

	const FCubeCoord StartCoord = StartTile->GetCubeCoord();
	const FCubeCoord EndCoord = EndTile->GetCubeCoord();

	if (!TileDataMap.Contains(StartCoord) || !TileDataMap.Contains(EndCoord))
	{
		return false;
	}

	if (StartCoord == EndCoord)
	{
		return false;
	}

	// 다음에 탐색할 좌표입니다.
	TQueue<FCubeCoord> NextCoordsQueue;
	// 이미 방문한 좌표입니다.
	TSet<FCubeCoord> VisitedCoords;
	// '이 좌표에 어디서 왔는지'를 저장, 경로 복원에 사용합니다.
	// 즉 Key는 다음 좌표, Value는 현재 좌표로 기록됩니다.
	TMap<FCubeCoord, FCubeCoord> ParentCoordMap;

	NextCoordsQueue.Enqueue(StartCoord);
	VisitedCoords.Emplace(StartCoord);

	bool bFoundPath = false;

	while (!NextCoordsQueue.IsEmpty())
	{
		// 다음 방문할 좌표를 꺼냅니다.
		FCubeCoord CurrentCoord;
		NextCoordsQueue.Dequeue(CurrentCoord);

		// 해당 좌표에 데이터가 존재하지 않으면 스킵합니다.
		const FTileData* CurrentTileData = TileDataMap.Find(CurrentCoord);
		if (!CurrentTileData)
		{
			continue;
		}

		// 해당 좌표의 6방향 타일들을 검사합니다.
		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			// 연결 여부를 검사하고, 막혀있다면 스킵합니다.
			if (!CurrentTileData->Connections[Direction])
			{
				continue;
			}

			// 이미 방문한 좌표거나 데이터가 없는 좌표라면 스킵합니다.
			const FCubeCoord NextCoord = CurrentCoord + FCubeCoord::GetDirection(Direction);
			if (VisitedCoords.Contains(NextCoord) || !TileDataMap.Contains(NextCoord))
			{
				continue;
			}

			// 방문했음을 기록하고, 다음 좌표로 가는 타일로 현재 타일을 등록합니다.
			VisitedCoords.Emplace(NextCoord);
			ParentCoordMap.Emplace(NextCoord, CurrentCoord);

			// 다음 좌표가 목적지라면 탐색을 중단합니다.
			if (NextCoord == EndCoord)
			{
				bFoundPath = true;
				break;
			}

			// 다음 좌표를 큐에 넣어 탐색을 계속합니다.
			NextCoordsQueue.Enqueue(NextCoord);
		}

		if (bFoundPath)
		{
			break;
		}
	}

	if (!bFoundPath)
	{
		return false;
	}

	TArray<FCubeCoord> ReversedPathCoords;
	ReversedPathCoords.Reserve(VisitedCoords.Num());

	FCubeCoord CurrentCoord = EndCoord;
	ReversedPathCoords.Emplace(CurrentCoord);

	// EndCoord부터 지금까지 거쳐왔던 경로를 거꾸로 다시 돌아가며 해당 경로를 기록합니다.
	while (CurrentCoord != StartCoord)
	{
		const FCubeCoord* ParentCoord = ParentCoordMap.Find(CurrentCoord);
		if (!ParentCoord)
		{
			OutPathTiles.Reset();
			return false;
		}

		CurrentCoord = *ParentCoord;
		ReversedPathCoords.Emplace(CurrentCoord);
	}

	// 경로가 거꾸로 기록되었으므로, 다시 뒤집어서 Out인자로 뱉어줍니다.
	for (int32 Index = ReversedPathCoords.Num() - 2; Index >= 0; --Index)
	{
		if (ATile* PathTile = GetTile(ReversedPathCoords[Index]))
		{
			OutPathTiles.Emplace(PathTile);
		}
	}

	return !OutPathTiles.IsEmpty();
}

void UTileManagerSubsystem::AddToReservedMoveTiles(ATile* Tile)
{
	ReservedMoveTiles.Emplace(Tile);
}

void UTileManagerSubsystem::RemoveToReservedMoveTiles(ATile* Tile)
{
	ReservedMoveTiles.RemoveSwap(Tile);
}

void UTileManagerSubsystem::EmptyReservedMoveTiles()
{
	ReservedMoveTiles.Empty();
}

bool UTileManagerSubsystem::CanMoveToTile(ATile* Tile) const
{
	const bool bIsReserved = ReservedMoveTiles.Contains(Tile);
	bool bIsPlayerCharacter = false;
	if (const AActor* ActorOnTile = GetActorOnTile(Tile))
	{
		bIsPlayerCharacter = ActorOnTile->Implements<UPlayableCharacterInterface>();
	}
	return !bIsReserved && !bIsPlayerCharacter;
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
