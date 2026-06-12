// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheCardAbility.h"
#include "Lethe/Actor/LetheProjectile.h"
#include "LetheProjectileCardAbility.generated.h"

UCLASS()
class LETHE_API ULetheProjectileCardAbility : public ULetheCardAbility
{
	GENERATED_BODY()
	
protected:
	virtual void OnEventReceived(FGameplayEventData InPayload) override;
	
	/**
	 * Projectile을 생성 및 발사하는 함수입니다.
	 * @param InProjectileSpawnLocation Projectile이 생성될 위치입니다.
	 * @param TargetActor Projectile의 목표 대상입니다.
	 * @return 생성된 Projectiles 배열입니다.
	 */
	TArray<ALetheProjectile*> SpawnProjectiles(UPARAM(ref) const FVector& InProjectileSpawnLocation, AActor* TargetActor);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<ALetheProjectile> ProjectileClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	int32 NumOfProjectiles = 1.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float ProjectileSpread = 30.f;

	/** 양수 입력 시 해당 각도만큼 위로 꺾여 발사됩니다. 음수는 무시합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float PitchOverride = 30.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	FProjectileSpawnPayload ProjectileSpawnPayload;
};
