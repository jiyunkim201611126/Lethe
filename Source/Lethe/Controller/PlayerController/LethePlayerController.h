// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "LethePlayerController.generated.h"

class ATile;
class AArrowRenderer;
class UAbilitySystemComponent;
class ULetheAbilitySystemComponent;
class ULetheCardAbility;
class ULetheHUD;
class UPreviewCoordinatorComponent;
class UTileSelectorComponent;
struct FGameplayAbilityActorInfo;
struct FPreviewData;

DECLARE_DELEGATE_OneParam(FOnNumberKeyPressedSignature, const int32 /* InNumber */);
DECLARE_MULTICAST_DELEGATE(FOnCardSelectCanceledSignature);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPreviewDataUpdatedSignature, const FPreviewData&);
DECLARE_DELEGATE_TwoParams(FOnResolveUseCardSignature, const int32 /* HandIndex */, const bool /* bSuccess */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCameraHeightChangedSignature, const float /* AttributeWidgetSize */);

UCLASS()
class LETHE_API ALethePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALethePlayerController();

	void OnNumberPressed(const int32 InNumber) const;
	void OnWheeled(const float AttributeWidgetSize) const;
	void OnLeftMouseButtonClickedOnWorld();
	void ResetSelectedCharacter();
	
	bool SetCardSelected(const bool bInCardSelected, ULetheAbilitySystemComponent* OwnerASC = nullptr, const FGameplayTag& CardTag = FGameplayTag());
	void SetMouseOnCardUseSection(const bool bInMouseOnCardUseSection);
	void RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, const int32 InHandIndex) const;

	void GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, FText& OutText) const;

	ULetheHUD* GetLetheHUD() const;
	UPreviewCoordinatorComponent* GetPreviewCoordinatorComponent() const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	//~ End of AActor Interface

private:
	ATile* GetTileUnderCursor() const;
	
	// 카드 선택 상태에서 마우스를 움직여서 다른 Tile이 검출되면 호출되는 콜백 함수입니다.
	void OnOtherTileDetected(const AActor* LastActor, const AActor* CurrentActor) const;

	void OnUpdatePreviewData(const FPreviewData& PreviewData);

public:
	FOnNumberKeyPressedSignature OnNumberKeyPressedDelegate;
	FOnCardSelectCanceledSignature OnCardSelectCanceledDelegate;
	FOnPreviewDataUpdatedSignature OnPreviewDataUpdatedDelegate;
	FOnResolveUseCardSignature OnResolveUseCardDelegate;
	FOnCameraHeightChangedSignature OnCameraHeightChangedDelegate;
	
protected:
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "LetheHUD")
	TObjectPtr<ULetheHUD> LetheHUD;
	
private:
	UPROPERTY()
	TObjectPtr<UTileSelectorComponent> TileSelector;

	UPROPERTY()
	TObjectPtr<UPreviewCoordinatorComponent> PreviewCoordinatorComponent;
	
	uint8 bMouseOnCardUseSection : 1 = false;
	
	// CDO를 캐싱할 멤버변수기 때문에 템플릿에도 const를 붙여줍니다.
	TWeakObjectPtr<const ULetheCardAbility> SelectedCardAbility;
	TWeakObjectPtr<UAbilitySystemComponent> SelectedCardOwnerASC;

	UPROPERTY(EditDefaultsOnly, Category = "ArrowRenderer")
	TSubclassOf<AArrowRenderer> ArrowRendererClass;

	UPROPERTY()
	TObjectPtr<AArrowRenderer> ArrowRenderer;

	TWeakObjectPtr<AActor> SelectedCharacter;
};
