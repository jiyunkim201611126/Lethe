// Copyright JETBLU, Inc. All Rights Reserved.

#include "AnimNotify_PlaySoundWithActorHidden.h"

FString UAnimNotify_PlaySoundWithActorHidden::GetNotifyName_Implementation() const
{
	if (Sound)
	{
		FString SoundName = Sound->GetName();
		SoundName.Append(TEXT("_WithActorHidden"));
		return SoundName;
	}
	else
	{
		return Super::GetNotifyName_Implementation();
	}
}

void UAnimNotify_PlaySoundWithActorHidden::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp)
	{
		if (const AActor* Owner = MeshComp->GetOwner())
		{
			if (!Owner->IsHidden())
			{
				Super::Notify(MeshComp, Animation, EventReference);
			}
		}
	}
}
