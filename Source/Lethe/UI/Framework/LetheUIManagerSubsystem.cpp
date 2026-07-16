// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheUIManagerSubsystem.h"

#include "Lethe/Manager/World/LevelManagerSubsystem.h"

void ULetheUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<ULevelManagerSubsystem>();
	
	ULevelManagerSubsystem* LevelManagerSubsystem = GetGameInstance()->GetSubsystem<ULevelManagerSubsystem>();
	if (LevelManagerSubsystem)
	{
		OnLevelChangeStartedHandle = LevelManagerSubsystem->OnStartLevelChange.AddUObject(this, &ThisClass::OnLevelChangeStarted);
		OnLevelChangeFinishedHandle = LevelManagerSubsystem->OnFinishLevelChange.AddUObject(this, &ThisClass::OnLevelChangeFinished);

		const ELevelType CurrentLevelType = LevelManagerSubsystem->GetCurrentLevelType();
		if (ULetheGameUIPolicy* CreatedUIPolicy = CreateUIPolicyByLevelType(CurrentLevelType))
		{
			SwitchToPolicy(CreatedUIPolicy);
		}
	}

	const UDataTable* LoadedUIFeatureDataTable = UIFeatureDataTable.LoadSynchronous();
	if (!ensure(LoadedUIFeatureDataTable))
	{
		return;
	}

	TArray<FUIFeatureTable*> Rows;
	LoadedUIFeatureDataTable->GetAllRows(TEXT("UIFeature"), Rows);
	
	for (const FUIFeatureTable* UIFeatureTableRow : Rows)
	{
		if (UIFeatureTableRow)
		{
			UIFeatureClasses.Emplace(UIFeatureTableRow->UIFeatureTag, UIFeatureTableRow->UIFeatureClass);
		}
	}
}

void ULetheUIManagerSubsystem::Deinitialize()
{
	if (CurrentPolicy)
	{
		CurrentPolicy->Deinitialize();
	}
	SwitchToPolicy(nullptr);

	if (ULevelManagerSubsystem* LevelManagerSubsystem = GetGameInstance()->GetSubsystem<ULevelManagerSubsystem>())
	{
		LevelManagerSubsystem->OnStartLevelChange.Remove(OnLevelChangeStartedHandle);
		LevelManagerSubsystem->OnFinishLevelChange.Remove(OnLevelChangeFinishedHandle);
	}

	UIFeatureClasses.Empty();

	Super::Deinitialize();
}

void ULetheUIManagerSubsystem::OnLevelChangeStarted()
{
	if (CurrentPolicy)
	{
		CurrentPolicy->Deinitialize();
	}
	SwitchToPolicy(nullptr);
}

void ULetheUIManagerSubsystem::OnLevelChangeFinished()
{
	const ULevelManagerSubsystem* LevelManagerSubsystem = GetGameInstance()->GetSubsystem<ULevelManagerSubsystem>();
	if (LevelManagerSubsystem)
	{
		const ELevelType CurrentLevelType = LevelManagerSubsystem->GetCurrentLevelType();
		if (ULetheGameUIPolicy* CreatedUIPolicy = CreateUIPolicyByLevelType(CurrentLevelType))
		{
			SwitchToPolicy(CreatedUIPolicy);
		}
	}
}

ULetheGameUIPolicy* ULetheUIManagerSubsystem::CreateUIPolicyByLevelType(const ELevelType LevelType)
{
	const UDataTable* LoadedUIPolicyDataTable = UIPolicyDataTable.LoadSynchronous();
	if (!ensure(LoadedUIPolicyDataTable))
	{
		return nullptr;
	}

	TArray<FUIPolicyTableRow*> Rows;
	LoadedUIPolicyDataTable->GetAllRows(TEXT("UIPolicy"), Rows);
	
	for (const FUIPolicyTableRow* UIPolicyTableRow : Rows)
	{
		if (UIPolicyTableRow && UIPolicyTableRow->LevelType == LevelType)
		{
			const TSubclassOf<ULetheGameUIPolicy> LoadedUIPolicyClass = UIPolicyTableRow->UIPolicyClass.LoadSynchronous();
			if (!ensure(LoadedUIPolicyClass))
			{
				return nullptr;
			}
	
			return NewObject<ULetheGameUIPolicy>(this, LoadedUIPolicyClass);
		}
	}
	return nullptr;
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

ULetheGameUIPolicy* ULetheUIManagerSubsystem::GetCurrentUIPolicy()
{
	return CurrentPolicy;
}
