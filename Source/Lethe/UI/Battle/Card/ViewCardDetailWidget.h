// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Framework/LetheUserWidget.h"
#include "ViewCardDetailWidget.generated.h"

class ACardActor;
class UCardPanelWidgetController;
class UCardWidget;
class ULetheRichTextBlock;
class UOverlay;

UCLASS(Abstract)
class LETHE_API UViewCardDetailWidget : public ULetheUserWidget
{
	GENERATED_BODY()

public:
	void StartViewDetail(const UCardWidget* InCardWidget);
	void StartViewDetail(const ACardActor* InCardActor);
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
	TObjectPtr<UCardWidget> DetailCardWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheRichTextBlock> CardDescriptionTextBlock;

private:
	UPROPERTY()
	TObjectPtr<UCardPanelWidgetController> CardPanelWidgetController;
};
