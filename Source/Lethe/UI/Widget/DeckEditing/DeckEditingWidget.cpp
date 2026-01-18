// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckEditingWidget.h"

#include "Components/WrapBox.h"
#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"
#include "Lethe/Data/CardViewData.h"
#include "Lethe/Manager/LetheSaveManagerSubsystem.h"
#include "Lethe/UI/Widget/Card/CardWidget.h"

void UDeckEditingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULetheSaveManagerSubsystem* SaveManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<ULetheSaveManagerSubsystem>())
	{
		const TMap<FGameplayTag, FSavedCharacterDeck>& UnlockedCards = SaveManagerSubsystem->GetUnlockedCards();
		
		for (const auto& UnlockedCard : UnlockedCards)
		{
			for (const auto& Card : UnlockedCard.Value.Cards)
			{
				ULetheGameplayAbility* CardAbilityCDO = Card.CardAbility.GetDefaultObject();
				FCardSelfViewInfo* CardSelfViewInfo = CardViewData->FindCardSelfViewInfoByTag(CardAbilityCDO->CardTag);
				if (CardSelfViewInfo->CardDescriptionText.IsEmpty())
				{
					const FText CardDescriptionText = CardAbilityCDO->GetCardDescription(Card.CardLevel);
					CardSelfViewInfo->CardDescriptionText = CardDescriptionText;
				}

				const FCardOwnerViewInfo* CardOwnerViewInfo = CardViewData->FindCardOwnerViewInfoByTag(UnlockedCard.Key);

				CreateCard(CardAbilityCDO->CardTag, CardSelfViewInfo, CardOwnerViewInfo);
			}
		}
	}
}

void UDeckEditingWidget::CreateCard(const FGameplayTag& InCardTag, const FCardSelfViewInfo* CardSelfViewInfo, const FCardOwnerViewInfo* CardOwnerViewInfo)
{
	if (UCardWidget* CreatedCard = CreateWidget<UCardWidget>(this, CardWidgetClass))
	{
		CreatedCard->SetCardInfo(InCardTag, CardSelfViewInfo, CardOwnerViewInfo);
		CreatedCard->SetSize(CardViewData->GetCardSize());

		// 실제로 Hand는 아니지만, 애니메이션 재생 등 기존 로직 활용을 위해 Hand로 설정합니다.
		CreatedCard->SetCardContainer(ECardContainer::Hand, true);
		
		CreatedCard->OnCardMouseEventDelegate.BindUObject(this, &ThisClass::OnCardMouseEvent);

		CardWrapBox->AddChild(CreatedCard);
	}
}

void UDeckEditingWidget::OnCardMouseEvent(UCardWidget* InCardWidget, const ECardAction InCardAction)
{
	switch (InCardAction)
	{
	case ECardAction::Drag:
		{
			// 우선은 MouseButtonDown 시 발생하는 Drag 이벤트를 그대로 이용합니다.
			
		}
		break;
	default:
		break;
	}
}
