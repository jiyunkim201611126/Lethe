// Copyright JETBLU, Inc. All Rights Reserved.

#include "TileGenerator.h"

#include "Engine/World.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Util.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Data/Stage/StageData.h"

namespace TileGeneratorInternal
{
	//타일과 타일 사이의 간격
	static constexpr float TileWidthInterval = 173.205f;
	static constexpr float TileHeightInterval = 150.f;

	//크기 만큼의 좌표 영역을 반환
	void GetCoordFromRange(const FCubeCoord& CenterCoord, TArray<FCubeCoord>& OutCoordList, const int32 Width, const int32 Height)
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
				OutCoordList.Add(CenterCoord + FCubeCoord(Q, R));
			}
		}
	}

	//Cube좌표를 World좌표로 전환
	FVector CubeCoordToWorldCoord(const FCubeCoord& Coord)
	{
		// 언리얼은 왼손 좌표계로, 검지가 X축, 중지가 Y축, 엄지가 Z축에 해당합니다.
		const float WorldX = TileHeightInterval * (-Coord.R);
		const float WorldY = TileWidthInterval * (Coord.Q + Coord.R * 0.5f);

		return FVector(WorldX, WorldY, 0.f);
	}

	template <typename BFSConditionFunc, typename SelectConditionFunc>
	void TileBFS(TMap<FCubeCoord, FTileData>& TileDataMap, const FCubeCoord& StartCoord, const int32 MaxDepth, const EBFSType BFSType, TSet<FCubeCoord>& OutCoords, const BFSConditionFunc& BFSCondition, const SelectConditionFunc& SelectCondition)
	{
		TSet<FCubeCoord> Visited;
		
		if (!TileDataMap.Contains(StartCoord))
		{
			return;
		}
		
		TQueue<TPair<FCubeCoord, int32>> Queue;
		constexpr int32 StartDepth = 0;
		FTileData* CurrentTileData = TileDataMap.Find(StartCoord);
		Queue.Enqueue({StartCoord, StartDepth});
		
		while (!Queue.IsEmpty())
		{
			TPair<FCubeCoord, int32> Current;
			Queue.Dequeue(Current);
			const FCubeCoord& CurrentCoord = Current.Key;
			const int32 CurrentDepth = Current.Value;

			//현재 타일에 대한 조건 검사
			if (Visited.Contains(CurrentCoord))
			{
				continue;
			}

			Visited.Add(CurrentCoord);
			CurrentTileData = TileDataMap.Find(CurrentCoord);

			if (SelectCondition(CurrentCoord, CurrentTileData, CurrentDepth))
			{
				OutCoords.Add(CurrentCoord);
			}

			//뻗어 나갈 타일들에 대한 조건 검사
			if (MaxDepth < CurrentDepth + 1)
			{
				continue;
			}
			
			for (int32 Direction = 0; Direction < 6; ++Direction)
			{
				const FCubeCoord NextCoord = CurrentCoord + FCubeCoord::GetDirection(Direction);
				if (Visited.Contains(NextCoord))
				{
					continue;
				}

				const FTileData* NextTileData = TileDataMap.Find(NextCoord);
				if (!NextTileData)
				{
					continue;
				}
				
				// TODO: 탐색하지 않는다는 조건에 있어 필요한 매개변수들 여기서 넣어줘야 할 듯? 현재는 단순 true, false로만 해당 타일에서 더 BFS할지 말지 결정하는 중
				if (!BFSCondition(CurrentTileData, NextTileData))
				{
					continue;
				}
				
				switch (BFSType)
				{
					case EBFSType::Connection:
						if (!CurrentTileData->Connections[Direction])
						{
							continue;
						}
					break;
					case EBFSType::Through:
					break;
					default:
					break;
				}
				
				Queue.Enqueue({NextCoord, CurrentDepth + 1});
			}
		}
	}

	//맵 데이터 초기화
	void InitMapData(TMap<FCubeCoord, FTileData>& TileDataMap, const UStageInitData* StageInitData)
	{
		TileDataMap.Reserve(StageInitData->MapWidth * StageInitData->MapHeight); //공간 미리 잡는 함수
		
		TArray<FCubeCoord> CoordList;
		GetCoordFromRange(FCubeCoord(0, 0, 0), CoordList, StageInitData->MapWidth, StageInitData->MapHeight);

		for (auto& Coord : CoordList)
		{
			TileDataMap.Emplace(Coord, FTileData());
		}
	}

	//높낮이맵 제작 알고리즘
	void MakeFloorData(TMap<FCubeCoord, FTileData>& TileDataMap, const FRandomStream* RandomStream, const UStageInitData* StageInitData)
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
			if (StageInitData->FloorIncrementTrialsCount <= RoomId)
			{
				break;
			}

			FTileData* RootTileData = TileDataMap.Find(Coord);
			
			if (StageInitData->MaxFloor <= RootTileData->Floor)
			{
				continue;
			}

			int32 ConsecutiveCount = 0; //연속 시행 횟수
			int32 PrevDepth = -1;
			int32 Probability = 0;
			
			TSet<FCubeCoord> SelectedCoords;
			TileBFS(TileDataMap, Coord, 10, EBFSType::Connection, SelectedCoords,
				[](const FTileData* CurrentTileData, const FTileData* NextTileData)
				{
					return true;
				},
				[&](const FCubeCoord& CurrentCoord, const FTileData* CurrentTileData, const int32 CurrentDepth)
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
			TileBFS(TileDataMap, Coord, 15, EBFSType::Connection, BoundCoords,
				[](const FTileData* CurrentTileData, const FTileData* NextTileData)
				{
					return true;
				},
				[&RoomId](const FCubeCoord& CurrentCoord, const FTileData* CurrentTileData, const int32 CurrentDepth)
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

	//타일맵 제작 알고리즘
	void MakeEventData(TMap<FCubeCoord, FTileData>& TileDataMap, TMap<int32, FRoomData>& RoomDataMap, const FRandomStream* RandomStream, const UStageInitData* StageInitData)
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
	    	
	    	TileBFS(TileDataMap, Elem.Key, 999, EBFSType::Connection, CurrentCoords,
	    		[](const FTileData* CurrentTileData, const FTileData* NextTileData)
	    		{
	    			return true;
	    		},
	    		[](const FCubeCoord& CurrentCoord, const FTileData* CurrentTileData, const int32 CurrentDepth)
	    		{
	    			return true;
	    		});

	    	FCubeCoord TempCoord = FCubeCoord(0, 0, 0);
	    	
	    	for (FCubeCoord Coord : CurrentCoords)
	    	{
	    		TempCoord = TempCoord + Coord;
	    		TileDataMap[Coord].RoomId = RoomId;
	    	}

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
	    		FRoomData RoomData;
	    		RoomData.CenterCoord = *Closest;
	    		RoomData.RoomSize = CurrentCoords.Num();
	    		RoomDataMap.Emplace(RoomId, RoomData);
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
			
			TileBFS(TileDataMap, Elem.Value.CenterCoord, 999, EBFSType::Connection, BoundCoords,
			[](const FTileData* CurrentTileData, const FTileData* NextTileData)
			{
				return true;
			},
			[&TileDataMap](const FCubeCoord& CurrentCoord, const FTileData* CurrentTileData, const int32 CurrentDepth)
			{
				//6방향 검사해서 하나라도 다른 RoomId가 나오면, 테두리 타일로 검출
				for (int32 i = 0; i < 6; i++)
				{
					const FCubeCoord NextCoord = CurrentCoord + FCubeCoord::GetDirection(i);
					const FTileData* NextTileData = TileDataMap.Find(NextCoord);
	
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
			for (auto& Temp : TempTiles)
			{
				if (!BoundTiles.Contains(Temp.Key))
				{
					BoundTiles.Add(Temp.Key, Temp.Value);
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

					const FCubeCoord FromCoord = Pair.Key;
					const FCubeCoord ToCoord = Pair.Key + FCubeCoord::GetDirection(Pair.Value);

					FTileData& FromTileData = TileDataMap.FindChecked(FromCoord);
					FTileData& ToTileData = TileDataMap.FindChecked(ToCoord);

					// 두 타일을 연결합니다.
					FromTileData.Connections[Pair.Value] = true;
					ToTileData.Connections[(Pair.Value + 3) % 6] = true;

					// 서로 반대편 타일을 입구 타일로 기록합니다.
					FRoomData& FromRoomData = RoomDataMap.FindChecked(FromTileData.RoomId);
					FRoomData& ToRoomData = RoomDataMap.FindChecked(ToTileData.RoomId);
					FromRoomData.VisibleEntranceCoords.AddUnique(ToCoord);
					ToRoomData.VisibleEntranceCoords.AddUnique(FromCoord);
				}

				SelectCount = SelectCount + StageInitData->AverageConnectionPerSixWays;
			}
		}
	}

	//타일 생성
	void MakeTileActor(UWorld* World, TMap<FCubeCoord, FTileData>& TileDataMap, TMap<int32, FRoomData>& RoomDataMap, const FStageData* StageData)
	{
		TMap<FCubeCoord, TArray<TWeakObjectPtr<ATile>>> TilesByCoord;
		
		for (auto& Pair : TileDataMap)
		{
			FVector WorldPosition = CubeCoordToWorldCoord(Pair.Key);

			TArray<ATile*> NonTopTiles;
			NonTopTiles.Reserve(Pair.Value.Floor);
			
			for (int32 Floor = 1; Floor <= Pair.Value.Floor; Floor++)
			{
				WorldPosition.Z += 40.f;
				
				ATile* TileActor = World->SpawnActor<ATile>(StageData->TileBP, WorldPosition, FRotator::ZeroRotator);
				TilesByCoord.FindOrAdd(Pair.Key).Add(TileActor);
				if (FRoomData* RoomData = RoomDataMap.Find(Pair.Value.RoomId))
				{
					RoomData->RoomTiles.Add(TileActor);
				}
		
				//타일의 중심 메쉬 결정을 위한 코드
				TArray<UStaticMesh*> TileMeshes;
				TileMeshes.Reserve(7);
				ETileMeshType MeshKey = ETileMeshType::Main;
				
				//꼭대기 층이 아닌 모든 경우에, key + 1을 하여 해당 메쉬의 Under 버전으로 변경 
				if (Floor < Pair.Value.Floor)
				{
					MeshKey = static_cast<ETileMeshType>(static_cast<int32>(MeshKey) + 1);
				}

				TileMeshes.Add(StageData->TileMeshes[MeshKey].LoadSynchronous());

				int32 Index = 0;
				TArray<ETileConnectionState> UVOffsetType; // UV Offset 기능을 위해서 추가
				UVOffsetType.Init(ETileConnectionState::Block, 6);
				
				//테두리 타일의 메쉬 결정을 위한 루프
				for (int32 Direction = 0; Direction < 6; ++Direction)
				{
					const FCubeCoord Offset = FCubeCoord::GetDirection(Direction);
					
					MeshKey = ETileMeshType::Side_Under3; //기본 선택은 막힌 메쉬로, 조건에 부합할 경우 해당 메쉬로 변경

					const FTileData* CurrentTileData = TileDataMap.Find(Pair.Key);
					const FTileData* NeighborTileData = TileDataMap.Find(Pair.Key + Offset);
					if (NeighborTileData && CurrentTileData && CurrentTileData->Connections[Index])
					{
						if (Pair.Value.Floor < NeighborTileData->Floor)
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

					TileMeshes.Add(StageData->TileMeshes[MeshKey].LoadSynchronous());
					++Index;
				}

				if (Floor == Pair.Value.Floor)
				{
					// 좌표에 해당하는 TileData에 꼭대기 타일만 할당합니다.
					if (FTileData* TileData = TileDataMap.Find(Pair.Key))
					{
						TileData->TileActor = TileActor;
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
					NonTopTiles.Add(TileActor);
				}
				
				TileActor->Init(TileMeshes, Pair.Key, Pair.Value.RoomId, UVOffsetType);
			}
		}

		// 입구 타일을 실제로 추가합니다.
		for (auto& Pair : RoomDataMap)
		{
			FRoomData& RoomData = Pair.Value;
			RoomData.VisibleEntranceTiles.Reset();
			
			for (const FCubeCoord& EntranceCoord : RoomData.VisibleEntranceCoords)
			{
				if (const auto* Tiles = TilesByCoord.Find(EntranceCoord))
				{
					RoomData.VisibleEntranceTiles.Append(*Tiles);
				}
			}
		}
	}
}

bool FTileGenerator::GenerateTileMap(UWorld* World, const FStageData* StageData, const UStageInitData* StageInitData, FTileGenerationResult& OutResult)
{
	//1. 맵 데이터를 초기화한다.
	//2. 타일에 층고를 만들기 위한 알고리즘을 실행한다.
	//3. 타일에 이벤트를 생성하기 위한 알고리즘을 실행한다.
	//4. 데이터를 기반으로 실제 액터를 생성한다.
	
	OutResult.TileDataMap.Reset();
	OutResult.RoomDataMap.Reset();

	if (!World || !StageData || !StageInitData)
	{
		return false;
	}

	//맵 시드 이용해서 디버깅 용이하게 만듬
	FRandomStream RandomStream;

	if (StageInitData->MapSeed == 0)
	{
		const int32 RandomSeed = FMath::RandRange(0, 10000);
		LETHE_LOG(LogTile, Log, "Random Seed : %d", RandomSeed);
		RandomStream.Initialize(RandomSeed);
	}
	else
	{
		RandomStream.Initialize(StageInitData->MapSeed);
		LETHE_LOG(LogTile, Log, "Map Seed : %d", StageInitData->MapSeed);
	}

	TileGeneratorInternal::InitMapData(OutResult.TileDataMap, StageInitData);
	TileGeneratorInternal::MakeFloorData(OutResult.TileDataMap, &RandomStream, StageInitData);
	TileGeneratorInternal::MakeEventData(OutResult.TileDataMap, OutResult.RoomDataMap, &RandomStream, StageInitData);
	TileGeneratorInternal::MakeTileActor(World, OutResult.TileDataMap, OutResult.RoomDataMap, StageData);

	return true;
}
