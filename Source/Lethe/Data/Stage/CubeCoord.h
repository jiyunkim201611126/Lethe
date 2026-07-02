// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CubeCoord.generated.h"

/**
 * 3차원 좌표를 표시하고, 직렬화를 지원하기 위해 만든 구조체
 * Q, R, S의 합은 무조건 0
 */
USTRUCT(BlueprintType)
struct FCubeCoord
{
	GENERATED_BODY()

public:
	/** 우상단, 우측으로 향하면 +, 좌하단, 좌측으로 향하면 - */
	UPROPERTY(EditAnywhere)
	int32 Q;
	/** 우하단, 좌하단으로 향하면 +, 우상단, 좌상단으로 향하면 - */ 
	UPROPERTY(EditAnywhere) 
	int32 R;
	/** 좌상단, 좌측으로 향하면 +, 우하단, 우측으로 향하면 - */
	UPROPERTY(EditAnywhere)
	int32 S;
	
	static constexpr int32 HexDirectionCount = 6;

	static float GetTileWidthInterval()
	{
		return TileWidthInterval;
	}

	FCubeCoord() : Q(0), R(0), S(0) {}
	
	constexpr FCubeCoord(const int32 InQ, const int32 InR)
		: Q(InQ), R(InR), S(-InQ - InR) {} // S는 계산식으로 항상 유지
	
	constexpr FCubeCoord(const int32 InQ, const int32 InR, const int32 InS)
		: Q(InQ), R(InR), S(InS) {} //덧셈 연산을 위해

	FCubeCoord operator+(const FCubeCoord& Other) const
	{
		return FCubeCoord(Q + Other.Q, R + Other.R, S + Other.S);
	}
	
	bool operator==(const FCubeCoord& Other) const
	{
		return Q == Other.Q && R == Other.R && S == Other.S;
	}
	
	//각 방향으로의 오프셋값, ETileDirection과 조합해서 사용
	static FCubeCoord GetDirection(const int32 DirectionIndex)
	{
		static constexpr FCubeCoord DirectionOffsets[HexDirectionCount] =
		{
			FCubeCoord(+0, -1), // LeftTop
			FCubeCoord(-1, +0), // Left
			FCubeCoord(-1, +1), // LeftBottom
			FCubeCoord(+0, +1), // RightBottom
			FCubeCoord(+1, +0), // Right
			FCubeCoord(+1, -1), // RightTop
		};
		return DirectionOffsets[DirectionIndex % HexDirectionCount];
	}

	//거리 계산
	static int32 Distance(const FCubeCoord& A, const FCubeCoord& B)
	{
		return (FMath::Abs(A.Q - B.Q) + FMath::Abs(A.R - B.R) + FMath::Abs(A.S - B.S)) / 2;
	}
	
	//CubeCoord를 기반으로 월드 좌표를 반환
	static FVector CubeCoordToWorldCoord(const FCubeCoord& Coord, const int32 Floor = 0)
	{
		// 언리얼은 왼손 좌표계로, 검지가 X축, 중지가 Y축, 엄지가 Z축에 해당합니다.
		const float WorldX = TileHeightInterval * (-Coord.R);
		const float WorldY = TileWidthInterval * (Coord.Q + Coord.R * 0.5f);
		const float WorldZ = TileFloorInterval * Floor;

		return FVector(WorldX, WorldY, WorldZ);
	}
	
private:
	//타일과 타일 사이의 간격
	static constexpr float TileWidthInterval = 173.205f;
	static constexpr float TileHeightInterval = 150.f;
	static constexpr float TileFloorInterval = 40.f;
};

FORCEINLINE uint32 GetTypeHash(const FCubeCoord& Coord)
{
	return HashCombine(HashCombine(::GetTypeHash(Coord.Q), ::GetTypeHash(Coord.R)), ::GetTypeHash(Coord.S));
}
