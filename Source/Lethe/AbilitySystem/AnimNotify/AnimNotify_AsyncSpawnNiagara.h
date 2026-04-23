// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_AsyncSpawnNiagara.generated.h"

/**
 * 애니메이션 재생 주체 Actor의 위치에서 나이아가라를 비동기 로드 재생하고 싶을 때 사용하는 클래스입니다.
 */
UCLASS()
class LETHE_API UAnimNotify_AsyncSpawnNiagara : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_AsyncSpawnNiagara();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayCue")
	FGameplayTag NiagaraTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayCue")
	bool bUseActorRotation = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayCue")
	FVector SpawnScale = FVector::OneVector;
};
