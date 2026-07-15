// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameUIFeature.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "Lethe/UI/Framework/LethePrimaryGameLayout.h"

void ULetheGameUIFeature::InitializeFeature(ULethePrimaryGameLayout* InLayoutWidget)
{
	LayoutWidget = InLayoutWidget;
}

void ULetheGameUIFeature::DeinitializeFeature()
{
	LayoutWidget.Reset();
}

UWorld* ULetheGameUIFeature::GetWorld() const
{
	if (LayoutWidget.IsValid())
	{
		return LayoutWidget->GetWorld();
	}
	return Super::GetWorld();
}
