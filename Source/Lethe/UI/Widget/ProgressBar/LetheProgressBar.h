// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LetheProgressBar.generated.h"

class UProgressBar;

/**
 * ProgressBar 2개로 구현되는 프로젝트 기본 ProgressBar입니다.
 * Front는 값이 바로 변경되며, Ghost는 타이머를 통해 Front를 뒤따라갑니다.
 * 3개의 함수를 통해 Ghost가 조정되며, 블루프린트에서 오버라이드해 사용하는 것도 가능합니다.
 */
UCLASS()
class LETHE_API ULetheProgressBar : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetProgressBarStyle(const FProgressBarStyle& FrontProgressBarStyle, const FProgressBarStyle& GhostProgressBarStyle);

	UFUNCTION(BlueprintCallable)
	void SetFillColorAndOpacity(const FLinearColor& InLinearColor);
	
	void SetBarPercent(const float InPercent, const bool bShouldInterp = true);
	void SetPreviewBarPercent(const float InPercent);
	void StopPreview(const float InPercent);

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeDestruct() override;
	//~ End of UUserWidget Interface
	
	void BarPercentSet();
	void InterpGhostBar();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> FrontProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> GhostProgressBar;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FrontBarBlinkingAnimation;

private:
	FTimerHandle GhostPercentSetTimerHandle;
	FTimerHandle PercentInterpTimerHandle;

	float GhostPercentTarget = 0.5f;
	float GhostStartDelay = 1.f;
	float GhostInterpDelay = 0.05f;
	float GhostInterpSpeed = 5.f;
};
