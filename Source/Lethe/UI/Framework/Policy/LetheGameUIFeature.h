// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LetheGameUIFeature.generated.h"

class UCommonActivatableWidget;
class ULethePrimaryGameLayout;
struct FGameplayTag;

/**
 * GameUIPolicy에 꽂히는 UI 기능 단위입니다.
 * 특정 UI 기능의 Widget/Controller 생성과 Push 흐름을 담당합니다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class LETHE_API ULetheGameUIFeature : public UObject
{
	GENERATED_BODY()

public:
	virtual void InitializeFeature(ULethePrimaryGameLayout* InLayoutWidget);
	virtual void DeinitializeFeature();

protected:
	TWeakObjectPtr<ULethePrimaryGameLayout> LayoutWidget;
};
