// Fill out your copyright notice in the Description page of Project Settings.


#include "TileSubsystem.h"

#include "LetheGameInstance.h"
#include "Tile.h"

void UTileSubsystem::MakeNewTileMap()
{
	//절차적 생성 맵 생성을 시작하는 함수 (GameMode에서 블루프린트 호출)
	//1. 맵 데이터를 초기화한다.
	//2. 타일에 층고를 만들기 위한 알고리즘을 실행한다.
	//3. 타일에 이벤트를 생성하기 위한 알고리즘을 실행한다.
	//4. 데이터를 기반으로 실제 액터를 생성한다.
	const FStageData* stageData = Cast<ULetheGameInstance>(GetWorld()->GetGameInstance())->GetStageData(FName("Forest"));
	const UStageInitData* stageInitData = stageData->StageInitData.LoadSynchronous();

	//맵 시드 이용해서 디버깅 용이하게 만듬
	FRandomStream randomStream;

	if (stageInitData->MapSeed == 0)
	{
		int32 rand = FMath::RandRange(0, 10000);
		UE_LOG(LogTemp, Warning, TEXT("Random Seed : %d"), rand);
		randomStream.Initialize(rand);
	}		
	else
	{
		randomStream.Initialize(stageInitData->MapSeed);
		UE_LOG(LogTemp, Warning, TEXT("Map Seed : %d"), stageInitData->MapSeed);
	}
	
	InitMapData(stageData, stageInitData);
	MakeFloorData(&randomStream, stageInitData);
	MakeEventData(stageData, stageInitData);
	MakeTileActor(stageData, stageInitData);

	
}

void UTileSubsystem::InitMapData(const FStageData* StageData, const UStageInitData* StageInitData)
{
	TMap<FCubeCoord, FTileData> tempDataMap;
	tempDataMap.Reserve(StageInitData->MapWidth * StageInitData->MapHeight); //공간 미리 잡는 함수
	
	//챗지피티가 최적화해준 버전
	TArray<FCubeCoord> coordList = GetCoordFromRange(FCubeCoord(0, 0, 0),
		StageInitData->MapWidth, StageInitData->MapHeight);

	for (auto& coord : coordList)
		tempDataMap.Add(coord, FTileData());
	
	TileDataMap = MoveTemp(tempDataMap); //적은 비용으로 복사하는 함수

	//타일 Mesh 미리 로딩
	for (auto& mesh : StageData->TileMeshes)
		mesh.Value.LoadSynchronous();
}

