// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Lethe/Data/PhaseData.h"
#include "Lethe/SaveGame/SavedCardTypes.h"
#include "HandStage.generated.h"

enum class ECardAction : uint8;
class ACardActor;
class UCardLayoutManager;
class ULetheAbilitySystemComponent;
class USceneCaptureComponent2D;
struct FKey;
struct FCardInitParams;

DECLARE_DELEGATE_OneParam(FOnViewCardDetailRequested, const ACardActor*);
DECLARE_DELEGATE_RetVal_ThreeParams(bool, FOnSelectCardRequested, bool, ULetheAbilitySystemComponent*, const FGameplayTag&);
DECLARE_DELEGATE(FOnGoPlayerTurnPhaseRequested);
DECLARE_DELEGATE(FOnStartResolvePlayerMovesRequested);
DECLARE_DELEGATE_ThreeParams(FOnUseCardRequested, ULetheAbilitySystemComponent*, const FSavedCard&, int32);
DECLARE_DELEGATE_RetVal(bool, FOnTurnEndRequested);

UCLASS(Abstract)
class LETHE_API AHandStage : public AActor
{
	GENERATED_BODY()

public:
	AHandStage();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface

	void Initialize(const FVector2D& CardSize, const TArray<TObjectPtr<ULetheAbilitySystemComponent>>& InAbilitySystemComponents);

	void CreateCard(const FCardInitParams& CardInitParams);

	void HandleCapturedMouseMove(const FVector2D& TargetUV);
	bool HandleCapturedMouseButtonDown(const FVector2D& TargetUV, const FKey& MouseButton);
	bool HandleCapturedMouseButtonUp(const FVector2D& TargetUV, const FKey& MouseButton);
	void HandleCapturedMouseLeave();
	void HandleCapturedMouseCaptureLost();
	bool HandleMouseButtonDownInCardUseSection() const;
	bool HandleMouseButtonUpInCardUseSection();
	void HandleKeyboardEvent(int32 Number);

	void HandlePhaseStateChanged(EPhaseState OldState, EPhaseState NewState);
	void HandleCancelSelectedCard();
	void HandleResolveUseCard(int32 HandIndex, bool bSuccess);
	void HandleTurnEndButtonClicked();
	void CancelSelectedCard() const;

private:
	/** 캡쳐된 이미지와 마우스를 기준으로 라인트레이스를 수행, CardActor를 검출합니다. */
	ACardActor* GetCardActorAtUV(const FVector2D& TargetUV) const;

	void OnCardMouseEvent(ACardActor* CardActor, ECardAction CardAction);
	void OnMouseEventWhenDrawPhase(const ACardActor* CardActor, ECardAction CardAction);
	void OnMouseEventWhenPlayerTurnPhase(ACardActor* CardActor, ECardAction CardAction);
	void OnKeyboardEventWhenDrawPhase(int32 Number);
	void OnKeyboardEventWhenPlayerTurnPhase(int32 Number);

	void UpdateAllCardLocations() const;

	void OnDeckHovered(const ACardActor* CardActor, bool bHovered) const;
	void TryDraw(ULetheAbilitySystemComponent* OwnerASC) const;
	void OnHandHovered(ACardActor* CardActor, bool bHovered) const;
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
	TSubclassOf<ACardActor> CardActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Card | Input")
	float CardTraceDistance = 1000.f;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterial> OutlineMaterial;

private:
	UPROPERTY()
	TObjectPtr<UCardLayoutManager> CardLayoutManager;

	uint8 bInitialized : 1 = false;

	UPROPERTY()
	TArray<TObjectPtr<ULetheAbilitySystemComponent>> AbilitySystemComponents;

	UPROPERTY()
	TArray<TObjectPtr<ACardActor>> SpawnedCards;

	EPhaseState CurrentPhaseState = EPhaseState::None;

	TWeakObjectPtr<ACardActor> HoveredCard;
	TWeakObjectPtr<ACardActor> PressedCard;

	UPROPERTY()
	TObjectPtr<ACardActor> CurrentSelectedCard;

	UPROPERTY()
	TMap<int32, TObjectPtr<ACardActor>> UseRequestedCards;
};
