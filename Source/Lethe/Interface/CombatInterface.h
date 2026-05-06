// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

class ATile;

UINTERFACE(NotBlueprintable)
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class LETHE_API ICombatInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	virtual int32 GetMoveDistance() const = 0;
	virtual int32 GetMaxMoveDistance() const = 0;
	
	UFUNCTION(BlueprintCallable)
	virtual void MoveToTile(UPARAM(ref)TArray<ATile*>& PathTiles, const bool bTeleport = false) = 0;

	/** Enemy만 재정의하는 함수로, Tile 상태에 따라 자신의 Hidden 상태를 갱신합니다. */
	virtual void UpdateHiddenByTile(const ATile* Tile);
	
	virtual void OnDamageTaken() = 0;
	virtual void Die() = 0;
	
	UFUNCTION(BlueprintCallable, meta = (ToolTip = "GA_Move에 할당된 MoveAnimation을 사용하지 않는 상황, 혹은 할당이 안 되어 있는 상황에 사용합니다."))
	virtual UAnimMontage* GetMoveAnimation() = 0;
};
