// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "LethePrimaryGameLayout.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class LETHE_API ULethePrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	static ULethePrimaryGameLayout* GetPrimaryGameLayout(APlayerController* PlayerController);

	template <typename ActivatableWidgetT = UCommonActivatableWidget>
	ActivatableWidgetT* PushWidgetToLayerStack(const FGameplayTag& LayerTag, UClass* WidgetClass)
	{
		if (UCommonActivatableWidgetContainerBase* Layer = GetLayerWidget(LayerTag))
		{
			return Layer->AddWidget<ActivatableWidgetT>(WidgetClass);
		}
		return nullptr;
	}

	template <typename ActivatableWidgetT = UCommonActivatableWidget>
	ActivatableWidgetT* PushWidgetToLayerStack(const FGameplayTag& LayerTag, UClass* WidgetClass, TFunctionRef<void(ActivatableWidgetT&)> InstanceInitFunc)
	{
		if (UCommonActivatableWidgetContainerBase* Layer = GetLayerWidget(LayerTag))
		{
			return Layer->AddWidget<ActivatableWidgetT>(WidgetClass, InstanceInitFunc);
		}
		return nullptr;
	}

	void FindAndRemoveWidgetFromLayer(UCommonActivatableWidget* ActivatableWidget);

	UFUNCTION(BlueprintCallable, Category = "Lethe|UI")
	UCommonActivatableWidgetContainerBase* GetLayerWidget(FGameplayTag LayerTag) const;

protected:
	UFUNCTION(BlueprintCallable, Category = "Lethe|UI|Layer")
	void RegisterLayer(UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag LayerTag, UCommonActivatableWidgetContainerBase* LayerWidget);

	void OnWidgetStackTransitioning(UCommonActivatableWidgetContainerBase* Widget, bool bIsTransitioning);

private:
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>> Layers;
};
