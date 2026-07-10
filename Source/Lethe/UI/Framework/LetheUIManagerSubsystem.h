// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPtr.h"
#include "Policy/LetheGameUIPolicy.h"
#include "LetheUIManagerSubsystem.generated.h"

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

	ULetheGameUIPolicy* GetCurrentUIPolicy();

	/** RootLayout의 생성이 확실할 때 사용하는 함수입니다. */
	template <typename FeatureT>
	FeatureT* FindUIFeature() const
	{
		static_assert(TIsDerivedFrom<FeatureT, ULetheGameUIFeature>::IsDerived, "FeatureT는 반드시 ULetheGameUIFeature를 상속받아야 합니다.");

		if (CurrentPolicy)
		{
			return CurrentPolicy->FindUIFeature<FeatureT>();
		}
		return nullptr;
	}

	/** RootLayout의 생성이 불확실한 상황(게임 시작 직후 등)에 Feature가 필요한 경우 사용합니다. */
	template <typename FeatureT>
	FeatureT* FindUIFeatureWithEnsureRootLayout(APlayerController* PlayerController) const
	{
		static_assert(TIsDerivedFrom<FeatureT, ULetheGameUIFeature>::IsDerived, "FeatureT는 반드시 ULetheGameUIFeature를 상속받아야 합니다.");
		
		if (EnsureCreateRootLayout(PlayerController))
		{
			if (CurrentPolicy)
			{
				return CurrentPolicy->FindUIFeature<FeatureT>();
			}
		}
		return nullptr;
	}

protected:
	void SwitchToPolicy(ULetheGameUIPolicy* InPolicy);

private:
	bool EnsureCreateRootLayout(APlayerController* PlayerController) const;

protected:
	UPROPERTY(Config, EditAnywhere)
	TSoftClassPtr<ULetheGameUIPolicy> DefaultUIPolicyClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULetheGameUIPolicy> CurrentPolicy;
};
