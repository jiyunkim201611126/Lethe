// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Lethe/SaveGame/SavedCardTypes.h"
#include "Lethe/UI/Framework/LetheWidgetController.h"
#include "CardPanelWidgetController.generated.h"

enum class EPhaseState : uint8;
class ALethePlayerController;
class ALetheGameState;
class UCardDefinitionData;
class UCardViewData;
class UCharacterDefinitionData;
class ULetheGameplayAbility;
struct FGameplayEventData;
struct FGameplayAbilitySpec;
struct FGameplayAbilitySpecHandle;
struct FSavedCard;

/**
 * CardWidget을 생성해 초기화하는 시점에 필요한 데이터입니다.
 * 편의성을 위해 선언되었으며, 로컬 변수로 생성되어 잠시 사용되고 사라지기 때문에 내부는 원시 포인터로 사용합니다.
 */
USTRUCT()
struct FCardInitParams
{
	GENERATED_BODY()

	UPROPERTY()
	ULetheAbilitySystemComponent* OwnerASC = nullptr;

	UPROPERTY()
	const UCharacterDefinitionData* CharacterDefinitionData = nullptr;

	UPROPERTY()
	const UCardDefinitionData* CardDefinition = nullptr;

	FSavedCard SavedCard;

	UPROPERTY()
	UCardViewData* CardViewData = nullptr;
};

DECLARE_DELEGATE_OneParam(FOnAbilityUpdated, const FCardInitParams&)
DECLARE_DELEGATE(FOnAbilitySystemReferencesUpdated);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPhaseStateChanged, const EPhaseState /* OldState */, const EPhaseState /* NewState */);
DECLARE_DELEGATE(FOnCardSelectCanceled);
DECLARE_DELEGATE_TwoParams(FOnUseCardResolved, const int32, const bool);

UCLASS(Abstract, Blueprintable)
class LETHE_API UCardPanelWidgetController : public ULetheWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin ULetheWidgetController Interface
	virtual void SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams) override;
	virtual void BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS, UPlayerAttributeSet* PAS) override;
	//~ End of ULetheWidgetController Interface

	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
	//~ End of UObject Interface

	bool SetCardSelected(bool bInCardSelected, ULetheAbilitySystemComponent* OwnerASC = nullptr, const FGameplayTag& CardTag = FGameplayTag()) const;

	void GoPlayerTurnPhase() const;
	void StartResolvePlayerMoves() const;
	bool RequestTurnEnd() const;
	void RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, int32 InHandIndex) const;

	void GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, FText& OutText) const;

private:
	void OnGiveAbility(ULetheAbilitySystemComponent* OwnerASC, const UCharacterDefinitionData* CharacterDefinitionData, const UCardDefinitionData* CardDefinitionData, const FSavedCard& SavedCard) const;
	void OnPhaseStateChanged(const EPhaseState OldState, const EPhaseState NewState) const;
	void OnCancelCardSelect() const;
	void OnResolveUseCard(const int32 HandIndex, const bool bSuccess) const;

public:
	FOnAbilityUpdated OnAbilityUpdatedDelegate;
	FOnAbilitySystemReferencesUpdated OnAbilitySystemReferencesUpdatedDelegate;
	FOnPhaseStateChanged OnPhaseStateChangedDelegate;
	FOnCardSelectCanceled OnCardSelectCanceledDelegate;
	FOnUseCardResolved OnUseCardResolvedDelegate;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UCardViewData> CardViewData;

private:
	uint8 bInitialized : 1 = false;

	UPROPERTY()
	TObjectPtr<ALethePlayerController> LethePlayerController;

	TWeakObjectPtr<ALetheGameState> LetheGameState;
};
