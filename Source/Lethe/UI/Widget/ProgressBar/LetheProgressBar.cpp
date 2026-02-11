// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheProgressBar.h"

#include "Components/ProgressBar.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"

void ULetheProgressBar::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(GhostPercentSetTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(PercentInterpTimerHandle);
	}
	
	Super::NativeDestruct();
}

void ULetheProgressBar::SetProgressBarStyle(const FProgressBarStyle& FrontProgressBarStyle, const FProgressBarStyle& GhostProgressBarStyle, UMaterialInterface* PreviewProgressBarMaterial)
{
	FrontProgressBar->SetWidgetStyle(FrontProgressBarStyle);
	GhostProgressBar->SetWidgetStyle(GhostProgressBarStyle);

	if (PreviewProgressBarMaterial)
	{
		FProgressBarStyle PreviewProgressBarStyle = FrontProgressBarStyle;
		FSlateBrush PreviewBrush = PreviewProgressBarStyle.FillImage;
		PreviewBarDynamicMaterialInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, PreviewProgressBarMaterial);
		PreviewBrush.SetResourceObject(PreviewBarDynamicMaterialInstance);
		PreviewProgressBarStyle.SetFillImage(PreviewBrush);
		PreviewProgressBar->SetWidgetStyle(PreviewProgressBarStyle);
		PreviewProgressBar->SynchronizeProperties();
	}
}

void ULetheProgressBar::SetFillColorAndOpacity(const FLinearColor& InLinearColor)
{
	FrontProgressBar->SetFillColorAndOpacity(InLinearColor);
	PreviewProgressBar->SetFillColorAndOpacity(InLinearColor);
	GhostProgressBar->SetFillColorAndOpacity(InLinearColor);
}

void ULetheProgressBar::SetBarPercent(const float InPercent, const bool bShouldInterp)
{
	FrontProgressBar->SetPercent(InPercent);
	PreviewProgressBar->SetVisibility(ESlateVisibility::Collapsed);

	if (bShouldInterp)
	{
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this, InPercent]()
		{
			BarPercentSet(InPercent);
		});
		
		GetWorld()->GetTimerManager().SetTimer(
			GhostPercentSetTimerHandle,
			TimerDelegate,
			GhostStartDelay,
			false);
	}
	else
	{
		GhostProgressBar->SetPercent(InPercent);
	}
}

void ULetheProgressBar::SetPreviewBarPercent(const float InPercent)
{
	PlayPreviewBlinking(GetWorld()->GetTimeSeconds());
	PreviewProgressBar->SetPercent(FrontProgressBar->GetPercent());
	PreviewProgressBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	FrontProgressBar->SetPercent(InPercent);
}

void ULetheProgressBar::StopPreview(const float InPercent)
{
	PreviewProgressBar->SetVisibility(ESlateVisibility::Collapsed);
	FrontProgressBar->SetPercent(InPercent);
}

void ULetheProgressBar::BarPercentSet(const float InPercent)
{
	GhostPercentTarget = InPercent;

	GetWorld()->GetTimerManager().ClearTimer(PercentInterpTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		PercentInterpTimerHandle,
		this,
		&ThisClass::InterpGhostBar,
		GhostInterpDelay,
		true);
}

void ULetheProgressBar::InterpGhostBar()
{
	const float CurrentGhostBarPercent = GhostProgressBar->GetPercent();
	const float NextGhostBarPercent = UKismetMathLibrary::FInterpTo(CurrentGhostBarPercent, GhostPercentTarget, GetWorld()->GetDeltaSeconds(), GhostInterpSpeed);
	GhostProgressBar->SetPercent(NextGhostBarPercent);

	if (UKismetMathLibrary::NearlyEqual_FloatFloat(NextGhostBarPercent, GhostPercentTarget))
	{
		GetWorld()->GetTimerManager().ClearTimer(PercentInterpTimerHandle);
	}
}
