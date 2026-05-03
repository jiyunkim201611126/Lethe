// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "AnimNotify_PlaySoundWithActorHidden.generated.h"

/**
 * 액터 Hidden 상태에 따라 사운드 재생 여부를 결정하는 AnimNotify입니다.
 */
UCLASS()
class LETHE_API UAnimNotify_PlaySoundWithActorHidden : public UAnimNotify_PlaySound
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
