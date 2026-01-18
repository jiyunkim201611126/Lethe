// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Widget/LetheUserWidget.h"
#include "Lethe/UI/Widget/Card/CardPanelWidget.h"
#include "DeckEditingWidget.generated.h"

class UButton;
class UWrapBox;
struct FGameplayTag;
struct FCardSelfViewInfo;
struct FCardOwnerViewInfo;
class UCardViewData;
class UCardWidget;

UCLASS()
class LETHE_API UDeckEditingWidget : public ULetheUserWidget
{
	GENERATED_BODY()

public:
	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	//~ End of UUserWidget Interface

private:
	void CreateCard(const FGameplayTag& InCardTag, const FCardSelfViewInfo* CardSelfViewInfo, const FCardOwnerViewInfo* CardOwnerViewInfo);
	void OnCardMouseEvent(UCardWidget* InCardWidget, ECardAction InCardAction);

protected:
	/**
	 * DeckEditing의 경우엔 WidgetController가 굳이 필요 없습니다.
	 * WidgetController는 보통 Model의 값 변화에 Widget이 바로바로 대응해야 하는 경우 사용합니다.
	 * 하지만 DeckEditing은 초기 Deck 상태를 Load해서 한 번 보여주고, 편집이 끝나면 Save 로직을 진행하며 위젯은 더이상 필요하지 않게 됩니다.
	 * 따라서 Model(Deck)의 상태 변화를 감시하고 있을 이유가 없으므로 위젯이 직접 Deck 정보를 가진 Subsystem에 접근하고, ViewData 또한 갖고 있는 방식으로 구현합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UCardViewData> CardViewData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TSubclassOf<UCardWidget> CardWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> CardWrapBox;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GoToBattleButton;
};
