// Copyright JETBLU, Inc. All Rights Reserved.

#include "AnimNotify_AsyncSpawnNiagara.h"

#include "Lethe/Manager/FXManagerSubsystem.h"

UAnimNotify_AsyncSpawnNiagara::UAnimNotify_AsyncSpawnNiagara()
{
	NotifyColor = FColor(180, 40, 255);
}

void UAnimNotify_AsyncSpawnNiagara::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !NiagaraTag.IsValid())
	{
		return;
	}

	const AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}
	
	UFXManagerSubsystem* FXManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFXManagerSubsystem>();
	if (!FXManagerSubsystem)
	{
		return;
	}

	const FRotator SpawnRotation = bUseActorRotation ? OwnerActor->GetActorRotation() : FRotator::ZeroRotator;
	FXManagerSubsystem->AsyncSpawnNiagaraAtLocation(NiagaraTag, OwnerActor->GetActorLocation(), SpawnRotation, SpawnScale);
}
