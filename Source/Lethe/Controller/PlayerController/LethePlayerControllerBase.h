// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LethePlayerControllerBase.generated.h"

UCLASS()
class LETHE_API ALethePlayerControllerBase : public APlayerController
{
	GENERATED_BODY()

public:
	ALethePlayerControllerBase();
	
protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End of AActor Interface
};
