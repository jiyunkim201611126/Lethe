// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Framework/LetheUserWidget.h"
#include "CardWidget.generated.h"

class UInvalidationBox;
class ULetheImage;
class USizeBox;
struct FCardInitParams;
struct FViewDetailData;

UCLASS()
class LETHE_API UCardWidget : public ULetheUserWidget
{
	GENERATED_BODY()

public:
	void SetSize(const FVector2D& InSize) const;
	void SetCardInfo(const FCardInitParams& InitParams) const;

	void SetViewDetail(const FViewDetailData& InData) const;

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
