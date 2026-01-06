// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Widget/LetheUserWidget.h"
#include "Lethe/UI/WidgetController/LetheWidgetController.h"
#include "CardPanelWidget.generated.h"

enum class ECardAction : uint8;
struct FCardViewInfo;
class UCardWidget;
class UCanvasPanel;
class UCanvasPanelSlot;
class ULetheAbilitySystemComponent;

// TMap 컨테이너 내부에 TArray를 사용할 수 없는 문제를 우회하기 위한 구조체입니다.
USTRUCT(BlueprintType)
struct FDeck
{
	GENERATED_BODY()

	// CardWidget은 CanvasPanel 내부에 소속되므로, 이를 용이하게 컨트롤하기 위해 UCanvasPanelSlot 배열로 선언합니다.
	UPROPERTY()
	TArray<TObjectPtr<UCanvasPanelSlot>> DeckSlots;

	uint8 bIsHovered : 1 = false;

	// Hand와 Grave는 분류할 필요가 없어 멤버변수로 따로 선언합니다.
};

UCLASS()
class LETHE_API UCardPanelWidget : public ULetheUserWidget
{
	GENERATED_BODY()

public:
	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~ End of UUserWidget Interface

	virtual void WidgetControllerSet_Implementation() override;
	
	void CreateCard(ULetheAbilitySystemComponent* OwnerASC, const FCardViewInfo* InCardInfo);

private:
	void UpdateDeckPosition(const float InDeltaTime);
	void UpdateHandPosition(const float InDeltaTime);
	
	void OnCardMouseEvent(UCardWidget* InCardWidget, const ECardAction InCardAction);

	void DeckHovered(const UCardWidget* InCardWidget, const bool bInHovered);
	void Draw(UCardWidget* InCardWidget);
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	TSubclassOf<UUserWidget> CardWidgetClass;
	
	/**
	 * 덱 위치들입니다. 정확히 4개여야 합니다.
	 * OverlayWidget이 CardPanelWidget을 소유하는데, OverlayWidget이 이 변수의 값을 캐싱해버려서 수정해도 반영되지 않는 현상이 있습니다.
	 * 따라서 OverlayWidget에서 이 변수를 보고 수정할 수 있도록 EditAnywhere로 선언했습니다.
	 * 수정 시 OverlayWidget에서도 함께 수정해 주시길 바랍니다.
	 */
	UPROPERTY(EditAnywhere, Category = "Card")
	TArray<FVector2D> DeckPositions;
	
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	float DeckYPosGap = 3.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	float CardMoveSpeed = 5.f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RootCanvasPanel;

private:
	// Key를 OwnerASC로, Value로 CardWidget(Deck)을 매핑한 변수입니다.
	// 이미 CardWidget도 OwnerASC를 멤버 변수로 갖고 있으나, CardPanelWidget도 정렬 및 접근 효율을 위해 매핑해두는 편이 좋습니다.
	UPROPERTY()
	TMap<TObjectPtr<ULetheAbilitySystemComponent>, FDeck> AbilitySystemComponentToCards;

	// AbilitySystemComponent를 순서대로 참조하기 위해 선언된 변수입니다.
	TArray<FAbilitySystemReference>* AbilitySystemReferences;

	UPROPERTY()
	TArray<TObjectPtr<UCanvasPanelSlot>> Hands;
	
	UPROPERTY()
	TArray<TObjectPtr<UCanvasPanelSlot>> Graves;
};
