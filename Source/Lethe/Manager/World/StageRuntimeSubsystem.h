// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StageRuntimeSubsystem.generated.h"

/**
 * FloorTransition을 위해 액터 Destroy, 타일을 재생성, 캐싱된 데이터를 지우는 등의 명령을 여러 곳에 뿌리는 Subsystem입니다.
 * FloorTransitionAction이 이런 역할을 직접 담당하기보다는 매니저 클래스를 두는 편이 재활용 가능성이 높아 이처럼 구현합니다.
 */
UCLASS()
class LETHE_API UStageRuntimeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterFloorActor(AActor* InActor);
	void StartFloorTransition();

private:
	TArray<TWeakObjectPtr<AActor>> FloorActors;
};
