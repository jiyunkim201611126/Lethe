// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectDelivery_Projectile.h"

#include "AbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemLibrary.h"

void FEffectDelivery_Projectile::StartDelivery(const FEffectDeliveryContext& Context) const
{
	if (!Context.IsValid())
	{
		return;
	}

	AActor* SourceActor = Context.SourceASC->GetOwnerActor();
	AActor* TargetActor = Context.TargetASC->GetOwnerActor();
	if (SourceActor && TargetActor)
	{
		const FVector ProjectileSpawnLocation = SourceActor->GetActorLocation(); 
		SpawnProjectile(ProjectileSpawnLocation, SourceActor, TargetActor, Context);
	}
}

void FEffectDelivery_Projectile::SpawnProjectile(const FVector& InProjectileSpawnLocation, AActor* SourceActor, AActor* TargetActor, const FEffectDeliveryContext& Context) const
{
	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FVector Forward = TargetLocation - InProjectileSpawnLocation;

	// 발사할 개수와 지정된 각도에 따라 부채꼴 모양으로 펼친 Rotators를 계산하고, 상하 각도를 적용합니다.
	TArray<FRotator> Rotations = ULetheAbilitySystemLibrary::EvenlySpacedRotators(Forward, FVector::UpVector, ProjectileSpread, NumOfProjectiles);
	if (PitchOverride > 0.f)
	{
		for (auto& Rotation : Rotations)
		{
			Rotation.Pitch += PitchOverride;
		}
	}

	for (const auto& Rotation : Rotations)
	{
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(InProjectileSpawnLocation);
		SpawnTransform.SetRotation(Rotation.Quaternion());
		APawn* Instigator = Cast<APawn>(SourceActor);
		ALetheProjectile* Projectile = TargetActor->GetWorld()->SpawnActorDeferred<ALetheProjectile>(ProjectileClass, SpawnTransform, Instigator, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Projectile)
		{
			continue;
		}

		// Payload를 원본 그대로 사용하지 않고 값복사해 따로 생성합니다.
		FProjectileSpawnPayload Payload = ProjectileSpawnPayload;

		Payload.Instigator = Instigator;
		Payload.TargetActor = TargetActor;
		Payload.SpecHandles = Context.EffectSpecHandles;

		FGameplayEffectContextHandle ContextHandle = Context.SourceASC->MakeEffectContext();
		ContextHandle.SetAbility(Context.OwnerAbility.Get());

		Projectile->SetPayload(Payload);

		Projectile->FinishSpawning(SpawnTransform);
	}
}
