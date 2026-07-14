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
UCLASS(Abstract, Within = LetheUIManagerSubsystem)
class LETHE_API ULetheGameUIPolicy : public UObject
{
	GENERATED_BODY()

public:
	static ULetheGameUIPolicy* GetGameUIPolicy(const UObject* WorldContextObject);

	ULetheUIManagerSubsystem* GetOwningUIManager() const;
	
	ULethePrimaryGameLayout* GetOrCreateRootLayout(ULocalPlayer* LocalPlayer);
	ULethePrimaryGameLayout* GetRootLayout() const;

	template <typename FeatureT>
	FeatureT* GetOrCreateUIFeature(const TSubclassOf<FeatureT>& FeatureTClass)
	{
		static_assert(TIsDerivedFrom<FeatureT, ULetheGameUIFeature>::IsDerived, "FeatureT는 반드시 ULetheGameUIFeature를 상속받아야 합니다.");

		for (ULetheGameUIFeature* UIFeature : UIFeatures)
		{
			if (FeatureT* TypedFeature = Cast<FeatureT>(UIFeature))
			{
				return TypedFeature;
			}
		}

		if (FeatureT* CreatedFeature = NewObject<FeatureT>(this, FeatureTClass))
		{
			CreatedFeature->InitializeFeature(GetRootLayout());
			UIFeatures.Add(CreatedFeature);
			return CreatedFeature;
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

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULetheGameUIFeature>> UIFeatures;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSoftClassPtr<ULetheGameUIFeature>> StartUIFeatureClasses;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULethePrimaryGameLayout> RootLayout;
};
