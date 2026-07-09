// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheUIManagerSubsystem.h"

#include "Policy/LetheGameUIPolicy.h"

void ULetheUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!CurrentPolicy && !DefaultUIPolicyClass.IsNull())
	{
		const TSubclassOf<ULetheGameUIPolicy> PolicyClass = DefaultUIPolicyClass.LoadSynchronous();
		SwitchToPolicy(NewObject<ULetheGameUIPolicy>(this, PolicyClass));
	}
}

void ULetheUIManagerSubsystem::Deinitialize()
{
	SwitchToPolicy(nullptr);

	Super::Deinitialize();
}

ULetheGameUIPolicy* ULetheUIManagerSubsystem::GetCurrentUIPolicy()
{
	return CurrentPolicy;
}

void ULetheUIManagerSubsystem::SwitchToPolicy(ULetheGameUIPolicy* InPolicy)
{
	if (CurrentPolicy != InPolicy)
	{
		CurrentPolicy = InPolicy;
	}
}

void ULetheUIManagerSubsystem::EnsureCreateRootLayout(APlayerController* PlayerController) const
{
	if (CurrentPolicy && !CurrentPolicy->GetRootLayout())
	{
		CurrentPolicy->GetOrCreateRootLayout(PlayerController);
	}
}
