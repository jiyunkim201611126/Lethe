// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameUIFeature.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "Lethe/UI/Framework/LethePrimaryGameLayout.h"

void ULetheGameUIFeature::Initialize(ULethePrimaryGameLayout* InLayoutWidget)
{
	LayoutWidget = InLayoutWidget;
	
	OnInitialized();
}

void ULetheGameUIFeature::Deinitialize()
{
	OnDeinitialized();
	
	LayoutWidget.Reset();
}

void ULetheGameUIFeature::OnInitialized()
{
}

void ULetheGameUIFeature::OnDeinitialized()
{
}

UWorld* ULetheGameUIFeature::GetWorld() const
{
	if (LayoutWidget.IsValid())
	{
		return LayoutWidget->GetWorld();
	}
	return Super::GetWorld();
}
