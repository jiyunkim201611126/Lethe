// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Actor.h"
#include "Lethe/SaveGame/SavedCardTypes.h"
#include "CardActor.generated.h"

class UBoxComponent;
class UCardDefinitionData;
class UCardViewData;
class UCharacterDefinitionData;
class UCurveFloat;
class ULetheAbilitySystemComponent;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;
struct FCardInitParams;
struct FViewDetailData;

/**
 * Card가 현재 어디에 속해있는지 나타내는 Enum입니다.
 */
UENUM()
enum class ECardContainer : uint8
{
	Deck,
	Hand,
	Selected,
	Grave,
};

/** 카드 View가 입력을 Action으로 변환할 때 사용하는 원본 마우스 이벤트입니다. */
UENUM()
enum class ECardMouseEvent : uint8
{
	MouseEnter,
	MouseLeave,
	MouseButtonDown,
	LeftMouseButtonUp,
	RightMouseButtonUp,
	MouseCaptureLost
};

/**
 * Card가 어떤 Action을 취해야 하는지 나타내는 Enum입니다.
 * CardActor는 LineTrace로 검출된 뒤 이 Action을 외부로 전달하는 View 역할만 담당합니다.
 */
UENUM()
enum class ECardAction : uint8
{
	DeckHovered,
	DeckUnhovered,
	Draw,
	HandHovered,
	HandUnhovered,
	Selected,
	ViewDetail,

	None,
};

USTRUCT()
struct FViewDetailData
{
	GENERATED_BODY()

	FText CardNameText;

	UPROPERTY()
	UObject* CardImage = nullptr;

	FLinearColor CardTypeColor;
};

class ACardActor;
DECLARE_DELEGATE_TwoParams(FOnCardActorMouseEventSignature, ACardActor*, const ECardAction);

UCLASS(Abstract)
class LETHE_API ACardActor : public AActor
{
	GENERATED_BODY()

public:
	ACardActor();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	//~ End of AActor Interface

	void SetCardInfo(const FCardInitParams& InitParams);
	void MakeViewDetailData(FViewDetailData& OutData) const;

	void SetCardContainer(ECardContainer InCardContainer, bool bShouldSkipAnimation = false);

	/** SceneCapture 화면 좌표에서 LineTrace로 검출한 카드에 대해 외부 입력 라우터가 호출하는 함수입니다. */
	void HandleCardMouseEvent(ECardMouseEvent InMouseEvent);

	void SetTargetTransform(const FTransform& InTransform);
	void MouseHovered(bool bInHovered);
	ECardAction GetCardActionForMouseEvent(ECardMouseEvent InMouseEvent) const;

	FGameplayTag GetCardTag() const;
	const FSavedCard& GetSavedCard() const;
	ECardContainer GetCurrentCardContainer() const;
	ULetheAbilitySystemComponent* GetOwnerASC() const;

private:
	UFUNCTION()
	void OnUpdatedTimeline(float InValue);

	UFUNCTION()
	void OnFinishedTimeline();

	void CreateDynamicMaterialInstances();
	void ApplyCardVisuals() const;
	void ToggleHighlightOutline(const bool bHighlightOn) const;

	/** 드로우 직후 HandHover가 안 되도록 잠깐 막아줍니다. */
	void StartBlockHandHoverTimer();

	ECardAction GetCardActionWhenDeckState(ECardMouseEvent InMouseEvent) const;
	ECardAction GetCardActionWhenHandState(ECardMouseEvent InMouseEvent) const;
	ECardAction GetCardActionWhenSelectedState(ECardMouseEvent InMouseEvent) const;

	FTransform GetTargetTransformWithHoverOffset() const;
	void FinishMovementImmediately();

public:
	FOnCardActorMouseEventSignature OnCardMouseEventDelegate;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<USceneComponent> CardRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UBoxComponent> InteractionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UStaticMeshComponent> CardMesh;

	/**
	 * Card의 경우 2D 이미지로 캡쳐되어 위젯으로 표시되기 때문에, 일반적인 Outline으로는 구현이 어렵습니다.
	 * 따라서 CardMesh보다 약간 큰 메쉬를 원하는 색상으로 설정해 하이라이팅을 구현합니다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UStaticMeshComponent> CardOutlineMesh;

	UPROPERTY(EditAnywhere, Category = "Card | Animation")
	TObjectPtr<UCurveFloat> MovementCurve;

	UPROPERTY(EditAnywhere, Category = "Card | Animation")
	FVector HoveredLocalOffset = FVector(0.f, 0.f, 10.f);

	UPROPERTY(EditAnywhere, Category = "Card | Material")
	FName CardTextureParamName = TEXT("CardTexture");

	UPROPERTY(EditAnywhere, Category = "Card | Material")
	FName TypeFrameColorParamName = TEXT("TypeFrameColor");

	UPROPERTY(EditAnywhere, Category = "Card | Material")
	FName SortFrameColorParamName = TEXT("SortFrameColor");

	UPROPERTY(EditAnywhere, Category = "Card | Material")
	FName CharacterColorParamName = TEXT("CharacterColor");

private:
	FText CardNameText;
	FSavedCard SavedCard;
	FLinearColor CardTypeColor = FLinearColor::White;
	FLinearColor CharacterColor = FLinearColor::White;

	UPROPERTY()
	TObjectPtr<UTexture2D> CardImage;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CardMaterialInstance;

	FTimeline MovementTimeline;
	FTransform StartTransform;
	FTransform TargetTransform;

	ECardContainer CurrentCardContainer = ECardContainer::Deck;

	uint8 bShouldMove : 1 = false;
	uint8 bBlockHandHover : 1 = false;
	uint8 bMouseHovered : 1 = false;

	TWeakObjectPtr<ULetheAbilitySystemComponent> OwnerASC;
};
