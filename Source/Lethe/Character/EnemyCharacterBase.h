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
	// TODO: 현재로선 간단하게 구현하기 위해 이곳에 선언되었으나, 추후 Ability에 선언된 AbilityRange를 사용하도록 구현할 필요가 있습니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status")
	FAbilityRange AbilityRange;

	// 캐릭터의 최대 이동 거리입니다.
	// TODO: 현재로선 간단하게 구현하기 위해 이곳에 선언되었으나, 추후 Attribute로 옮길 필요가 있습니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status")
	int32 MoveDistance = 2;
};
