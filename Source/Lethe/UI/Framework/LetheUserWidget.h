// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LetheUserWidget.generated.h"

class ULetheWidgetController;

UCLASS(Abstract)
class LETHE_API ULetheUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetWidgetController(ULetheWidgetController* InWidgetController);

protected:
	/**
	 * WidgetController가 바인드되는 순간, 즉 SetWidgetController에서 호출하는 함수입니다.
	 * 주로 자신이 참조하고 있는 다른 위젯에 WidgetController를 할당하거나
	 * WidgetController의 델리게이트에 자신의 함수를 바인드하는 데에 사용합니다.
	 */
	UFUNCTION(BlueprintNativeEvent)
	void WidgetControllerSet();

public:
	/**
	 * 전역함수를 통해 그때그때 가져와 사용할 수도 있으나, 웬만하면 멤버변수로 참조하고 있는 편이 낫습니다.
	 * 명시적으로 의존성을 주입해 MVVM 패턴임을 확실하게 보여 가독성을 확보합니다.
	 */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ULetheWidgetController> WidgetController;
};
