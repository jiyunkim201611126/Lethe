// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Lethe/Data/TurnPhaseState.h"
#include "CardStage.generated.h"

enum class ECardAction : uint8;
class ACardActor;
class ADeckBoxes;
class UBoxComponent;
class UCardContainerComponent;
class ULetheAbilitySystemComponent;
class USceneCaptureComponent2D;
struct FCardInitParams;
struct FGameplayAbilitySpecHandle;
struct FKey;

DECLARE_DELEGATE_OneParam(FOnViewCardDetailRequested, const ACardActor*);
DECLARE_DELEGATE_ThreeParams(FOnSelectCardRequested, const int32 /* HandSlotIndex */, ULetheAbilitySystemComponent*, const FGameplayAbilitySpecHandle&);
DECLARE_DELEGATE(FOnStartResolvePlayerMovesRequested);
DECLARE_DELEGATE_FourParams(FOnUseCardRequested, ULetheAbilitySystemComponent*, const FGameplayAbilitySpecHandle&, const FGameplayTag&, int32 /* HandSlotIndex */);
DECLARE_DELEGATE(FOnTurnEndRequested);

/**
 * 카드의 뷰를 담당하는 액터입니다. SceneCapture로 렌더링되는 3D 카드 액터의 배치, 선택, 덱 미리보기, 입력 라우팅을 담당합니다.
 *
 * Deck/Hand/Graveyard 상태의 진실의 원천은 GameState의 UCardContainerComponent(모델)이며,
 * 이 클래스는 모델의 OnContainerChanged를 구독해 카드 액터의 위치와 표시 상태를 모델로부터 파생시킵니다.
 * 덱 미리보기와 선택 상태는 게임 규칙과 무관한 뷰 상태이므로 여기서만 관리합니다.
 */
UCLASS(Abstract)
class LETHE_API ACardStage : public AActor
{
	GENERATED_BODY()

public:
	ACardStage();

	void Initialize(const TArray<TObjectPtr<ULetheAbilitySystemComponent>>& InAbilitySystemComponents);
	void CreateCard(const FCardInitParams& CardInitParams);

	bool HandleLeftMouseButtonClickedInCardStageSection(const FVector2D& TargetUV);
	void HandleCapturedMouseMove(const FVector2D& TargetUV) const;
	void HandleCapturedMouseLeave() const;
	bool HandleLeftMouseButtonClickedInWorldSection();
	void HandleKeyboardEvent(int32 Number) const;

	void OnCardSelected(const int32 HandSlotIndex);
	void OnCancelSelectedCard();
	void OnResolveUseCard(const int32 HandSlotIndex, const bool bSuccess);

	bool TryViewDetail(const FVector2D& TargetUV) const;
	void OnTurnEndButtonClicked() const;
	void ResetSelectedDeckBox();

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface

private:
	/** 캡쳐된 이미지와 마우스를 기준으로 라인트레이스를 수행, CardActor를 검출합니다. */
	ACardActor* GetCardActorAtUV(const FVector2D& TargetUV) const;
	UBoxComponent* GetDeckBoxCollisionAtUV(const FVector2D& TargetUV) const;
	bool TryGetHitResultByCardChannel(const FVector2D& TargetUV, FHitResult& OutHitResult) const;

	void OnCardMouseEvent(const ACardActor* CardActor, ECardAction CardAction) const;
	void OnMouseEventWhenPlayerTurnPhase(const ACardActor* CardActor, const ECardAction CardAction) const;
	void OnKeyboardEventWhenDrawPhase(const int32 Number) const;
	void OnKeyboardEventWhenPlayerPhase(const int32 Number) const;

	void TryDraw(ULetheAbilitySystemComponent* DeckOwnerASC) const;
	void RequestSelectCard(const ACardActor* CardActor) const;

	void OnTurnPhaseStateChanged(const ETurnPhaseState OldTurnPhaseState, const ETurnPhaseState NewTurnPhaseState);

	//~ Begin View Sync
	/** 모델이 변경되면 호출되며, 모든 카드 액터의 표시 상태와 위치를 모델로부터 다시 파생시킵니다. */
	void OnContainerChanged();
	void RefreshCardContainerStates();
	/** 모델의 Zone과 미리보기 상태를 기준으로 모든 카드 액터를 배치하고 CurrentSlots를 갱신합니다. */
	void UpdateAllCardLocations();
	//~ End of View Sync

