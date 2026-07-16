// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LetheGameUIFeature.generated.h"

class UCommonActivatableWidget;
class ULethePrimaryGameLayout;

/**
 * GameUIPolicy에 꽂히는, 특정 UI를 구현하기 위한 기능 단위입니다.
 * 자식 클래스들이 아래 기능들 중 몇 가지를 선택해 수행합니다.
 * 전부 수행하는 경우도 있고, 한 가지만 수행하는 경우도 있습니다.
 *
 * Widget 생성 및 Layout에 푸쉬
 * WidgetController 생성
 * Widget과 WidgetController, Model(캐릭터 등)을 바인드
 */
UCLASS(Abstract)
class LETHE_API ULetheGameUIFeature : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(ULethePrimaryGameLayout* InLayoutWidget);
	void Deinitialize();

	virtual void OnInitialized();
	virtual void OnDeinitialized();

	virtual UWorld* GetWorld() const override;

protected:
	TWeakObjectPtr<ULethePrimaryGameLayout> LayoutWidget;
};
