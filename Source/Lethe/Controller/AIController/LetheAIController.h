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
	void SelectMoveAbility();

	UFUNCTION(BlueprintCallable)
	void SelectRandomAbility() const;

	UFUNCTION(BlueprintCallable)
	FFoundTileData FindNearestPlayerCharacterTile() const;

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "선택된 Ability의 ActivationData에 TargetTile을 할당하는 함수로, 반드시 Ability를 선택한 후 호출해야 합니다."))
	void SetTargetTile(ATile* TargetTile) const;

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

private:
	// 이 값이 낮은 캐릭터가 먼저 Ability를 사용합니다.
	int32 AbilityPriority = 0;
};
