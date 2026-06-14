// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Lethe/Data/PhaseData.h"
#include "Lethe/SaveGame/SavedCardTypes.h"
#include "CardStage.generated.h"

class UBoxComponent;
enum class ECardAction : uint8;
class ACardActor;
class UCardContainerManager;
class ADeckBoxes;
class ULetheAbilitySystemComponent;
class USceneCaptureComponent2D;
struct FCardInitParams;
struct FKey;

DECLARE_DELEGATE_OneParam(FOnViewCardDetailRequested, const ACardActor*);
DECLARE_DELEGATE_RetVal_ThreeParams(bool, FOnSelectCardRequested, bool, ULetheAbilitySystemComponent*, const FGameplayTag&);
DECLARE_DELEGATE(FOnGoPlayerTurnPhaseRequested);
DECLARE_DELEGATE(FOnStartResolvePlayerMovesRequested);
DECLARE_DELEGATE_ThreeParams(FOnUseCardRequested, ULetheAbilitySystemComponent*, const FSavedCard&, int32);
DECLARE_DELEGATE_RetVal(bool, FOnTurnEndRequested);

UCLASS(Abstract)
class LETHE_API ACardStage : public AActor
{
	GENERATED_BODY()

public:
	ACardStage();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface

	void Initialize(const TArray<TObjectPtr<ULetheAbilitySystemComponent>>& InAbilitySystemComponents);

	void CreateCard(const FCardInitParams& CardInitParams);

	bool HandleCapturedMouseButtonDown(const FVector2D& TargetUV, const FKey& MouseButton);
	bool HandleCapturedMouseButtonUp(const FVector2D& TargetUV, const FKey& MouseButton);
	void HandleCapturedMouseMove(const FVector2D& TargetUV) const;
	void HandleCapturedMouseLeave() const;
	void HandleCapturedMouseCaptureLost();
	bool HandleMouseButtonDownInCardUseSection() const;
	bool HandleMouseButtonUpInCardUseSection();
	void HandleKeyboardEvent(int32 Number);

	void HandlePhaseStateChanged(EPhaseState OldState, EPhaseState NewState);
	void HandleCancelSelectedCard();
	void HandleResolveUseCard(int32 HandIndex, bool bSuccess);
	void HandleTurnEndButtonClicked() const;
	void HandleRightMouseButtonDown() const;

private:
	/** 캡쳐된 이미지와 마우스를 기준으로 라인트레이스를 수행, CardActor를 검출합니다. */
	ACardActor* GetCardActorAtUV(const FVector2D& TargetUV) const;
	UBoxComponent* GetDeckBoxCollisionAtUV(const FVector2D& TargetUV) const;
	bool TryGetHitResultByCardChannel(const FVector2D& TargetUV, FHitResult& OutHitResult) const;

	void OnCardMouseEvent(ACardActor* CardActor, ECardAction CardAction);
	void OnMouseEventWhenDrawPhase(const ACardActor* CardActor, ECardAction CardAction) const;
	void OnMouseEventWhenPlayerTurnPhase(ACardActor* CardActor, ECardAction CardAction);
	void OnKeyboardEventWhenDrawPhase(int32 Number);
	void OnKeyboardEventWhenPlayerTurnPhase(int32 Number);

	void UpdateAllCardLocations() const;

	void TryDraw(ULetheAbilitySystemComponent* OwnerASC) const;
	void SelectCard(ACardActor* CardActor);
	void OnDrawPhaseStarted() const;

public:
	FOnViewCardDetailRequested OnViewCardDetailRequested;
	FOnSelectCardRequested OnSelectCardRequested;
	FOnGoPlayerTurnPhaseRequested OnGoPlayerTurnPhaseRequested;
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

	EPhaseState CurrentPhaseState = EPhaseState::None;

	UPROPERTY()
	TObjectPtr<ADeckBoxes> DeckBoxes;

	/** 캐릭터 순서대로 접근, 혹은 순서에 맞춰 인덱스 기반으로 접근하기 위해 AbilitySystemComponent 배열을 캐싱해둡니다. */
	TArray<TWeakObjectPtr<ULetheAbilitySystemComponent>> AbilitySystemComponents;

	UPROPERTY()
	TArray<TObjectPtr<ACardActor>> SpawnedCards;

	UPROPERTY()
	TObjectPtr<ACardActor> PressedCard;

	UPROPERTY()
	TObjectPtr<ACardActor> CurrentSelectedCard;

	TWeakObjectPtr<UBoxComponent> PressedDeckBox;
	TWeakObjectPtr<UBoxComponent> CurrentSelectedDeckBox;

	UPROPERTY()
	TMap<int32, TObjectPtr<ACardActor>> UseRequestedCards;
};
