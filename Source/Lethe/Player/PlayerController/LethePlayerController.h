// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/PlayerController.h"
#include "LethePlayerController.generated.h"

class AArrowRenderer;
class UAbilitySystemComponent;
class ULetheAbilitySystemComponent;
class ULetheGameplayAbility;
class ULetheHUD;
class UTileSelectorComponent;
struct FGameplayAbilityActorInfo;

USTRUCT()
struct FUseCardData
{
	GENERATED_BODY()

	int32 HandIndex = INDEX_NONE;
	
	FGameplayAbilitySpecHandle AbilitySpecHandle;
	
	FGameplayTag CardTag;

	UPROPERTY()
	FGameplayEventData Payload;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilityOwnerASC;
};

DECLARE_DELEGATE_OneParam(FOnNumberKeyPressedSignature, const int32 /* InNumber */);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCardSelectedSignature, const ULetheAbilitySystemComponent* /* CardOwnerASC */, const ULetheGameplayAbility* /* CardAbility */)
DECLARE_MULTICAST_DELEGATE(FOnCardSelectCanceledSignature);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnOtherTileDetected, const AActor* /* LastActor */, const AActor* /* CurrentActor */, const UAbilitySystemComponent* /* CardOwnerASC */, const ULetheGameplayAbility* /* CardAbility */)
DECLARE_DELEGATE_TwoParams(FOnResolveUseCardSignature, const int32 /* HandIndex */, const bool /* bSuccess */)

UCLASS()
class LETHE_API ALethePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALethePlayerController();

	void OnNumberPressed(const int32 InNumber) const;
	
	void SetCardSelected(const bool bInCardSelected, ULetheAbilitySystemComponent* OwnerASC = nullptr, const FGameplayTag& CardTag = FGameplayTag());
	void SetMouseOnCardUseSection(const bool bInMouseOnCardUseSection);
	void RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, const int32 InHandIndex);
	void OnAbilityEnded();

	ULetheHUD* GetLetheHUD() const;
	bool IsProgressingCardAbility() const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	//~ End of AActor Interface

private:
	// 카드 선택 상태에서 마우스를 움직여서 다른 Tile이 검출되면 호출되는 콜백 함수입니다.
	void OnOtherTileDetected(const AActor* LastActor, const AActor* CurrentActor) const;

	void TryUseNextCardAbility();

public:
	FOnNumberKeyPressedSignature OnNumberKeyPressedDelegate;
	FOnCardSelectedSignature OnCardSelectedDelegate;
	FOnCardSelectCanceledSignature OnCancelCardSelectDelegate;
	FOnOtherTileDetected OnOtherTileDetectedDelegate;
	FOnResolveUseCardSignature OnResolveUseCardDelegate;
	
protected:
	UPROPERTY(EditDefaultsOnly, Instanced, Category = LetheHUD)
	TObjectPtr<ULetheHUD> LetheHUD;
	
private:
	UPROPERTY()
	TObjectPtr<UTileSelectorComponent> TileSelector;
	
	uint8 bCardSelected : 1 = false;
	uint8 bMouseOnCardUseSection : 1 = false;
	
	// CDO를 캐싱할 멤버변수기 때문에 템플릿에도 const를 붙여줍니다.
	TWeakObjectPtr<const ULetheGameplayAbility> SelectedCardAbility;
	TWeakObjectPtr<UAbilitySystemComponent> SelectedCardOwnerASC;

	// 사실상 Queue로 사용하며, Key용 값인 HandIndex도 있어 Map으로도 구현 가능합니다.
	// 하지만 내부에 할당되는 개수가 어차피 최대 7개라 그냥 순회하면서 찾는 게 더 빠르니까 Array로 구현했습니다.
	UPROPERTY()
	TArray<FUseCardData> WaitingForUseCardsQueue;
	uint8 bIsProgressingCardAbility : 1 = false;

	UPROPERTY(EditDefaultsOnly, Category = ArrowRenderer)
	TSubclassOf<AArrowRenderer> ArrowRendererClass;

	UPROPERTY()
	TObjectPtr<AArrowRenderer> ArrowRenderer;
};
