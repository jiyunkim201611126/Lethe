// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "LetheProjectile.generated.h"

class UGameplayAbility;
class UProjectileMovementComponent;
class USphereComponent;

USTRUCT()
struct FProjectileSpawnPayload
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	float ProjectileSpeed = 500.f;
	
	/**
	 * PitchOverride를 사용하거나, 부채꼴 모양으로 펼쳐져 발사되는 Projectile들의 경우, 첫 발사각이 타겟을 향하지 않습니다.
	 * 이 때 사용되는 궤도 변경 속도입니다.
	 */
	UPROPERTY(EditDefaultsOnly)
	float HomingAcceleration = 1000.f;

	/** 여러 Projectile을 한 번에 발사하는 Ability는 매우 시끄러울 수 있으므로 이 수치를 통해 사운드 크기를 조절합니다. */
	UPROPERTY(EditDefaultsOnly)
	float VolumeMultiplier = 1.f;

	TWeakObjectPtr<APawn> Instigator;
	TWeakObjectPtr<AActor> TargetActor;
	
	TArray<FGameplayEffectSpecHandle> SpecHandles;
};

UCLASS()
class LETHE_API ALetheProjectile : public AActor
{
	GENERATED_BODY()

public:
	ALetheProjectile();

	void SetPayload(const FProjectileSpawnPayload& InPayload);

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface

private:
	UFUNCTION()
	void OnHitComponent(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	void PlayHitFXs() const;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> SphereComponent;

	UPROPERTY()
	TObjectPtr<UAudioComponent> LoopingSoundComponent;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag LoopingSoundTag;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag ImpactSoundTag;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag ImpactEffectTag;

private:
	FProjectileSpawnPayload Payload;
};
