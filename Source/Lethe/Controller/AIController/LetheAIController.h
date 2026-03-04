// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "LetheAIController.generated.h"

enum class EPhaseState : uint8;
class ATile;
class UStateTreeAIComponent;

UCLASS()
class LETHE_API ALetheAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALetheAIController();

	UFUNCTION(BlueprintCallable)
	void SelectMoveAbility();

	UFUNCTION(BlueprintCallable)
	void SelectRandomAbility();

	UFUNCTION(BlueprintCallable)
	void FindNearestPlayerCharacterTileCoord();

	UFUNCTION(BlueprintCallable)
	bool UseAbilityToTargetTile();

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
	FAbilityActivationData SelectedAbilityData;

	TWeakObjectPtr<ATile> TargetTile;
};
