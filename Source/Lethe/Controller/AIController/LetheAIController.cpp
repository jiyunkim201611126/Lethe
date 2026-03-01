// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAIController.h"

#include "Components/StateTreeAIComponent.h"

ALetheAIController::ALetheAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
}
