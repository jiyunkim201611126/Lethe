// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LethePlayerController.generated.h"

struct FGameplayTag;
class ULetheHUD;
class IHighlightInterface;
class UCardWidget;

DECLARE_DELEGATE_OneParam(FOnNumberKeyPressedSignature, const int32);

UCLASS()
class LETHE_API ALethePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALethePlayerController();

	void OnNumberPressed(const int32 InNumber) const;
	
	void SetReadyToUseCard(const bool bReady);
	bool RequestUseCard(const UCardWidget* InCardWidget);

	ULetheHUD* GetLetheHUD() const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	//~ End of AActor Interface

public:
	FOnNumberKeyPressedSignature OnNumberKeyPressedDelegate;
	
protected:
	UPROPERTY(EditDefaultsOnly, Instanced)
	TObjectPtr<ULetheHUD> LetheHUD;
	
private:
	uint8 bReadyToUseCard : 1 = false;
	
	TScriptInterface<IHighlightInterface> LastActor;
	TScriptInterface<IHighlightInterface> ThisActor;
};
