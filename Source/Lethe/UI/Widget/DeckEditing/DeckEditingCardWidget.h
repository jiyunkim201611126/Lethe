// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "DeckEditingCardWidget.generated.h"

/**
 * 복잡한 마우스 이벤트나 움직임 로직 없이, View만 담당하는 CardWidget입니다.
 */

class UImage;
class USizeBox;

UCLASS()
class LETHE_API UDeckEditingCardWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	//~ Begin IUserObjectListEntry Interface
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	//~ End of IUserObjectListEntry Interface

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CardBorderImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CardImage;
};
