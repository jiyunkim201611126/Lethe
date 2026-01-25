// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LethePlayerController.generated.h"

class IHighlightInterface;
class UCardWidget;
struct FGameplayTag;

UCLASS()
class LETHE_API ALethePlayerController : public APlayerController
{
	GENERATED_BODY()

	ALethePlayerController();

public:
	void SetReadyToUseCard(const bool bReady);
	bool RequestUseCard(const UCardWidget* InCardWidget);

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	//~ End of AActor Interface

private:
	uint8 bReadyToUseCard : 1 = false;
	
	TScriptInterface<IHighlightInterface> LastActor;
	TScriptInterface<IHighlightInterface> ThisActor;
};
