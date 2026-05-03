// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TriggerAction.generated.h"

USTRUCT(BlueprintType)
struct FTriggeredActionContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> InstigatorActor = nullptr;
};

/**
 * 어떤 일이 발생할지를 정의하는 클래스입니다.
 * 재활용 여지가 많은 경우 상속받아 구현합니다.
 * 예시) 레버를 작동시키면 잠겨있던 문이 열림(OpenDoorAction)과 동시에 몬스터가 스폰(SpawnMonsterAction)
 *
 * 너무 단순한 상호작용도 해당 클래스를 사용하게 될 경우 과설계가 되어 추적이 어려울 수 있습니다.
 */
UCLASS(Abstract, NotBlueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LETHE_API UTriggerAction : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	virtual void Action(UPARAM(ref)const FTriggeredActionContext& ActionContext) PURE_VIRTUAL(UTriggeredAction::Action, );
};
