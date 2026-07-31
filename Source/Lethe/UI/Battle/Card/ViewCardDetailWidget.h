// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Framework/LetheActivatableWidget.h"
#include "ViewCardDetailWidget.generated.h"

class ACardActor;
class UCardWidget;
class UInputAction;
class ULetheRichTextBlock;
class UOverlay;
class UViewCardDetailWidgetController;

UCLASS(Abstract)
class LETHE_API UViewCardDetailWidget : public ULetheActivatableWidget
{
	GENERATED_BODY()

public:
	//~ Begin ULetheActivatableWidget Interface
	virtual void WidgetControllerSet_Implementation() override;
	//~ End of ULetheActivatableWidget Interface
	
	void StartViewDetail(const ACardActor* InCardActor);
	
	//~ Begin UCommonActivatableWidget Interface
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	//~ End of UCommonActivatableWidget Interface

protected:
	//~ Begin UUserWidget Interface
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//~ End of UUserWidget Interface

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCardWidget> DetailCardWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheRichTextBlock> CardDescriptionTextBlock;

private:
	UPROPERTY()
	TObjectPtr<UViewCardDetailWidgetController> ViewCardDetailWidgetController;
};
