// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Framework/LetheActivatableWidget.h"
#include "SubmitCardWidget.generated.h"

class ACardActor;
class UCardWidget;

UCLASS()
class LETHE_API USubmitCardWidget : public ULetheActivatableWidget
{
	GENERATED_BODY()

public:
	void SetCard(const ACardActor* CardActor);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCardWidget> SubmitCardWidget;
};
