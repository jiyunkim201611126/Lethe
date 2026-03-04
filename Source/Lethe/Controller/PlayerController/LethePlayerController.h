// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "LethePlayerController.generated.h"

class UPreviewCoordinatorComponent;
class AArrowRenderer;
class UAbilitySystemComponent;
class ULetheAbilitySystemComponent;
class ULetheCardAbility;
class ULetheHUD;
class UTileSelectorComponent;
struct FGameplayAbilityActorInfo;

DECLARE_DELEGATE_OneParam(FOnNumberKeyPressedSignature, const int32 /* InNumber */);
DECLARE_MULTICAST_DELEGATE(FOnCardSelectCanceledSignature);
DECLARE_DELEGATE_TwoParams(FOnResolveUseCardSignature, const int32 /* HandIndex */, const bool /* bSuccess */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCameraHeightChangedSignature, const float /* AttributeWidgetSize */);

UCLASS()
class LETHE_API ALethePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALethePlayerController();

	void OnNumberPressed(const int32 InNumber) const;
	void OnWheeled(const float AttributeWidgetSize) const;
	void OnLeftMouseButtonClickedOnWorld();
	void ResetSelectedCharacter();
	
	bool SetCardSelected(const bool bInCardSelected, ULetheAbilitySystemComponent* OwnerASC = nullptr, const FGameplayTag& CardTag = FGameplayTag());
	void SetMouseOnCardUseSection(const bool bInMouseOnCardUseSection);
	void RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, const int32 InHandIndex);
	void OnAbilityEnded(const bool bUseSuccess);

	void GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, FText& OutText) const;

	ULetheHUD* GetLetheHUD() const;
	bool IsProgressingCardAbility() const;
	UPreviewCoordinatorComponent* GetPreviewCoordinatorComponent() const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	//~ End of AActor Interface

private:
	// 카드 선택 상태에서 마우스를 움직여서 다른 Tile이 검출되면 호출되는 콜백 함수입니다.
	void OnOtherTileDetected(const AActor* LastActor, const AActor* CurrentActor) const;

	// WaitingForUseCardsQueue에서 하나씩 Ability를 꺼내 사용하는 함수입니다.
	void TryUseNextCardAbility();

public:
	FOnNumberKeyPressedSignature OnNumberKeyPressedDelegate;
	FOnCardSelectCanceledSignature OnCancelCardSelectDelegate;
	FOnResolveUseCardSignature OnResolveUseCardDelegate;
	FOnCameraHeightChangedSignature OnCameraHeightChangedDelegate;
	
protected:
	UPROPERTY(EditDefaultsOnly, Instanced, Category = LetheHUD)
	TObjectPtr<ULetheHUD> LetheHUD;
	
private:
	UPROPERTY()
	TObjectPtr<UTileSelectorComponent> TileSelector;

	UPROPERTY()
	TObjectPtr<UPreviewCoordinatorComponent> PreviewCoordinatorComponent;
	
	uint8 bMouseOnCardUseSection : 1 = false;
	
	// CDO를 캐싱할 멤버변수기 때문에 템플릿에도 const를 붙여줍니다.
	TWeakObjectPtr<const ULetheCardAbility> SelectedCardAbility;
	TWeakObjectPtr<UAbilitySystemComponent> SelectedCardOwnerASC;

	// 사실상 Queue로 사용하며, Key용 값인 HandIndex도 있어 Map으로도 구현 가능합니다.
	// 하지만 내부에 할당되는 개수가 어차피 최대 7개라 그냥 순회하면서 찾는 게 더 빠르니까 Array로 구현했습니다.
	UPROPERTY()
	TArray<FAbilityActivationData> WaitingForUseCardsQueue;
	uint8 bIsProgressingCardAbility : 1 = false;

	UPROPERTY(EditDefaultsOnly, Category = ArrowRenderer)
	TSubclassOf<AArrowRenderer> ArrowRendererClass;

	UPROPERTY()
	TObjectPtr<AArrowRenderer> ArrowRenderer;

	TWeakObjectPtr<AActor> SelectedCharacter;
};
