// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LethePlayerController.generated.h"

struct FGameplayTag;

UCLASS()
class LETHE_API ALethePlayerController : public APlayerController
{
	GENERATED_BODY()

	ALethePlayerController();

public:
	bool RequestUseCard(const FGameplayTag& InCardTag) const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End of AActor Interface
};
