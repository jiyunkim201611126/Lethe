// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Widget/LetheWidgetController.h"
#include "Lethe/UI/Widget/BattleWidget/CardPanelWidget.h"
#include "Lethe/UI/Widget/BattleWidget/CardWidget.h"
#include "CardPanelWidgetController.generated.h"

class ALethePlayerController;
class ALetheGameState;
class UCardDefinitionData;
class UCardSelfViewData;
class UCardViewData;
class UCharacterDefinitionData;
class ULetheGameplayAbility;
struct FCardSelfViewInfo;
struct FGameplayAbilitySpec;
struct FGameplayAbilitySpecHandle;
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
	const UCharacterDefinitionData* CharacterDefinitionData;
};

DECLARE_DELEGATE_OneParam(FOnAbilityUpdated, const FCardInitParams&)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerPhaseStateChanged, const EPlayerPhaseState);
DECLARE_DELEGATE_OneParam(FOnNumberKeyPressed, const int32);

UCLASS(Abstract, Blueprintable)
class LETHE_API UCardPanelWidgetController : public ULetheWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin ULetheWidgetController Interface
	virtual void BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS) override;
	virtual void BroadcastInitialValue() override;
	//~ End of ULetheWidgetController Interface

	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
	//~ End of UObject Interface

	FVector2D GetCardSize() const;
	float GetCardExpandScale() const;
	
	void SetCardSelected(bool bInCardSelected, const ULetheAbilitySystemComponent* OwnerASC = nullptr, const FGameplayTag& CardTag = FGameplayTag()) const;

	void GoDrawPhase() const;
	void GoBattlePhase() const;
	bool RequestTurnEnd() const;
	bool RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag) const;

private:
	void OnGiveAbility(ULetheAbilitySystemComponent* OwnerASC, const UCardDefinitionData* CardDefinitionData, const UCardSelfViewData* CardSelfViewData, const UCharacterDefinitionData* CharacterDefinitionData) const;
	void OnPlayerPhaseChanged(const EPlayerPhaseState InState) const;
	void OnNumberKeyPressed(int32 InNumber) const;

public:
	FOnAbilityUpdated OnAbilityUpdatedDelegate;
	FOnPlayerPhaseStateChanged OnPlayerPhaseStateChangedDelegate;
	FOnNumberKeyPressed OnNumberKeyPressedDelegate;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UCardViewData> CardViewData;

private:
	uint8 bInitialized : 1 = false;
	
	UPROPERTY()
	TObjectPtr<ALethePlayerController> LethePlayerController;

	TWeakObjectPtr<ALetheGameState> LetheGameState;
};