	//~ Begin Deck Preview
	void PreviewDeck(const FGameplayTag& CharacterTag);
	void StopPreviewDeck(const bool bShouldMoveCards = true);
	//~ End of Deck Preview

	ACardActor* FindCardActor(const int32 InstanceId) const;
	int32 FindCurrentSlotIndex(const ACardActor* CardActor) const;
	FGameplayTag GetCharacterTag(const ULetheAbilitySystemComponent* AbilitySystemComponent) const;
	bool AreAllCardActorsBound() const;

public:
	FOnViewCardDetailRequested OnViewCardDetailRequested;
	FOnSelectCardRequested OnSelectCardRequested;
	FOnStartResolvePlayerMovesRequested OnStartResolvePlayerMovesRequested;
	FOnUseCardRequested OnUseCardRequested;
	FOnTurnEndRequested OnTurnEndRequested;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Card")
	TSubclassOf<ADeckBoxes> DeckBoxesClass;

	UPROPERTY(EditDefaultsOnly, Category = "Card")
	TSubclassOf<ACardActor> CardActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Card")
	FGameplayTag DrawSoundTag;

	UPROPERTY(EditDefaultsOnly, Category = "Card")
	FGameplayTag TurnEndSoundTag;

	UPROPERTY(EditDefaultsOnly, Category = "Card | Input")
	float CardTraceDistance = 1000.f;

	//~ Begin Layout
	UPROPERTY(EditDefaultsOnly, Category = "Card | Layout")
	FVector DeckFirstCardLocation = FVector(-4.9f, 0.3f, 3.82f);

	UPROPERTY(EditDefaultsOnly, Category = "Card | Layout")
	float DeckCardXOffset = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category = "Card | Layout")
	FVector HandSlotFirstCardLocation = FVector(1.f, -2.80185f, 3.8543f);

	UPROPERTY(EditDefaultsOnly, Category = "Card | Layout")
	float HandSlotCardXOffset = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Card | Layout")
	FVector GraveyardFirstCardLocation = FVector(0.f, -2.8f, 5.58038f);

	UPROPERTY(EditDefaultsOnly, Category = "Card | Layout")
	FVector GraveyardCardOffset = FVector(0.f, -0.3f, 0.51962f);
	//~ End of Layout

private:
	TWeakObjectPtr<UCardContainerComponent> CardContainer;

	uint8 bInitialized : 1 = false;
	uint8 bContainerDelegateBound : 1 = false;

	ETurnPhaseState CurrentTurnPhaseState = ETurnPhaseState::None;

	UPROPERTY()
	TObjectPtr<ADeckBoxes> DeckBoxes;

	/** 캐릭터 순서대로 접근, 혹은 덱 박스 인덱스 기반으로 접근하기 위해 AbilitySystemComponent 배열을 캐싱해둡니다. */
	TArray<TWeakObjectPtr<ULetheAbilitySystemComponent>> AbilitySystemComponents;

	UPROPERTY()
	TArray<TObjectPtr<ACardActor>> SpawnedCards;

	/** 모델의 InstanceId와 카드 액터의 매핑입니다. */
	UPROPERTY()
	TMap<int32, TObjectPtr<ACardActor>> CardActorsByInstance;

	/**
	 * 현재 핸드 줄에 배치된 슬롯의 InstanceId 목록으로, 빈 슬롯은 INDEX_NONE입니다.
	 * 카드 선택/사용 요청과 그 결과 콜백은 이 배열의 인덱스(HandSlotIndex)를 키로 주고받습니다.
	 * 덱 미리보기 중에는 미리보기 카드가 이 배열을 채우므로, 비전투 중에도 덱의 카드를 바로 사용할 수 있습니다.
	 */
	TArray<int32> CurrentSlots;

	/** 덱 미리보기 상태입니다. 미리보기 중이 아니면 CharacterTag가 비어 있습니다. */
	FGameplayTag PreviewCharacterTag;
	TArray<int32> DeckPreviewInstanceIds;

	UPROPERTY()
	TObjectPtr<ACardActor> CurrentSelectedCard;

	TWeakObjectPtr<UBoxComponent> CurrentSelectedDeckBox;

	UPROPERTY()
	TMap<int32, TObjectPtr<ACardActor>> UseRequestedCards;
};
