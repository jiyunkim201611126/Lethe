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
	PrimaryActorTick.bCanEverTick = true;
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	SetRootComponent(SphereComponent);
	SphereComponent->SetCollisionObjectType(ECC_Projectile);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnComponentBeginOverlap);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
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

	if (!Payload.TargetActor.IsValid())
	{
		// 추적 중인 타겟이 사망 등의 이유로 유효하지 않게 되면 추적을 중단합니다.
		if (ProjectileMovementComponent->bIsHomingProjectile)
		{
			ProjectileMovementComponent->bIsHomingProjectile = false;
			ProjectileMovementComponent->HomingTargetComponent = nullptr;

			// 0.5초 후 파괴되도록 설정합니다.
			SetLifeSpan(0.5f);
		}
	}
}

void ALetheProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SphereComponent->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnComponentBeginOverlap);

	if (LoopingSoundComponent)
	{
		LoopingSoundComponent->Stop();
		LoopingSoundComponent->DestroyComponent();
	}
	
	Super::EndPlay(EndPlayReason);
}

void ALetheProjectile::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsHandled)
	{
		return;
	}

	// Overlap 대상이 추적 중인 대상인 경우에만 Effect 적용을 수행합니다.
	if (OtherActor == Payload.TargetActor)
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
		
		const FVector PlayFXLocation = bFromSweep ? FVector(SweepResult.Location) : GetActorLocation();
		PlayHitFXs(PlayFXLocation);
		
		bIsHandled = true;
		Destroy();
	}
}

void ALetheProjectile::PlayHitFXs(const FVector& PlayFXLocation) const
{
	if (UFXManagerSubsystem* FXManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFXManagerSubsystem>())
	{
		if (ImpactSoundTag.IsValid())
		{
			FXManagerSubsystem->AsyncPlaySoundAtLocation(ImpactSoundTag, PlayFXLocation, FRotator::ZeroRotator, Payload.VolumeMultiplier);
		}
		if (ImpactEffectTag.IsValid())
		{
			FXManagerSubsystem->AsyncSpawnNiagaraAtLocation(ImpactEffectTag, PlayFXLocation);
		}
	}
}

