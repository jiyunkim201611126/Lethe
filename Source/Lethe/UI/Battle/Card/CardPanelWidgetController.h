// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "Lethe/SaveGame/SavedCardTypes.h"
#include "Lethe/UI/Framework/LetheWidgetController.h"
#include "CardPanelWidgetController.generated.h"

class ALethePlayerController;
class ALetheGameState;
class UCardDefinitionData;
class UCardViewData;
class UCharacterDefinitionData;
struct FGrantedCardAbilityInfo;
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
	ULetheAbilitySystemComponent* CardOwnerASC = nullptr;

	UPROPERTY()
	const UCharacterDefinitionData* CharacterDefinitionData = nullptr;

	UPROPERTY()
	const UCardDefinitionData* CardDefinition = nullptr;

	FSavedCard SavedCard;

	FGameplayAbilitySpecHandle AbilitySpecHandle;

	UPROPERTY()
	UCardViewData* CardViewData = nullptr;
};

DECLARE_DELEGATE_OneParam(FOnAbilityUpdated, const FCardInitParams&)
DECLARE_DELEGATE(FOnAbilitySystemReferencesUpdated);
DECLARE_DELEGATE_OneParam(FOnCardSelected, const int32 /* HandSlotIndex */);
DECLARE_DELEGATE(FOnCardSelectCanceled);
DECLARE_DELEGATE_TwoParams(FOnUseCardResolved, const int32 /* HandSlotIndex */, const bool /* bSuccess */);

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

	void NotifyDrawPhaseCompleted() const;
	void RequestTurnEnd() const;
	
	void UpdateMouseInWorldSection(const bool bIsMouseInWorldSection) const;
	void HandleLeftMouseButtonClickedInWorldSection() const;
	void ResetSelectedCharacter() const;
	
	void StartResolvePlayerMoves() const;
	
	void OnSelectCardRequested(const int32 HandSlotIndex, ULetheAbilitySystemComponent* CardOwnerASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle) const;
	void ResetSelectedCard() const;
	void RequestUseCard(ULetheAbilitySystemComponent* CardOwnerASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle, const FGameplayTag& CardTag, int32 InHandSlotIndex) const;
	
	bool IsCardSelected() const;

private:
	void OnGiveAbility(const FGrantedCardAbilityInfo& GrantedCardAbilityInfo) const;
	void OnSelectCard(const int32 HandSlotIndex) const;
	void OnCancelCardSelect() const;
	void OnResolveUseCard(const int32 HandSlotIndex, const bool bSuccess) const;

public:
	FOnAbilityUpdated OnAbilityUpdatedDelegate;
	FOnAbilitySystemReferencesUpdated OnAbilitySystemReferencesUpdatedDelegate;
	FOnCardSelected OnCardSelectedDelegate;
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
