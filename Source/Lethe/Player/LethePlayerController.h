// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LethePlayerController.generated.h"

class UCardWidget;
struct FGameplayTag;

UCLASS()
class LETHE_API ALethePlayerController : public APlayerController
{
	GENERATED_BODY()

	ALethePlayerController();

public:
	bool RequestUseCard(const UCardWidget* InCardWidget) const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End of AActor Interface
};
