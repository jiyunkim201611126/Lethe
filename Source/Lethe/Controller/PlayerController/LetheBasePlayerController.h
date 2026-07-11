// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LetheBasePlayerController.generated.h"

UCLASS()
class LETHE_API ALetheBasePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALetheBasePlayerController();
	
protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End of AActor Interface
};
