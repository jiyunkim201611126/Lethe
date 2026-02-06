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
	
	void OnHealthPreviewValueChanged(const float PreviewDeltaValue);
	void OnMaxHealthPreviewValueChanged(const float PreviewDeltaValue);
	void StartHealthPreview() const;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<ULetheProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheTextBlock> HealthText;

private:
	/**
	 * Preview 구현을 위해 Attribute를 캐싱해놓는 변수들입니다.
	 * 기본적으로 Widget을 위한 데이터를 캐싱하는 건 WidgetController의 책임입니다.
	 * 그러나 AttributeWidgetController는 생산성을 위해 콜백 함수를 통합했으므로, 개별 함수를 가질 수 없습니다.
	 * 따라서 Widget이 직접 캐싱해 사용합니다.
	 */
	float Health = 0.f;
	float MaxHealth = 0.f;
	
	float PreviewDeltaHealth = 0.f;
	float PreviewDeltaMaxHealth = 0.f;
};
