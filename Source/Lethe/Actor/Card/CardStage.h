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
class UCardContainerManager;
class ULetheAbilitySystemComponent;
class USceneCaptureComponent2D;
struct FCardInitParams;
struct FGameplayAbilitySpecHandle;
struct FKey;

DECLARE_DELEGATE_OneParam(FOnViewCardDetailRequested, const ACardActor*);
DECLARE_DELEGATE_ThreeParams(FOnSelectCardRequested, const int32 /* HandSlotIndex */, ULetheAbilitySystemComponent*, const FGameplayAbilitySpecHandle&);
DECLARE_DELEGATE(FOnDrawPhaseCompleted);
DECLARE_DELEGATE(FOnStartResolvePlayerMovesRequested);
DECLARE_DELEGATE_FourParams(FOnUseCardRequested, ULetheAbilitySystemComponent*, const FGameplayAbilitySpecHandle&, const FGameplayTag&, int32 /* HandSlotIndex */);
DECLARE_DELEGATE(FOnTurnEndRequested);

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

	void UpdateAllCardLocations() const;

	void TryDraw(ULetheAbilitySystemComponent* DeckOwnerASC) const;
	void RequestSelectCard(const ACardActor* CardActor) const;
	void OnTurnPhaseStateChanged(const ETurnPhaseState OldTurnPhaseState, const ETurnPhaseState NewTurnPhaseState);
	void OnDrawPhaseStarted() const;

public:
	FOnViewCardDetailRequested OnViewCardDetailRequested;
	FOnSelectCardRequested OnSelectCardRequested;
	FOnDrawPhaseCompleted OnDrawPhaseCompleted;
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

private:
	UPROPERTY()
	TObjectPtr<UCardContainerManager> CardContainerManager;
	
	uint8 bInitialized : 1 = false;

	ETurnPhaseState CurrentTurnPhaseState = ETurnPhaseState::None;

	UPROPERTY()
	TObjectPtr<ADeckBoxes> DeckBoxes;

	/** 캐릭터 순서대로 접근, 혹은 순서에 맞춰 인덱스 기반으로 접근하기 위해 AbilitySystemComponent 배열을 캐싱해둡니다. */
	TArray<TWeakObjectPtr<ULetheAbilitySystemComponent>> AbilitySystemComponents;

	UPROPERTY()
	TArray<TObjectPtr<ACardActor>> SpawnedCards;

	UPROPERTY()
	TObjectPtr<ACardActor> CurrentSelectedCard;
	
	TWeakObjectPtr<UBoxComponent> CurrentSelectedDeckBox;

	UPROPERTY()
	TMap<int32, TObjectPtr<ACardActor>> UseRequestedCards;
};
