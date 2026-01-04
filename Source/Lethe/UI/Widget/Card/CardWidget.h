// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Widget/LetheUserWidget.h"
#include "CardWidget.generated.h"

class UImage;
struct FCardViewInfo;

UENUM()
enum class ECardPosition : uint8
{
	Deck,
	Hand,
	Grave,
};

UCLASS()
class LETHE_API UCardWidget : public ULetheUserWidget
{
	GENERATED_BODY()

public:
	void UpdateCardView(const FCardViewInfo* InCardInfo) const;

public:
	ECardPosition CurrentCardPosition = ECardPosition::Deck;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CardImage;
};
