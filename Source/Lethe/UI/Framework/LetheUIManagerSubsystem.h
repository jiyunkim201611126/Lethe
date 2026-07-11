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

	bool EnsureCreateRootLayout(ULocalPlayer* LocalPlayer) const;
	
	ULetheGameUIPolicy* GetCurrentUIPolicy();

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

protected:
	void SwitchToPolicy(ULetheGameUIPolicy* InPolicy);

private:
	void OnLevelChangeStarted() const;

protected:
	UPROPERTY(Config, EditAnywhere)
	TSoftClassPtr<ULetheGameUIPolicy> DefaultUIPolicyClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULetheGameUIPolicy> CurrentPolicy;

	FDelegateHandle OnLevelChangeStartedHandle;
};
