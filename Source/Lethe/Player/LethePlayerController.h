// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LethePlayerController.generated.h"

UCLASS()
class LETHE_API ALethePlayerController : public APlayerController
{
	GENERATED_BODY()

	ALethePlayerController();

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End of AActor Interface
};
