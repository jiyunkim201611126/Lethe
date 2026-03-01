// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LetheAIController.generated.h"

enum class EPhaseState : uint8;
class UStateTreeAIComponent;

UCLASS()
class LETHE_API ALetheAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALetheAIController();

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface
	
	//~ Begin AAIController Interface
	virtual void OnPossess(APawn* InPawn) override;
	//~ End of AAIController Interface

private:
	void OnPhaseStateChanged(const EPhaseState OldPhase, const EPhaseState NewPhase);
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;
};
