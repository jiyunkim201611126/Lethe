// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HighlightInterface.generated.h"

UINTERFACE()
class UHighlightInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 하이라이트용 OutlineColor는 상속받은 클래스가 직접 선언해 갖고 있거나, Lethe.h에 정의한 값을 사용합니다.
 */
class LETHE_API IHighlightInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void HighlightActorTransparentByMouse();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void HighlightActorByMouse();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void UnhighlightActorByMouse();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void HighlightActorByAbility(const int32 InOutlineColor);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void UnhighlightActorByAbility();
};
