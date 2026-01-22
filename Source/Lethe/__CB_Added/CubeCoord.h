// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CubeCoord.generated.h"

/** 3차원 좌표를 표시하고, 직렬화를 지원하기 위해 만든 구조체
 * 
 */
USTRUCT(BlueprintType)
struct FCubeCoord
{
	GENERATED_BODY()

	//세 값의 합은 무조건 0
	UPROPERTY(EditAnywhere)
	int32 Q; // 우상단으로 향하면 +, 좌하단으로 향하면 -
	UPROPERTY(EditAnywhere) 
	int32 R; // 하단으로 향하면 +, 상단으로 향하면 - 
	UPROPERTY(EditAnywhere)
	int32 S; // 좌상단으로 향하면 +, 우하단으로 향하면 -

	FCubeCoord() : Q(0), R(0), S(0) {}
	
	constexpr FCubeCoord(const int32 InQ, const int32 InR)
		: Q(InQ), R(InR), S(-InQ - InR) {} // S는 계산식으로 항상 유지
	
	constexpr FCubeCoord(int32 InQ, int32 InR, int32 InS) : Q(InQ), R(InR), S(InS) {} //덧셈 연산을 위해

	FCubeCoord operator+(const FCubeCoord& Other) const
	{
		return FCubeCoord(Q + Other.Q, R + Other.R, S + Other.S);
	}
	
	bool operator==(const FCubeCoord& Other) const
	{
		return Q == Other.Q && R == Other.R && S == Other.S;
	}

};

FORCEINLINE uint32 GetTypeHash(const FCubeCoord& Coord)
{
	return HashCombine(HashCombine(::GetTypeHash(Coord.Q), ::GetTypeHash(Coord.R)), ::GetTypeHash(Coord.S));
}