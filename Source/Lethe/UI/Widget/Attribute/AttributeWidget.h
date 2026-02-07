// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Widget/LetheUserWidget.h"
#include "AttributeWidget.generated.h"

class ULetheTextBlock;
class ULetheProgressBar;

UCLASS()
class LETHE_API UAttributeWidget : public ULetheUserWidget
{
	GENERATED_BODY()

protected:
	//~ Begin ULetheUserWidget Interface
	virtual void WidgetControllerSet_Implementation() override;
	//~ End of ULetheUserWidget Interface
	
	void OnHealthChanged(const float NewValue);
	void OnMaxHealthChanged(const float NewValue);

	bool IsPreviewing(const float PreviewDeltaValue) const;
	
private:
	void UpdateHealthUI() const;
	
	void OnHealthPreviewValueChanged(const float PreviewDeltaValue);
	void OnMaxHealthPreviewValueChanged(const float PreviewDeltaValue);
	void StartHealthPreview() const;
	void StopHealthPreview() const;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<ULetheProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheTextBlock> HealthText;

private:
	/**
	 * Preview 시스템을 구현하기 위해선 Attribute를 반드시 캐싱해놓아야 합니다.
	 * 기본적으로 V를 위한 데이터를 캐싱 및 가공하는 건 VM의 책임입니다.
	 * 그러나 AttributeWidgetController는 생산성을 위해 델리게이트를 통합했으므로, 개별 함수를 가질 수 없습니다.
	 * 따라서 Widget이 직접 캐싱해 사용합니다.
	 *
	 * 이는 다른 Widget과 값을 공유할 수 없기 때문에, 다른 곳에서 Health Percent 같은 값을 요구하게 될 경우 리팩토링 가능성이 있습니다.
	 */
	float Health = 0.f;
	float MaxHealth = 0.f;
	
	float PreviewDeltaHealth = 0.f;
	float PreviewDeltaMaxHealth = 0.f;
};