void UTileSubsystem::MakeFloorData(const FRandomStream* RandomStream, const UStageInitData* StageInitData)
{
	//현재는 층 수가 올라갈수록 타일 생성이 어려워지는 변수를 추가하지 않았음
	//맵 구성되는거 보고 최상단 층이 너무 넓다는 생각이 들면 추가하기
	TArray<FCubeCoord> cubeCoordList;
	TileDataMap.GetKeys(cubeCoordList);
	ShuffleArray(RandomStream, cubeCoordList); //Array 셔플
	UCurveFloat* curve = StageInitData->ProbabilityCurve.LoadSynchronous(); //확률 커브
	FRandomStream randomStream; //시드 값에 따라 랜덤 값 고정
	int32 roomID = 0;
	
	for (FCubeCoord& coord : cubeCoordList)
	{
		if (roomID >= StageInitData->FloorIncrementTrialsCount)
		{
			break;
		}

		FTileData* rootTileData = TileDataMap.Find(coord);
		
		if (rootTileData->Floor >= StageInitData->MaxFloor)
		{
			continue;			
		}

		int32 consecutiveCount = 0; //연속 시행 횟수
		int32 prevDepth = -1;
		int32 probability = 0;
		
		TSet<FCubeCoord> selectedCoords = TileBFS(coord, 10, EBFSType::Connection, 
	[&]() -> bool
		{
			return true;
		},
	[&](const FTileData* CurrentTileData, const int32 CurrentDepth) -> bool
		{
			if (CurrentTileData->RoomID != rootTileData->RoomID)
			{
				return false;
			}

			//타일 생성의 랜덤성을 위해 확률 커브, 랜덤 값 등을 활용
			//단, 랜덤성 때문에 이가 빠진 듯이 타일이 삐죽하게 생성되는걸 원치 않아서
			//확률에 한 번 선정되었을 경우 연속 횟수만큼은 확률 계산 하지 않고 통과하도록 함
		
			if (prevDepth != CurrentDepth) //Depth가 변경되는 페이즈에 잔여 연속 횟수 소진을 위해 필요함
			{
				prevDepth = CurrentDepth;
				probability = curve->GetFloatValue(CurrentDepth * 0.1f) * 100;
				consecutiveCount = 0;
			}

			int32 rand = 0;
		
			if (consecutiveCount == 0)
			{
				consecutiveCount = StageInitData->ConsecutiveTileCount;
				rand = RandomStream->FRandRange(0, 100);
			}
			
			if (rand < probability)
			{
				--consecutiveCount;
				return true;
			}
		
			return false;
		});

		if (selectedCoords.Num() <= 0)
		{
			continue;
		}

		++roomID;

		//선택된 영역들에 대해 값 변경
		for (auto& selectedCoord : selectedCoords)
		{
			FTileData* tileData = TileDataMap.Find(selectedCoord);
			tileData->RoomID = roomID;
			tileData->Floor++;
		}

		//테두리 검출 후 타일 연결 끊기
		TSet<FCubeCoord> boundCoords = TileBFS(coord, 15, EBFSType::Connection, 
	[&]() -> bool
	{
		return true;
	},
	[&](const FTileData* CurrentTileData, const int32 CurrentDepth) -> bool
		{
			if (CurrentTileData->RoomID != roomID) //이새끼가 문제임
			{
				return true;
			}

			return false;
		});

		for (FCubeCoord boundCoord : boundCoords)
		{
			for (int32 dir = 0; dir < 6; ++dir)
			{
				FTileData* currentTileData = TileDataMap.Find(boundCoord);
				FTileData* nextTileData = TileDataMap.Find(boundCoord + DirectionOffsets[dir]);

				if (nextTileData == nullptr)
				{
					continue;
				}

				if (currentTileData->RoomID != nextTileData->RoomID)
				{
					currentTileData->bConnections[dir] = false;
					nextTileData->bConnections[(dir + 3) % 6] = false;
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("boundCoord : [%d, %d, %d]"), boundCoord.Q, boundCoord.R, boundCoord.S);
		}
	}
}

void UTileSubsystem::MakeEventData(const FStageData* StageData, const UStageInitData* StageInitData)
{
}

void UTileSubsystem::MakeTileActor(const FStageData* StageData, const UStageInitData* StageInitData)
{
	TMap<FCubeCoord, ATile*> tempActorMap;

	for (auto& pair : TileDataMap)
	{
		
		FVector worldPosition = CubeCoordToWorldCoord(pair.Key);
		//worldPosition.Z = pair.Value.Floor * 40;

		ATile* tileClass = nullptr;
		
		for (int floor = 1; floor <= pair.Value.Floor; floor++)
		{
			worldPosition.Z += 40.f;
			
			AActor* tileActor = GetWorld()->SpawnActor<AActor>(StageData->TileBP, worldPosition, FRotator(0, 0, 0));
			tileClass = Cast<ATile>(tileActor);
	
			//타일의 중심 메쉬 결정을 위한 코드
			TArray<UStaticMesh*> tileMeshes;
			ETileMeshType key = ETileMeshType::Main;
			
			//꼭대기 층이 아닌 모든 경우에, key + 1을 하여 해당 메쉬의 Under 버전으로 변경 
			if (floor < pair.Value.Floor)
				key = static_cast<ETileMeshType>(static_cast<int32>(key) + 1);

			tileMeshes.Add(StageData->TileMeshes[key].Get());

			int32 index = 0;
			//테두리 타일의 메쉬 결정을 위한 루프
			for (auto& offset : DirectionOffsets)
			{				
				key = ETileMeshType::Side_Under3; //기본 선택은 막힌 메쉬로, 조건에 부합할 경우 해당 메쉬로 변경

				if (TileDataMap.Contains(pair.Key + offset) && TileDataMap[pair.Key].bConnections[index])
				{
					if (TileDataMap[pair.Key + offset].Floor > pair.Value.Floor)
					{
						key = ETileMeshType::Side_Upper;
					}
					else if (TileDataMap[pair.Key + offset].Floor < pair.Value.Floor)
					{
						key = ETileMeshType::Side_Lower;						
					}
					else
					{
						if (TileDataMap[pair.Key + offset].RoomID == pair.Value.RoomID) //같은 층 같은 룸이면 연결
						{
							key = ETileMeshType::Side;							
						}
					}
				}
				//꼭대기 층이 아닌 모든 경우에, key + 1을 하여 해당 메쉬의 Under 버전으로 변경 
				if (floor < pair.Value.Floor)
					key = static_cast<ETileMeshType>(static_cast<int32>(key) + 1);

				tileMeshes.Add(StageData->TileMeshes[key].Get());
				++index;
			}
			tileClass->Init(tileMeshes, pair.Key.Q, pair.Key.R, pair.Key.S, pair.Value.RoomID, floor == pair.Value.Floor);
		}
		TileActorMap.Add(pair.Key, tileClass);
	}
}

TArray<FCubeCoord> UTileSubsystem::GetCoordFromRange(const FCubeCoord& CenterCoord, int32 Width, int32 Height) const
{
	TArray<FCubeCoord> TempCoordList;
	TempCoordList.Reserve(Width * Height);
    
	const int32 HalfWidth = Width / 2;
	const int32 HalfHeight = Height / 2;

	for (int32 r = -HalfHeight; r <= HalfHeight; ++r)
	{
		int32 qMin = FMath::Max(-HalfWidth, -r - HalfWidth);
		int32 qMax = FMath::Min(HalfWidth, -r + HalfWidth);

		for (int32 q = qMin; q <= qMax; ++q)
		{
			TempCoordList.Add(CenterCoord + FCubeCoord(q, r));
		}
	}

	return TempCoordList;
}

FVector UTileSubsystem::CubeCoordToWorldCoord(const FCubeCoord& Coord)
{
	//언리얼 월드 좌표에서 X축은 위로, Y축은 오른쪽으로 향함.
	const float WorldX = TileHeightInterval * (-Coord.R);
	const float WorldY = TileWidthInterval * (Coord.Q + Coord.R * 0.5f);

	return FVector(WorldX, WorldY, 0.f);
}

void UTileSubsystem::ShuffleArray(const FRandomStream* RandomStream, TArray<FCubeCoord>& Array)
{
	const int32 lastIndex = Array.Num() - 1;
	for (int32 i = lastIndex; i > 0; --i)
	{
		int32 swapIndex = RandomStream->FRandRange(0, i);
		if (i != swapIndex)
		{
			Array.Swap(i, swapIndex);
		}
	}
}
