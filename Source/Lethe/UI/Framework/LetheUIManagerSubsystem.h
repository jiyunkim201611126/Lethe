// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Lethe/Data/LevelData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UIPolicy/LetheGameUIPolicy.h"
#include "LetheUIManagerSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FUIPolicyTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ELevelType LevelType = ELevelType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<ULetheGameUIPolicy> UIPolicyClass;
};

USTRUCT(BlueprintType)
struct FUIFeatureTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag UIFeatureTag;

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<ULetheGameUIFeature> UIFeatureClass;
};

/**
 * 게임 시작 시 정해진 GameUIPolicy 객체를 생성, 관리하는 매니저 클래스입니다.
 */
UCLASS(Config = Game)
class LETHE_API ULetheUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool EnsureCreateRootLayout(ULocalPlayer* LocalPlayer) const;
	
	ULetheGameUIPolicy* GetCurrentUIPolicy();

	template <typename FeatureT>
	FeatureT* GetOrCreateUIFeature(const FGameplayTag& FeatureTag) const
	{
		static_assert(TIsDerivedFrom<FeatureT, ULetheGameUIFeature>::IsDerived, "FeatureT는 반드시 ULetheGameUIFeature를 상속받아야 합니다.");

		if (!EnsureCreateRootLayout(GetGameInstance()->GetFirstGamePlayer()))
		{
			return nullptr;
		}

		if (CurrentPolicy)
		{
			if (const auto* UIFeatureClass = UIFeatureClasses.Find(FeatureTag))
			{
				const TSubclassOf<FeatureT> LoadedUIFeatureClass = UIFeatureClass->LoadSynchronous();
				return CurrentPolicy->GetOrCreateUIFeature<FeatureT>(LoadedUIFeatureClass);
			}
		}
		return nullptr;
	}

private:
	void OnLevelChangeStarted();
	void OnLevelChangeFinished();
	
	ULetheGameUIPolicy* CreateUIPolicyByLevelType(const ELevelType LevelType);
	
	void SwitchToPolicy(ULetheGameUIPolicy* InPolicy);

protected:
	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> UIPolicyDataTable;
	
	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> UIFeatureDataTable;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULetheGameUIPolicy> CurrentPolicy;

	TMap<FGameplayTag, TSoftClassPtr<ULetheGameUIFeature>> UIFeatureClasses;

	FDelegateHandle OnLevelChangeStartedHandle;
	FDelegateHandle OnLevelChangeFinishedHandle;
};
