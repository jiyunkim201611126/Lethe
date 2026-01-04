// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Widget/LetheUserWidget.h"
#include "Lethe/UI/WidgetController/LetheWidgetController.h"
#include "CardPanelWidget.generated.h"

class UCanvasPanel;
class UCardWidget;
class UAbilitySystemComponent;
class ULetheAbilitySystemComponent;
struct FCardViewInfo;

// TMap 안에 TArray를 사용할 수 없는 문제를 우회하기 위한 구조체입니다.
USTRUCT(BlueprintType)
struct FCardWidgets
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UCardWidget>> CardWidgets;
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
	
	void CreateCard(UAbilitySystemComponent* OwnerASC, const FCardViewInfo* InCardInfo);
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	TSubclassOf<UUserWidget> CardWidgetClass;

	// Key를 OwnerASC로, Value로 Card를 매핑한 변수입니다.
	UPROPERTY()
	TMap<UAbilitySystemComponent*, FCardWidgets> AbilitySystemComponentToCards;

	// AbilitySystemComponent를 순서대로 참조하기 위해 선언된 변수입니다.
	TArray<FAbilitySystemReference>* AbilitySystemReferences;
	
	/**
	 * 덱 위치들입니다. 정확히 4개여야 합니다.
	 * OverlayWidget이 CardPanelWidget을 소유하는데, OverlayWidget이 이 변수의 값을 캐싱해버려서 수정해도 반영되지 않는 현상이 있습니다.
	 * 따라서 OverlayWidget에서 이 변수를 보고 수정할 수 있도록 EditAnywhere로 선언했습니다.
	 * 수정 시 OverlayWidget에서도 함께 수정해 주시길 바랍니다.
	 */
	UPROPERTY(EditAnywhere, Category = "Card")
	TArray<FVector2D> DeckPositions;
	
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	float DeckYPosGap = 10.f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RootCanvasPanel;
};
