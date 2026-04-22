// Copyright JETBLU, Inc. All Rights Reserved.

#include "AnimNotify_LetheGameplayCue.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemLibrary.h"

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

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	if (!ASC)
	{
		return;
	}

	FCueDataContext RuntimeCueDataContext = CueDataContext;
	if (RuntimeCueDataContext.Locations.IsEmpty())
	{
		RuntimeCueDataContext.Locations.Emplace(OwnerActor->GetActorLocation());
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

	ASC->ExecuteGameplayCue(GameplayCueTag, CueParameters);
}
