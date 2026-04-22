// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Lethe/LetheAbilityTypes.h"
#include "AnimNotify_LetheGameplayCue.generated.h"

UCLASS()
class LETHE_API UAnimNotify_LetheGameplayCue : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_LetheGameplayCue();
	
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayCue")
	FGameplayTag GameplayCueTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayCue")
	FCueDataContext CueDataContext;
};
