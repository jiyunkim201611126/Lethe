// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "LethePrimaryGameLayout.generated.h"

/**
 * 위젯 계층을 관리하는 위젯입니다.
 * 블루프린트의 Construct에서 계층(CommonActivatableWidgetStack)마다 Tag로 매핑되어 등록됩니다.
 * 해당 Tag와 함께 CommonActivatableWidget 클래스가 들어오면 생성하고 계층에 등록시켜주며, 생성된 위젯을 반환합니다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class LETHE_API ULethePrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	static ULethePrimaryGameLayout* GetPrimaryGameLayout(const APlayerController* PlayerController);
	static ULethePrimaryGameLayout* GetPrimaryGameLayout(ULocalPlayer* LocalPlayer);

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
