// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "CardWidget.generated.h"

class UCardWidgetInitContext;
class UInvalidationBox;
class ULetheImage;
class USizeBox;
struct FCardInitParams;

UCLASS(Abstract)
class LETHE_API UCardWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/** TileView에서 사용되지 않고, ViewCardDetail 등의 상황에서 사용합니다. */
	void InitCardView(const UCardWidgetInitContext* InContext) const;

protected:
	//~ Begin IUserObjectListEntry Interface
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	//~ End of IUserObjectListEntry Interface

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> RootSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInvalidationBox> InvalidationBox;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> CardImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> CardBorderImage;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> TypeFrameImage;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> SortFrameImage;
};
