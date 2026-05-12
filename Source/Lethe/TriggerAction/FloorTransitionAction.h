// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TriggerAction.h"
#include "FloorTransitionAction.generated.h"

UCLASS()
class LETHE_API UFloorTransitionAction : public UTriggerAction
{
	GENERATED_BODY()

public:
	/** 임시로 Collision을 통해 구현되어 있으므로, 수정 시 BP_PlayerCharacterBase의 CapsuleCollision Generate Overlap Event도 꺼야 함 */
	virtual void Action(const FTriggeredActionContext& ActionContext) override;
};
