// Copyright JETBLU, Inc. All Rights Reserved.

#include "AnimNotify_LetheGameplayCue.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemLibrary.h"
#include "GameplayCueManager.h"

UAnimNotify_LetheGameplayCue::UAnimNotify_LetheGameplayCue()
{
	NotifyColor = FColor(40, 180, 255);
}

void UAnimNotify_LetheGameplayCue::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !GameplayCueTag.IsValid())
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	if (!ASC)
	{
		return;
	}

	FCueDataContext RuntimeCueDataContext = CueDataContext;
	if (RuntimeCueDataContext.Locations.IsEmpty())
	{
		RuntimeCueDataContext.Locations.Add(OwnerActor->GetActorLocation());
	}
	else
	{
		RuntimeCueDataContext.Locations[0] = OwnerActor->GetActorLocation();
	}

	FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
	ULetheAbilitySystemLibrary::SetCueContextToEffectContext(RuntimeCueDataContext, EffectContextHandle);

	FGameplayCueParameters CueParameters;
	CueParameters.EffectContext = EffectContextHandle;
	CueParameters.Instigator = OwnerActor;
	
	UGameplayCueManager::ExecuteGameplayCue_NonReplicated(OwnerActor, GameplayCueTag, CueParameters);
}
