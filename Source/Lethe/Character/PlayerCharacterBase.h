// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LetheCharacterBase.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "PlayerCharacterBase.generated.h"

UCLASS()
class LETHE_API APlayerCharacterBase : public ALetheCharacterBase, public IPlayerCharacterInterface
{
	GENERATED_BODY()

public:
	APlayerCharacterBase(const FObjectInitializer& ObjectInitializer);

	//~ Begin IPlayerCharacterInterface
	virtual void SetPersonalColor(const FLinearColor& InColor) override;
	virtual const FLinearColor& GetPersonalColor() const override;
	virtual void SetPlayerOrderIndex(const int32 Index) override;
	virtual int32 GetPlayerOrderIndex() const override;
	//~ End of IPlayerCharacterInterface

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWidgetComponent> MarkerWidgetComponent;

private:
	FLinearColor PersonalColor;

	int32 PlayerOrderIndex = INDEX_NONE;
};
