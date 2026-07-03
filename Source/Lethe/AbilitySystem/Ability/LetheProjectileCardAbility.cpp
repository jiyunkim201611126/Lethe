// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheProjectileCardAbility.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemLibrary.h"
#include "Lethe/Actor/LetheProjectile.h"

void ULetheProjectileCardAbility::ExecuteEffectAppliersByPolicy(const FEffectApplyPolicy& EffectApplyPolicy, AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	const AActor* Instigator = CurrentActorInfo->AvatarActor.Get();
	if (!Instigator)
	{
		return;
	}
	
	if (TargetActor == Instigator)
	{
		// 자기 자신에게 투사체를 발사하면 어떻게 처리할 건지 여기서 결정합니다.
		return;
	}

	if (ensureMsgf(ProjectileClass, TEXT("Projectile 클래스가 할당되지 않았습니다.")))
	{
		SpawnProjectiles(Instigator->GetActorLocation(), EffectApplyPolicy, TargetActor);
	}
}

void ULetheProjectileCardAbility::SpawnProjectiles(const FVector& InProjectileSpawnLocation, const FEffectApplyPolicy& EffectApplyPolicy, AActor* TargetActor) const
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
		APawn* Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
		ALetheProjectile* Projectile = GetWorld()->SpawnActorDeferred<ALetheProjectile>(ProjectileClass, SpawnTransform, Instigator, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Projectile)
		{
			continue;
		}

		// Payload를 원본 그대로 사용하지 않고 값복사해 따로 생성합니다.
		FProjectileSpawnPayload Payload = ProjectileSpawnPayload;

		Payload.Instigator = Instigator;
		Payload.TargetActor = TargetActor;

		// 이번 발사에 적용할 EffectAppliers를 가져옵니다.
		TArray<const FGameplayEffectApplier*> OutEffectAppliers;
		GetEffectAppliersByPolicy(EffectApplyPolicy, OutEffectAppliers);

		UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
		if (!SourceASC)
		{
			Projectile->Destroy();
			continue;
		}

		// 가져온 EffectAppliers를 순회하며 Spec을 생성, Payload에 할당합니다.
		for (const FGameplayEffectApplier* EffectApplier : OutEffectAppliers)
		{
			FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
			ContextHandle.SetAbility(this);
			TArray<FGameplayEffectSpecHandle> SpecHandles;
			EffectApplier->TryPrepareSpecHandles(SourceASC, ContextHandle, SpecHandles);
			Payload.SpecHandles.Append(SpecHandles);
		}

		Projectile->SetPayload(Payload);

		Projectile->FinishSpawning(SpawnTransform);
	}
}
