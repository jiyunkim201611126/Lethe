// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "LethePlayerController.generated.h"

class ULetheAbilitySystemComponent;
class ULetheGameplayAbility;
class ULetheHUD;
class UTileSelectorComponent;

DECLARE_DELEGATE_OneParam(FOnNumberKeyPressedSignature, const int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCardSelectedSignature, const ULetheAbilitySystemComponent*, const ULetheGameplayAbility*)
DECLARE_MULTICAST_DELEGATE(FOnCancelCardSelectSignature);

UCLASS()
class LETHE_API ALethePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALethePlayerController();

	void OnNumberPressed(const int32 InNumber) const;
	
	void SetCardSelected(const bool bInCardSelected, const ULetheAbilitySystemComponent* OwnerASC = nullptr, const FGameplayTag& CardTag = FGameplayTag());
	void SetMouseOnCardUseSection(const bool bInMouseOnCardUseSection);
	bool RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag);

	ULetheHUD* GetLetheHUD() const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	//~ End of AActor Interface

public:
	FOnNumberKeyPressedSignature OnNumberKeyPressedDelegate;
	FOnCardSelectedSignature OnCardSelectedDelegate;
	FOnCancelCardSelectSignature OnCancelCardSelectDelegate;
	
protected:
	UPROPERTY(EditDefaultsOnly, Instanced)
	TObjectPtr<ULetheHUD> LetheHUD;
	
private:
	UPROPERTY()
	TObjectPtr<UTileSelectorComponent> TileSelector;
	
	uint8 bCardSelected : 1 = false;
	uint8 bMouseOnCardUseSection : 1 = false;
};
