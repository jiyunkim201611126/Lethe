// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LetheAIController.generated.h"

class UStateTreeAIComponent;

UCLASS()
class LETHE_API ALetheAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALetheAIController();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;
};
