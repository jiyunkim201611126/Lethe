// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "Lethe/Data/PhaseData.h"
#include "LethePlayerController.generated.h"

class AArrowRenderer;
class APlayerCharacterBase;
class ATile;
class UAbilitySystemComponent;
class UActorSelectorComponent;
class UAttributeSet;
class ULetheAbilitySystemComponent;
class ULetheCardAbility;
class ULetheHUD;
class ULetheWidgetController;
class UPlayerAbilityRequestComponent;
class UPreviewCoordinatorComponent;
struct FGameplayAbilityActorInfo;
struct FPreviewData;
struct FSavedCard;

DECLARE_MULTICAST_DELEGATE(FOnCancelCardSelectSignature);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPreviewDataUpdatedSignature, const FPreviewData&);
DECLARE_DELEGATE_TwoParams(FOnResolveUseCardSignature, const int32 /* HandIndex */, const bool /* bSuccess */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCameraHeightChangedSignature, const float /* AttributeWidgetSize */);

UCLASS()
class LETHE_API ALethePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALethePlayerController();

	ULetheWidgetController* InitPlayerUI(UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS);
	ULetheWidgetController* InitEnemyUI(UAbilitySystemComponent* ASC, UAttributeSet* AS);

	void OnWheeled(const float AttributeWidgetSize) const;
	void OnLeftMouseButtonClickedOnWorld();
	void ResetSelectedCharacter();
	void ToggleMovePreview();
	void RefreshMovePreview() const;
	void StartResolvePlayerMoves() const;
	void OnPlayerMovedResolved(AActor* MovedCharacter) const;
	
	bool SetCardSelected(const bool bInCardSelected, ULetheAbilitySystemComponent* OwnerASC = nullptr, const FGameplayTag& CardTag = FGameplayTag());
	void SetMouseOnCardUseSection(const bool bInMouseOnCardUseSection);
	void RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, const int32 InHandIndex) const;

	void GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, FText& OutText) const;

	ULetheHUD* GetLetheHUD() const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	//~ End of AActor Interface

private:
	void OnPhaseStateChanged(const EPhaseState OldState, const EPhaseState NewState);
	
	/** 카드 선택 상태에서 마우스를 움직여서 다른 Tile이 검출되면 호출되는 콜백 함수입니다. */
	void OnOtherTileDetected(const TArray<AActor*>& CurrentActors) const;

	void OnUpdatePreviewData(const FPreviewData& PreviewData) const;
	
	void OnCardUseResolved(const int32 HandIndex, const bool bSuccess) const;

public:
	FOnCancelCardSelectSignature OnCancelCardSelectCancelDelegate;
	FOnPreviewDataUpdatedSignature OnPreviewDataUpdatedDelegate;
	FOnResolveUseCardSignature OnResolveUseCardDelegate;
	FOnCameraHeightChangedSignature OnCameraHeightChangedDelegate;
	
protected:
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "LetheHUD")
	TObjectPtr<ULetheHUD> LetheHUD;
	
private:
	UPROPERTY()
	TObjectPtr<UActorSelectorComponent> ActorSelector;

	UPROPERTY()
	TObjectPtr<UPreviewCoordinatorComponent> PreviewCoordinatorComponent;

	UPROPERTY()
	TObjectPtr<UPlayerAbilityRequestComponent> PlayerAbilityRequestComponent;

	EPhaseState CurrentPhaseState = EPhaseState::None;
	FDelegateHandle OnPhaseStateChangedHandle;
	
	uint8 bMouseOnCardUseSection : 1 = false;
	
	/** CDO를 캐싱할 멤버변수기 때문에 템플릿에도 const를 붙여줍니다. */
	TWeakObjectPtr<const ULetheCardAbility> SelectedCardAbility;
	TWeakObjectPtr<UAbilitySystemComponent> SelectedCardOwnerASC;
	TWeakObjectPtr<AActor> SelectedCharacter;

	UPROPERTY(EditDefaultsOnly, Category = "ArrowRenderer")
	TSubclassOf<AArrowRenderer> ArrowRendererClass;

	UPROPERTY()
	TObjectPtr<AArrowRenderer> ArrowRenderer;

	uint8 bIsReservedMovePreviewingMode : 1 = false;
};
