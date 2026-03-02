// Fill out your copyright notice in the Description page of Project Settings.

#include "TileManagerSubsystem.h"

#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Data/Stage/StageData.h"
#include "Lethe/Data/Stage/RoomData.h"

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
	//1. 모든 타일을 순차적으로 BFS를 돌아서 RoomNum 기준으로 묶는다.
    	//2. 서로 다른 RoomNum을 가진 지역이 있을 경우 (대부분 고립된 지역임) 더 큰 Size를 가진 쪽이 RoomNum을 승계한다.
    	//3. 고립된 지역에 새로운 RoomNum을 부여한다.
    	//4. Room 단위로 BFS를 돌아서 길이 연결될 가능성이 있는 구역을 탐색한다 (Floor 차이가 1이면서 RoomNum이 다른 구역)
    	//5. 각 Room마다 적절한 양의 길을 연결한다. 알고리즘이 잘 짜여졌다면 추가로 고립 여부를 BFS로 확인할 필요가 없다.
    	//7. 구조물 및 이벤트 룸을 정의하고 조건을 작성한다. Floor, RoomSize 등이 조건이 되며, 핵심 구조물 및 이벤트 타일이 배치 가능한지 우선 확인해야 한다.
    	//8. 조건이 맞는 Room에 필수 Event부터 선택하여 배치한다.
    	//9. 이벤트가 없는 나머지 룸들에 비필수 Event들을 랜덤하게 배치한다.
    	//10. 아무 이벤트가 없는 룸들에게 약간의 이벤트라도 배치한다.
    
    	//RoomSize, RoomNum 등을 가진 Room 구조체 필요
    	//예전에 만들었다 지웠던 RoomInitData 데이터 에셋 필요

		//모든 Room은 초기에 고립되어 있으므로, 추가 조건 없이 고립 구역만 검출하면 Room 검출 가능. 이후 새 인덱스 부여
    	TMap<int32, FRoomData> RoomDataMap;
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

    			RoomDataMap.Emplace(RoomId, FRoomData(CurrentCoords.Num(), TempCoord));
    		}
    		
    		VisitedCoords.Append(CurrentCoords);
    		RoomId++;
    	}

		//Key : RoomId, Value : 테두리 타일의 Coord 집합
		TMap<int32, TSet<FCubeCoord>> BoundTiles;
	
		for (auto& Elem : RoomDataMap)
		{
			TSet<FCubeCoord> BoundCoords;
			
			TileBFS(Elem.Value.CenterCoords, 999, EBFSType::Connection, BoundCoords,
			[](const FTileData* CurrentTileData, const FTileData* NextTileData)
			{
				if (CurrentTileData->RoomId == NextTileData->RoomId)
				{
					return true;
				}
				
				return false;
			},
			[this](const FCubeCoord CurrentCoord, const FTileData* CurrentTileData, const int32 CurrentDepth)
			{
				//6방향 검사해서 하나라도 다른 RoomId가 나오면, 테두리 타일로 검출
				for (int32 i = 0; i < 6; i++)
				{
					FCubeCoord NextCoord = CurrentCoord + FCubeCoord::GetDirection(i);
					FTileData* NextTileData = TileDataMap.Find(NextCoord);

					if (NextTileData && abs(CurrentTileData->Floor - NextTileData->Floor) == 1 && CurrentTileData->RoomId != NextTileData->RoomId)
					{
						return true;
					}
				}

				return false;
			});

			BoundTiles.Emplace(Elem.Key, BoundCoords);
		}

		for (auto& Elem : BoundTiles)
		{
			TArray<FCubeCoord> RandArray = Elem.Value.Array();
			ShuffleArray(RandomStream, RandArray); //순서 보장을 위해 한 번 섞음

			int32 SelectCount = 6; //최소 1개의 길 보장

			//6방향 연결점을 랜덤으로 접근해서, 막힌 길을 만날 때마다 길을 뚫기 위해 시도
			for (auto& Coord : RandArray)
			{
				TArray<int32> RandDir = {0, 1, 2, 3, 4, 5};
				ShuffleArray(RandomStream, RandDir);

				for (int32 Dir : RandDir)
				{
					FTileData* CurrentTileData = TileDataMap.Find(Coord);
					
					if (CurrentTileData && !CurrentTileData->Connections[Dir])
					{
						FTileData* NextTileData = TileDataMap.Find(Coord + FCubeCoord::GetDirection(Dir));

						if (NextTileData && abs(CurrentTileData->Floor - NextTileData->Floor) == 1 && SelectCount / 6)
						{
							SelectCount -= 6;
							NextTileData->Connections[(Dir + 3) % 6] = true;
							CurrentTileData->Connections[Dir] = true;
						}

						SelectCount = SelectCount + StageInitData->AverageConnectionPerSixWays;
					}					
				}
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
						if (NeighborTileData->RoomId == Pair.Value.RoomId) //같은 층 같은 룸이면 연결
						{
							MeshKey = ETileMeshType::Side;
							UVOffsetType[Direction] = ETileConnectionState::Connected;
						}
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

void UTileManagerSubsystem::ShuffleArray(const FRandomStream* RandomStream, TArray<int32>& Array) const
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
