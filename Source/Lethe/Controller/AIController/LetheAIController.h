// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LetheAIController.generated.h"

class AArrowRenderer;
enum class EBFSType : uint8;
enum class EPhaseState : uint8;
class ATile;
class UStateTreeAIComponent;

UCLASS()
class LETHE_API ALetheAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALetheAIController();

	void SetAbilityPriority(const int32 InPriority);
	
	UFUNCTION(BlueprintCallable, meta = (ToolTip = "가장 가까운 플레이어 캐릭터를 찾아 그 타일들을 반환합니다. 거리가 같다면 여러 타일을 반환합니다."))
	int32 FindNearestPlayerCharacterTiles(const int32 MaxDepth, const EBFSType BFSType, TArray<ATile*>& OutNearestTiles);
	
	UFUNCTION(BlueprintCallable)
	void SelectRandomAbility(ATile* TargetTile) const;
	
	UFUNCTION(BlueprintCallable)
	void SelectMoveAbility(ATile* CurrentTile, ATile* TargetTile) const;

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "TargetTile로 이동하기 위한 최단 경로를 계산한 뒤, 그 모든 타일을 우선순위대로 정렬해 반환합니다."))
	void GetPrioritizedMoveTiles(const ATile* TargetTile, const int32 MoveDistance, TArray<ATile*>& OutPathTiles) const;

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
	void OnAbilityActivated(AActor* AbilityInstigator) const;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;

	UPROPERTY(EditDefaultsOnly, Category = "ArrowRenderer")
	TSubclassOf<AArrowRenderer> ArrowRendererClass;

	UPROPERTY()
	TObjectPtr<AArrowRenderer> ArrowRenderer;

private:
	// 이 값이 낮은 캐릭터가 먼저 Ability를 사용합니다.
	int32 AbilityPriority = 0;
};
