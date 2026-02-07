// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "LethePlayerController.generated.h"

class UAbilitySystemComponent;
class ULetheAbilitySystemComponent;
class ULetheGameplayAbility;
class ULetheHUD;
class UTileSelectorComponent;

DECLARE_DELEGATE_OneParam(FOnNumberKeyPressedSignature, const int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCardSelectedSignature, const ULetheAbilitySystemComponent*, const ULetheGameplayAbility*)
DECLARE_MULTICAST_DELEGATE(FOnCardSelectCanceledSignature);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnOtherTileDetected, const AActor*, const AActor*, const ULetheGameplayAbility*)

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

private:
	// 카드 선택 상태에서 마우스를 움직여서 다른 Tile이 검출되면 호출되는 콜백 함수입니다.
	void OnOtherTileDetected(const AActor* LastActor, const AActor* CurrentActor) const;

public:
	FOnNumberKeyPressedSignature OnNumberKeyPressedDelegate;
	FOnCardSelectedSignature OnCardSelectedDelegate;
	FOnCardSelectCanceledSignature OnCancelCardSelectDelegate;
	FOnOtherTileDetected OnOtherTileDetectedDelegate;
	
protected:
	UPROPERTY(EditDefaultsOnly, Instanced)
	TObjectPtr<ULetheHUD> LetheHUD;
	
private:
	UPROPERTY()
	TObjectPtr<UTileSelectorComponent> TileSelector;
	
	uint8 bCardSelected : 1 = false;
	uint8 bMouseOnCardUseSection : 1 = false;

	// CDO를 캐싱할 멤버변수기 때문에 템플릿에도 const를 붙여줍니다.
	TWeakObjectPtr<const ULetheGameplayAbility> SelectedCardAbility;
};
