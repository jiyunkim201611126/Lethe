// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayAbilitySpecHandle.h"
#include "LetheAIController.generated.h"

enum class EBFSType : uint8;
class AArrowRenderer;
class ATile;
class UActorSelectorComponent;
class UStateTreeAIComponent;

UCLASS()
class LETHE_API ALetheAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALetheAIController();

	void ProcessPlanPhase() const;
	void ProcessCommitPlan() const;

	void DeactivateArrow() const;
	
	UFUNCTION(BlueprintCallable, meta = (ToolTip = "가장 가까운 플레이어 캐릭터를 찾아 그 타일들을 반환합니다. 거리가 같다면 여러 타일을 반환합니다."))
	int32 FindNearestPlayerCharacterTiles(const EBFSType BFSType, const int32 MaxDepth, TArray<ATile*>& OutTiles);
	
	UFUNCTION(BlueprintCallable, meta = (ToolTip = "부여된 CardAbility 중 하나를 랜덤으로 선택합니다."))
	bool GetRandomAbility(FGameplayAbilitySpecHandle& OutAbilitySpecHandle);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Ability로 TargetActor을 공격할 수 있는 타일들을 반환합니다."))
	bool GetAttackOriginTiles(const FGameplayAbilitySpecHandle AbilitySpecHandle, const ATile* TargetTile, TArray<ATile*>& OutAttackOriginTiles);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "공격 가능한 타일 중에서 가장 공격하기 좋은 타일을 반환합니다."))
	ATile* SelectBestAttackOriginTile(const TArray<ATile*>& AttackOriginTiles);
	
	UFUNCTION(BlueprintCallable, meta = (ToolTip = "이번 턴에 사용할 AbilitySpecHandle과 대상 타일을 세팅합니다."))
	void SetPlannedData(const FGameplayAbilitySpecHandle SelectedAbilitySpecHandle, ATile* TargetTile);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "정해진 범위 내에 랜덤한, 위에 아무것도 없는 타일을 반환합니다."))
	bool GetRandomMovePath(const EBFSType BFSType, const int32 MaxDepth, TArray<ATile*>& OutRandomMovePath);
	
	UFUNCTION(BlueprintCallable)
	void ActivateMoveAbility(const TArray<ATile*>& PathTiles);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "TargetTile로 이동하기 위한 최단 경로를 계산한 뒤, 그 모든 타일을 우선순위대로 정렬해 반환합니다."))
	void GetPrioritizedMoveTiles(const ATile* TargetTile, const int32 MoveRange, TArray<ATile*>& OutPathTiles) const;

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "선택한 Ability 사용을 예고합니다."))
	void CommitPlan();

	UFUNCTION(BlueprintCallable)
	void StartCombat();

	bool IsInCombat() const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface
	
	//~ Begin AAIController Interface
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	//~ End of AAIController Interface

private:
	void OnEnemyAbilityTriedActivate(AActor* AbilityInstigator) const;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;
	
	UPROPERTY()
	TObjectPtr<UActorSelectorComponent> ActorSelector;

	UPROPERTY(EditDefaultsOnly, Category = "ArrowRenderer")
	TSubclassOf<AArrowRenderer> ArrowRendererClass;

	UPROPERTY()
	TObjectPtr<AArrowRenderer> ArrowRenderer;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	bool bIsInCombat = false;

private:
	FDelegateHandle OnEnemyAbilityTriedActivateDelegateHandle;

	FGameplayAbilitySpecHandle PlannedAbilitySpecHandle;
	TWeakObjectPtr<ATile> PlannedTargetTile;
};
