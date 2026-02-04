// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheProgressBar.h"

#include "Components/ProgressBar.h"
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

void ULetheProgressBar::SetProgressBarStyle(const FProgressBarStyle& FrontProgressBarStyle, const FProgressBarStyle& GhostProgressBarStyle)
{
	FrontProgressBar->SetWidgetStyle(FrontProgressBarStyle);
	GhostProgressBar->SetWidgetStyle(GhostProgressBarStyle);
}

void ULetheProgressBar::SetFillColorAndOpacity(const FLinearColor& InLinearColor)
{
	FrontProgressBar->SetFillColorAndOpacity(InLinearColor);
	GhostProgressBar->SetFillColorAndOpacity(InLinearColor);
}

void ULetheProgressBar::SetBarPercent(const float InPercent, const bool bShouldInterp)
{
	FrontProgressBar->SetPercent(InPercent);

	if (bShouldInterp)
	{
		GetWorld()->GetTimerManager().SetTimer(
			GhostPercentSetTimerHandle,
			this,
			&ThisClass::BarPercentSet,
			GhostStartDelay,
			false);
	}
	else
	{
		GhostProgressBar->SetPercent(InPercent);
	}
}

void ULetheProgressBar::SetPreviewValue(const float InPercent) const
{
	FrontProgressBar->SetPercent(InPercent);
}

void ULetheProgressBar::BarPercentSet()
{
	GhostPercentTarget = FrontProgressBar->GetPercent();

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
