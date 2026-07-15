// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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
	FeatureT* GetOrCreateUIFeature(const TSubclassOf<FeatureT>& FeatureTClass) const
	{
		static_assert(TIsDerivedFrom<FeatureT, ULetheGameUIFeature>::IsDerived, "FeatureT는 반드시 ULetheGameUIFeature를 상속받아야 합니다.");

		if (!EnsureCreateRootLayout(GetGameInstance()->GetFirstGamePlayer()))
		{
			return nullptr;
		}

		if (CurrentPolicy)
		{
			return CurrentPolicy->GetOrCreateUIFeature<FeatureT>(FeatureTClass);
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

private:
	UPROPERTY(Transient)
	TObjectPtr<ULetheGameUIPolicy> CurrentPolicy;

	FDelegateHandle OnLevelChangeStartedHandle;
	FDelegateHandle OnLevelChangeFinishedHandle;
};
