// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TriggerAction.h"
#include "Components/ActorComponent.h"
#include "TriggerActionComponent.generated.h"

/**
 * OwnerActor와 상호작용 시, 어떤 일이 발생할지를 조합해 정의하는 클래스입니다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LETHE_API UTriggerActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void Action();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced)
	TArray<TObjectPtr<UTriggerAction>> Actions;
};
