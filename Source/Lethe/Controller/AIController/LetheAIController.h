// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LetheAIController.generated.h"

enum class EPhaseState : uint8;
class ATile;
class UStateTreeAIComponent;

USTRUCT(BlueprintType)
struct FFoundTileData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	ATile* FoundTile = nullptr;

	UPROPERTY(BlueprintReadOnly)
	int32 Depth = 0;
};

UCLASS()
class LETHE_API ALetheAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALetheAIController();

	void SetAbilityPriority(const int32 InPriority);

	UFUNCTION(BlueprintCallable)
	FFoundTileData FindNearestPlayerCharacterTile() const;
	
	UFUNCTION(BlueprintCallable)
	void SelectRandomAbility() const;
	
	UFUNCTION(BlueprintCallable)
	void SelectMoveAbility() const;

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "선택된 Ability의 ActivationData에 TargetTile을 할당하는 함수로, 반드시 Ability를 선택한 후 호출해야 합니다."))
	void SetTargetTile(ATile* TargetTile) const;

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "선택된 MoveAbility의 ActivationData에 TargetTile을 할당하는 함수로, 반드시 MoveAbility를 선택한 후 호출해야 합니다."))
	void SetTargetTileToMove(ATile* CurrentTile, ATile* TargetTile);
	
	UFUNCTION(BlueprintCallable)
	TArray<ATile*> GetPathTiles(ATile* TargetTile) const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface
	
	//~ Begin AAIController Interface
	virtual void OnPossess(APawn* InPawn) override;
	//~ End of AAIController Interface

private:
	void OnPhaseStateChanged(const EPhaseState OldPhase, const EPhaseState NewPhase) const;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;
	
	// 임시로 구현된 이 AI의 사거리입니다.
	// TODO: 이곳에서 선언하지 않는 방향으로 수정될 수 있습니다. 예시) Attribute
	// TODO: 이곳에 선언한다 하더라도 주입은 외부에서 하거나 Blueprint에서 수정 가능한 형태로 구현하는 것이 좋습니다.
	UPROPERTY(BlueprintReadOnly)
	int32 Range = 1;
	
	// 임시로 구현된 이 AI의 이동 거리입니다.
	// TODO: 이곳에서 선언하지 않는 방향으로 수정될 수 있습니다. 예시) Attribute
	// TODO: 이곳에 선언한다 하더라도 주입은 외부에서 하거나 Blueprint에서 수정 가능한 형태로 구현하는 것이 좋습니다.
	UPROPERTY(BlueprintReadOnly)
	int32 MoveLength = 2;

private:
	// 이 값이 낮은 캐릭터가 먼저 Ability를 사용합니다.
	int32 AbilityPriority = 0;
};
