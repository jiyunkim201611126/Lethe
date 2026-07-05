// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectDelivery.h"
#include "Lethe/Actor/LetheProjectile.h"
#include "EffectDelivery_Projectile.generated.h"

/**
 * Projectile을 스폰, 해당 투사체에게 EffectSpec을 넘겨주는 방식의 Delivery입니다.
 * 예를 들어 7개의 타일을 대상으로 사용하는 Ability로 디자인됐고, 해당 위치에서 3명의 적을 검출했다면 3명의 적에게 Projectile을 날립니다.
 * 
 * 따라서 NumOfProjectiles를 수정할 때 오버밸런스가 되지 않도록 주의해야 합니다.
 * 3명의 적에게 Projectile을 날리는 상황에 NumOfProjectiles를 3으로 지정했다면 총 9개의 Projectile이 스폰되며, 대상마다 3배의 효과가 적용됩니다.
 */
USTRUCT(BlueprintType)
struct LETHE_API FEffectDelivery_Projectile : public FGameplayEffectDelivery
{
	GENERATED_BODY()
	
public:
	virtual void StartDelivery(const FEffectDeliveryContext& Context) const override;

private:
	/**
	 * @param InProjectileSpawnLocation Projectile이 생성될 위치입니다.
	 * @param SourceActor Projectile을 발사한 캐릭터입니다.
	 * @param TargetActor Projectile의 목표 대상입니다.
	 * @param Context 전달 시작을 위해 필요한 변수들이 담긴 Context입니다.
	 */
	void SpawnProjectile(const FVector& InProjectileSpawnLocation, AActor* SourceActor, AActor* TargetActor, const FEffectDeliveryContext& Context) const;

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
	float PitchOverride = 0.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	FProjectileSpawnPayload ProjectileSpawnPayload;
};
