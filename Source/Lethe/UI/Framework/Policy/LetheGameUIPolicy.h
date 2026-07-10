// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameUIFeature.h"
#include "Engine/World.h"
#include "LetheGameUIPolicy.generated.h"

class ULetheGameUIFeature;
class ULethePrimaryGameLayout;
class ULetheUIManagerSubsystem;

/**
 * Root Layout으로 어떤 위젯을 사용할지 결정하는 UI 정책용 클래스입니다.
 * RootLayout과 Layer 접근을 제공합니다.
 */
UCLASS(Blueprintable, Within = LetheUIManagerSubsystem)
class LETHE_API ULetheGameUIPolicy : public UObject
{
	GENERATED_BODY()

public:
	static ULetheGameUIPolicy* GetGameUIPolicy(const UObject* WorldContextObject);

	ULetheUIManagerSubsystem* GetOwningUIManager() const;
	
	ULethePrimaryGameLayout* GetOrCreateRootLayout(ULocalPlayer* LocalPlayer);
	ULethePrimaryGameLayout* GetRootLayout() const;

	template <typename FeatureT>
	FeatureT* FindUIFeature() const
	{
		static_assert(TIsDerivedFrom<FeatureT, ULetheGameUIFeature>::IsDerived, "FeatureT는 반드시 ULetheGameUIFeature를 상속받아야 합니다.");

		for (ULetheGameUIFeature* UIFeature : UIFeatures)
		{
			if (FeatureT* TypedFeature = Cast<FeatureT>(UIFeature))
			{
				return TypedFeature;
			}
		}
		return nullptr;
	}

	void DeinitializeFeatures();

	virtual UWorld* GetWorld() const override;

protected:
	void CreateLayoutWidget(ULocalPlayer* LocalPlayer);
	
	void AddLayoutToViewport(ULocalPlayer* LocalPlayer, ULethePrimaryGameLayout* Layout);

	virtual void OnRootLayoutAddedToViewport(ULocalPlayer* LocalPlayer, ULethePrimaryGameLayout* Layout);

protected:
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<ULethePrimaryGameLayout> RootLayoutWidgetClass;

	UPROPERTY(EditDefaultsOnly, Instanced)
	TArray<TObjectPtr<ULetheGameUIFeature>> UIFeatures;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULethePrimaryGameLayout> RootLayout;

	uint8 bFeaturesInitialized : 1 = false;
};
