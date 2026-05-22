// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeWidgetController.h"
#include "Lethe/Data/PhaseData.h"
#include "PlayerAttributeWidgetController.generated.h"

class ULetheCardAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMarkerVisibilityChanged, const ESlateVisibility, Visibility);

UCLASS(Abstract, Blueprintable)
class LETHE_API UPlayerAttributeWidgetController : public UAttributeWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin ULetheWidgetController Interface
	virtual void BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS, UPlayerAttributeSet* PAS) override;
	//~ End of ULetheWidgetController Interface

protected:
	virtual void StartAllPreview(const TMap<FGameplayTag, float>& InPreviewData) override;
	virtual void StopAllPreview() override;

private:
	void OnManaChanged(const FOnAttributeChangeData& AttributeData);
	void BroadcastManaChanged() const;
	
	void OnCostChanged(const FOnAttributeChangeData& AttributeData);
	void BroadcastCostChanged() const;
	
	void OnMoveDistanceChanged(const FOnAttributeChangeData& AttributeData);
	void OnPhaseStateChanged(const EPhaseState OldPhase, const EPhaseState NewPhase);
	void BroadcastMarkerVisibilityChanged() const;

public:
	UPROPERTY(BlueprintAssignable)
	FOnMarkerVisibilityChanged OnMarkerVisibilityChanged;
	
private:
	uint8 bHasRemainingMoveDistance : 1 = false;
	EPhaseState CurrentPhaseState = EPhaseState::None;
};
