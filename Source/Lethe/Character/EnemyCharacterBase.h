// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheCharacterBase.h"
#include "Lethe/Data/Stage/TileData.h"
#include "EnemyCharacterBase.generated.h"

UCLASS()
class LETHE_API AEnemyCharacterBase : public ALetheCharacterBase
{
	GENERATED_BODY()

public:
	void SetEnemyAbilityPriority(const int32 InPriority) const;

protected:
	// 캐릭터의 사정거리입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status")
	FAbilityRange AbilityRange;

	// 캐릭터의 최대 이동 거리입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status")
	int32 MoveLength = 2;
};
