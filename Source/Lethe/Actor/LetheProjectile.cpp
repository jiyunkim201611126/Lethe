// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/Lethe.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/FX/FXManagerSubsystem.h"

ALetheProjectile::ALetheProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SetRootComponent(SphereComponent);
	SphereComponent->SetCollisionObjectType(ECC_Projectile);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	SphereComponent->OnComponentHit.AddDynamic(this, &ThisClass::OnHitComponent);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->SetUpdatedComponent(SphereComponent);
	ProjectileMovementComponent->InitialSpeed = 500.f;
	ProjectileMovementComponent->MaxSpeed = 500.f;
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
	ProjectileMovementComponent->bIsHomingProjectile = true;
	ProjectileMovementComponent->bSweepCollision = true;
}

void ALetheProjectile::SetPayload(const FProjectileSpawnPayload& InPayload)
{
	Payload = InPayload;

	SphereComponent->IgnoreActorWhenMoving(Payload.Instigator.Get(), true);

	ProjectileMovementComponent->HomingTargetComponent = Payload.TargetActor.Get()->GetRootComponent();
	ProjectileMovementComponent->HomingAccelerationMagnitude = Payload.HomingAcceleration;
	ProjectileMovementComponent->InitialSpeed = Payload.ProjectileSpeed;
	ProjectileMovementComponent->MaxSpeed = Payload.ProjectileSpeed;

	// 추적 도중 적이 사망했을 때를 대비한 로직을 위해 Tick을 활성화합니다.
	SetActorTickEnabled(true);
	SetActorTickInterval(0.2f);
}

void ALetheProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (!LoopingSoundTag.IsValid())
	{
		return;
	}

	if (UFXManagerSubsystem* FXManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFXManagerSubsystem>())
	{
		TWeakObjectPtr<ALetheProjectile> WeakThis = MakeWeakObjectPtr(this);
		FXManagerSubsystem->AsyncGetSound(LoopingSoundTag, [WeakThis](USoundBase* LoopingSound)
		{
			if (WeakThis.IsValid() && LoopingSound)
			{
				WeakThis->LoopingSoundComponent = UGameplayStatics::SpawnSoundAttached(LoopingSound, WeakThis->GetRootComponent());
				if (WeakThis->LoopingSoundComponent)
				{
					WeakThis->LoopingSoundComponent->SetVolumeMultiplier(WeakThis->Payload.VolumeMultiplier);
				}
			}
		});
	}
}

void ALetheProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ProjectileMovementComponent->HomingTargetComponent.IsValid())
	{
		// 추적 중인 타겟이 사망한 경우 일반 Projectile로 변경합니다.
		ProjectileMovementComponent->bIsHomingProjectile = false;
		ProjectileMovementComponent->HomingTargetComponent = nullptr;

		// 0.5초 후 파괴되도록 설정합니다.
		SetLifeSpan(0.5f);
	}
}

void ALetheProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PlayHitFXs();
	
	SphereComponent->OnComponentHit.RemoveDynamic(this, &ThisClass::OnHitComponent);

	if (LoopingSoundComponent)
	{
		LoopingSoundComponent->Stop();
		LoopingSoundComponent->DestroyComponent();
	}
	
	Super::EndPlay(EndPlayReason);
}

void ALetheProjectile::OnHitComponent(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(OtherActor))
	{
		if (!CombatInterface->IsDead())
		{
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
			{
				for (const FGameplayEffectSpecHandle& SpecHandle : Payload.SpecHandles)
				{
					if (SpecHandle.IsValid())
					{
						TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
					}
				}
			}
		}
	}
	
	Destroy();
}

void ALetheProjectile::PlayHitFXs() const
{
	if (UFXManagerSubsystem* FXManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFXManagerSubsystem>())
	{
		if (ImpactSoundTag.IsValid())
		{
			FXManagerSubsystem->AsyncPlaySoundAtLocation(ImpactSoundTag, GetActorLocation(), FRotator::ZeroRotator, Payload.VolumeMultiplier);
		}
		if (ImpactEffectTag.IsValid())
		{
			FXManagerSubsystem->AsyncSpawnNiagaraAtLocation(ImpactEffectTag, GetActorLocation());
		}
	}
}

