// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheProjectileCardAbility.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemLibrary.h"
#include "Lethe/Actor/LetheProjectile.h"
#include "Lethe/Manager/LetheGameplayTags.h"

void ULetheProjectileCardAbility::OnEventReceived(FGameplayEventData InPayload)
{
	for (const FEffectApplyPolicy& EffectApplyPolicy : EffectApplyPolicies)
	{
		if (InPayload.EventTag.MatchesTagExact(EffectApplyPolicy.MontageEventTag))
		{
			// 수신한 이벤트 태그와 EffectApplyPolicy의 이벤트 태그가 일치하는 경우 들어오는 분기입니다.
			TArray<AActor*> TargetActors;
			TargetActors.Reserve(CachedTargetActors.Num());
			for (const auto& CachedTargetActor : CachedTargetActors)
			{
				if (CachedTargetActor.IsValid())
				{
					TargetActors.Add(CachedTargetActor.Get());
				}
			}

			TArray<AActor*> OutTargetActors;
			GetTargetActorsByPolicy(EffectApplyPolicy, TargetActors, OutTargetActors);

			if (!OutTargetActors.IsEmpty())
			{
				for (AActor* TargetActor : OutTargetActors)
				{
					SpawnProjectiles(CurrentActorInfo->AvatarActor->GetActorLocation(), TargetActor);
				}
			}
			return;
		}
	}
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	if (InPayload.EventTag.MatchesTagExact(LetheGameplayTags.Event_Montage_EndAbility))
	{
		ResetCachedValues();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
	}
}

TArray<ALetheProjectile*> ULetheProjectileCardAbility::SpawnProjectiles(const FVector& InProjectileSpawnLocation, AActor* TargetActor)
{
	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FVector Forward = TargetLocation - InProjectileSpawnLocation;

	TArray<FRotator> Rotations = ULetheAbilitySystemLibrary::EvenlySpacedRotators(Forward, FVector::UpVector, ProjectileSpread, NumOfProjectiles);
	if (PitchOverride > 0.f)
	{
		for (auto& Rotation : Rotations)
		{
			Rotation.Pitch += PitchOverride;
		}
	}

	TArray<ALetheProjectile*> Projectiles;
	Projectiles.Reserve(NumOfProjectiles);
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

		ProjectileSpawnPayload.Instigator = Instigator;
		ProjectileSpawnPayload.TargetActor = TargetActor;
		for (const UGameplayEffectApplier* EffectApplier : EffectAppliers)
		{
			FGameplayEffectContextHandle ContextHandle;
			TArray<FGameplayEffectSpecHandle> SpecHandles;
			EffectApplier->TryPrepareSpecHandles(GetAbilitySystemComponentFromActorInfo(), ContextHandle, SpecHandles);
			ProjectileSpawnPayload.SpecHandles.Append(SpecHandles);
		}

		Projectiles.Add(Projectile);
		Projectile->SetPayload(ProjectileSpawnPayload);

		Projectile->FinishSpawning(SpawnTransform);
	}

	return Projectiles;
}
