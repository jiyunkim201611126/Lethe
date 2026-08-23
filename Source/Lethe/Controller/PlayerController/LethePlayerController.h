// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LethePlayerControllerBase.h"
#include "GameFramework/PlayerController.h"
#include "Lethe/Data/TurnPhaseState.h"
#include "LethePlayerController.generated.h"

class ATile;
class AArrowRenderer;
class UAbilitySystemComponent;
class UActorSelectorComponent;
class ULetheAbilitySystemComponent;
class ULetheCardAbility;
class UPlayerAbilityRequestComponent;
class UPreviewCoordinatorComponent;
struct FGameplayAbilitySpecHandle;
struct FGameplayTag;
struct FPreviewData;
struct FSavedCard;

DECLARE_DELEGATE_OneParam(FOnSelectCardSignature, const int32 /* HandSlotIndex */);
DECLARE_MULTICAST_DELEGATE(FOnCancelCardSelectSignature);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPreviewDataUpdatedSignature, const FPreviewData&);
DECLARE_DELEGATE_TwoParams(FOnResolveUseCardSignature, const int32 /* HandSlotIndex */, const bool /* bSuccess */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCameraHeightChangedSignature, const float /* AttributeWidgetSize */);

/**
 * LethePawn을 통해 들어오는 WASD, 마우스 휠 조작 처리 및 게임 플레이 규칙(캐릭터 선택, 카드 사용 등)의 로직을 담당합니다.
 */
UCLASS()
class LETHE_API ALethePlayerController : public ALethePlayerControllerBase
{
	GENERATED_BODY()

public:
	ALethePlayerController();
	
	//~ Begin AActor Interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	//~ End of AActor Interface

	void OnWheeled(const float AttributeWidgetSize) const;
	void HandleLeftMouseButtonClickedInWorldSection();
	void ResetSelectedCharacter();
	void ToggleMovePreview();
	void RefreshMovePreview() const;
	void StartResolvePlayerMoves() const;
	void OnPlayerMovedResolved(AActor* MovedCharacter) const;
	
	void OnSelectCardRequested(const int32 HandSlotIndex, ULetheAbilitySystemComponent* CardOwnerASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle);
	void ResetSelectedCard();
	void SetMouseOnWorldSection(const bool bInMouseOnWorldSection);
	void RequestUseCard(ULetheAbilitySystemComponent* CardOwnerASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle, const FGameplayTag& CardTag, const int32 InHandSlotIndex);

	bool IsCardSelected() const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End of AActor Interface

private:
	void OnTurnPhaseStateChanged(const ETurnPhaseState OldTurnPhaseState, const ETurnPhaseState NewTurnPhaseState);
	
	/** 카드 선택 상태에서 마우스를 움직여서 다른 Tile이 검출되면 호출되는 콜백 함수입니다. */
	void OnOtherTileDetected() const;

	void OnUpdatePreviewData(const FPreviewData& PreviewData) const;
	
	void OnCardUseResolved(const int32 HandSlotIndex, const bool bSuccess) const;

	/** 액터 아래에 있는 타일을 하이라이팅 하고자 할 때 사용하는 헬퍼용 함수입니다. */
	void GetTileUnderActorAsArray(const UWorld* World, const AActor* Actor, TArray<ATile*>& OutTiles) const;

public:
	FOnSelectCardSignature OnSelectCardDelegate;
	FOnCancelCardSelectSignature OnCancelCardSelectCancelDelegate;
	FOnPreviewDataUpdatedSignature OnPreviewDataUpdatedDelegate;
	FOnResolveUseCardSignature OnResolveUseCardDelegate;
	FOnCameraHeightChangedSignature OnCameraHeightChangedDelegate;
	
private:
	UPROPERTY()
	TObjectPtr<UActorSelectorComponent> ActorSelector;

	UPROPERTY()
	TObjectPtr<UPreviewCoordinatorComponent> PreviewCoordinatorComponent;

	UPROPERTY()
	TObjectPtr<UPlayerAbilityRequestComponent> PlayerAbilityRequestComponent;

	ETurnPhaseState CurrentTurnPhaseState = ETurnPhaseState::None;
	FDelegateHandle OnTurnPhaseStateChangedHandle;
	
	uint8 bMouseOnWorldSection : 1 = false;
	
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
