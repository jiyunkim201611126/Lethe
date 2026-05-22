// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GASManagerComponent.h"
#include "PlayerGASManagerComponent.generated.h"

class UPlayerAttributeSet;

UCLASS(NotBlueprintable)
class LETHE_API UPlayerGASManagerComponent : public UGASManagerComponent
{
	GENERATED_BODY()

public:
	UPlayerGASManagerComponent(const FObjectInitializer& ObjectInitializer);

	virtual void SetPlayerAttributeSet(UPlayerAttributeSet* InPlayerAttributeSet) override;

protected:
	virtual void InitUI(const TArray<UUserWidget*>& AttributeWidgets) override;
	
	virtual void OnPlanPhaseStarted() const override;

protected:
	/** 턴 시작 시 Cost와 Mana를 회복하는 GameplayEffect입니다. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> TurnStartRecovery;

	UPROPERTY()
	TObjectPtr<UPlayerAttributeSet> PlayerAttributeSet;
};
