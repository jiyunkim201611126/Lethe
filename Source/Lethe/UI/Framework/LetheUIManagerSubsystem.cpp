// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheUIManagerSubsystem.h"

#include "Lethe/Manager/World/LevelManagerSubsystem.h"
#include "Policy/LetheGameUIPolicy.h"

void ULetheUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<ULevelManagerSubsystem>();
	if (ULevelManagerSubsystem* LevelManagerSubsystem = GetGameInstance()->GetSubsystem<ULevelManagerSubsystem>())
	{
		OnLevelChangeStartedHandle = LevelManagerSubsystem->OnStartLevelChange.AddUObject(this, &ThisClass::OnLevelChangeStarted);
	}

	if (!CurrentPolicy && !DefaultUIPolicyClass.IsNull())
	{
		const TSubclassOf<ULetheGameUIPolicy> PolicyClass = DefaultUIPolicyClass.LoadSynchronous();
		SwitchToPolicy(NewObject<ULetheGameUIPolicy>(this, PolicyClass));
	}
}

void ULetheUIManagerSubsystem::Deinitialize()
{
	if (CurrentPolicy)
	{
		CurrentPolicy->DeinitializeFeatures();
	}
	SwitchToPolicy(nullptr);

	if (ULevelManagerSubsystem* LevelManagerSubsystem = GetGameInstance()->GetSubsystem<ULevelManagerSubsystem>())
	{
		LevelManagerSubsystem->OnStartLevelChange.Remove(OnLevelChangeStartedHandle);
	}

	Super::Deinitialize();
}

void ULetheUIManagerSubsystem::SwitchToPolicy(ULetheGameUIPolicy* InPolicy)
{
	if (CurrentPolicy != InPolicy)
	{
		CurrentPolicy = InPolicy;
	}
}

bool ULetheUIManagerSubsystem::EnsureCreateRootLayout(ULocalPlayer* LocalPlayer) const
{
	if (CurrentPolicy && CurrentPolicy->GetOrCreateRootLayout(LocalPlayer))
	{
		return true;
	}
	return false;
}

void ULetheUIManagerSubsystem::OnLevelChangeStarted() const
{
	if (CurrentPolicy)
	{
		CurrentPolicy->DeinitializeFeatures();
	}
}

ULetheGameUIPolicy* ULetheUIManagerSubsystem::GetCurrentUIPolicy()
{
	return CurrentPolicy;
}
