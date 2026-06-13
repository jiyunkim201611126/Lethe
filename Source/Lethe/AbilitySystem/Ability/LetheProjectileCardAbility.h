// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheCardAbility.h"
#include "Lethe/Actor/LetheProjectile.h"
#include "LetheProjectileCardAbility.generated.h"

/**
 * Projectile을 발사하는 Ability로, EffectApplyPolicy를 그대로 따릅니다.
 * 예를 들어 7개의 타일을 대상으로 사용하는 Ability로 디자인됐고, 해당 위치에서 3명의 적을 검출했다면 3명의 적에게 Projectile을 날립니다.
 * LetheCardAbility와 마찬가지로 위치에 따라 효과를 다르게 적용할 수 있습니다.
 * 
 * 따라서 NumOfProjectiles를 수정할 때 오버밸런스가 되지 않도록 주의해야 합니다.
 * 3명의 적에게 Projectile을 날리는 상황에 NumOfPorjectiles를 3으로 지정했다면 총 9개의 Projectile이 스폰되며, 대상마다 3배의 효과가 적용됩니다.
 */
UCLASS()
class LETHE_API ULetheProjectileCardAbility : public ULetheCardAbility
{
	GENERATED_BODY()
	
protected:
	virtual void ExecuteEffectAppliersByPolicy(const FEffectApplyPolicy& EffectApplyPolicy, AActor* TargetActor) override;
	
	/**
	 * Projectile을 생성 및 발사하는 함수입니다.
	 * @param InProjectileSpawnLocation Projectile이 생성될 위치입니다.
	 * @param EffectApplyPolicy 어떤 대상에게 어떤 EffectApplier를 적용할지 결정하는 정책 묶음으로, 여기선 EffectApplier를 가져오기 위해 사용합니다.
	 * @param TargetActor Projectile의 목표 대상입니다.
	 */
	void SpawnProjectiles(const FVector& InProjectileSpawnLocation, const FEffectApplyPolicy& EffectApplyPolicy, AActor* TargetActor) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<ALetheProjectile> ProjectileClass;

	/** 검출된 대상에게 발사할 Projectile 개수입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	int32 NumOfProjectiles = 1;
	
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float ProjectileSpread = 30.f;

	/** 양수 입력 시 해당 각도만큼 위로 꺾여 발사됩니다. 음수는 무시합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float PitchOverride = 30.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	FProjectileSpawnPayload ProjectileSpawnPayload;
};
