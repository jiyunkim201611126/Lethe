// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "LetheActivatableWidget.generated.h"

class ULetheWidgetController;

/**
 * 활성화되면 키보드나 게임패드 등의 입력을 직접 받을 수 있는 위젯 클래스입니다.
 */
UCLASS(Abstract)
class LETHE_API ULetheActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetWidgetController(ULetheWidgetController* InWidgetController);

protected:
	UFUNCTION(BlueprintNativeEvent)
	void WidgetControllerSet();

public:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ULetheWidgetController> WidgetController;
};
