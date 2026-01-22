// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LetheWidgetController.h"
#include "CardPanelWidgetController.generated.h"

struct FGameplayAbilitySpecHandle;
class UCardOwnerViewData;
class UCardSelfViewData;
class UCardDefinitionData;
struct FGameplayAbilitySpec;
struct FGameplayTag;
struct FCardSelfViewInfo;
class ULetheGameplayAbility;
class UCardViewData;
enum class EPlayerPhaseState : uint8;

/**
 * CardWidget을 생성해 초기화하는 시점에 필요한 데이터입니다.
 * 편의성을 위해 선언되었으며, 로컬 변수로 생성되어 잠시 사용되고 사라지기 때문에 내부는 원시 포인터로 사용합니다.
 */
USTRUCT()
struct FCardInitParams
{
	GENERATED_BODY()

	UPROPERTY()
	ULetheAbilitySystemComponent* OwnerASC;
	
	UPROPERTY()
	UCardViewData* CardViewData;

	UPROPERTY()
	const UCardDefinitionData* CardDefinition;

	UPROPERTY()
	const UCardSelfViewData* CardSelfViewData;

	UPROPERTY()
	const UCardOwnerViewData* CardOwnerViewData;
};

DECLARE_DELEGATE_OneParam(FOnAbilityUpdatedSignature, const FCardInitParams&)

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerPhaseStateChangedSignature, const EPlayerPhaseState);

UCLASS(Abstract, Blueprintable)
class LETHE_API UCardPanelWidgetController : public ULetheWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin ULetheWidgetController Interface
	virtual void BindCallbacksToDependencies(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS) override;
	virtual void BroadcastInitialValue() override;
	//~ End of ULetheWidgetController Interface

	FVector2D GetCardSize() const;
	float GetCardHighlightScale() const;

	void GoDrawPhase() const;
	void GoBattlePhase() const;

private:
	void OnGiveAbility(ULetheAbilitySystemComponent* OwnerASC, const UCardDefinitionData* CardDefinitionData, const UCardSelfViewData* CardSelfViewData, const UCardOwnerViewData* CardOwnerViewData) const;
	void OnPlayerPhaseChanged(const EPlayerPhaseState InState) const;

public:
	FOnAbilityUpdatedSignature OnAbilityUpdatedDelegate;
	FOnPlayerPhaseStateChangedSignature OnPlayerPhaseStateChangedDelegate;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UCardViewData> CardViewData;

private:
	uint8 bInitialized : 1 = false;
};
