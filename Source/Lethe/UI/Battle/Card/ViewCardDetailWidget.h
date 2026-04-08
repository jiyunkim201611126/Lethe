// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Framework/LetheUserWidget.h"
#include "ViewCardDetailWidget.generated.h"

class UCardPanelWidgetController;
class ULetheRichTextBlock;
class UOverlay;
class ULetheImage;
class UCardWidget;

UCLASS()
class LETHE_API UViewCardDetailWidget : public ULetheUserWidget
{
	GENERATED_BODY()

public:
	void StartViewDetail(const UCardWidget* InCardWidget);
	void EndViewDetail();

protected:
	//~ Begin UUserWidget Interface
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//~ End of UUserWidget Interface

	//~ Begin ULetheUserWidget Interface
	virtual void WidgetControllerSet_Implementation() override;
	//~ End of ULetheUserWidget Interface

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> CardOverlay;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> CardImage;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> CardFrontsideBorderImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheRichTextBlock> CardDescriptionTextBlock;

private:
	UPROPERTY()
	TObjectPtr<UCardPanelWidgetController> CardPanelWidgetController;
};
