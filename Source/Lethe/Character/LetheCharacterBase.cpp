// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "LethePawn.h"
#include "Component/GASManagerComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/Interface/TileVisionAffectedInterface.h"
#include "Lethe/Manager/EngineSystem/LetheAssetManager.h"
#include "Lethe/UI/Framework/LetheUserWidget.h"

ALetheCharacterBase::ALetheCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Tile, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Destructible, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Block);

	GetMesh()->SetGenerateOverlapEvents(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	GASManagerComponent = CreateDefaultSubobject<UGASManagerComponent>(TEXT("GASManagerComponent"));
	AbilitySystemComponent = CreateDefaultSubobject<ULetheAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AttributeSet = CreateDefaultSubobject<ULetheAttributeSet>(TEXT("AttributeSet"));

	CharacterStatusWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("CharacterStatusWidgetComponent"));
	CharacterStatusWidgetComponent->SetupAttachment(RootComponent);
}

UAbilitySystemComponent* ALetheCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ALetheCharacterBase::MoveToTile(TArray<ATile*>& PathTiles, const bool bTeleport)
{
	// 캐릭터 절반 높이만큼 위로 올려줍니다.
	const float ZOffset = GetDefaultHalfHeight();

	if (bTeleport)
	{
		// TargetTile로 즉시 이동합니다.
		if (!PathTiles.IsEmpty())
		{
			ATile* TargetTile = PathTiles.Last();
			FVector TargetTileLocation = TargetTile->GetActorLocation();
			TargetTileLocation.Z = TargetTileLocation.Z + ZOffset;
			SetActorLocation(TargetTileLocation);
			
			OnMoveTileChanged(nullptr, TargetTile);
			
			// Hidden 상태를 갱신합니다. PlayerCharacter는 구현되지 않았고, Enemy에서만 동작합니다.
			if (Implements<UTileVisionAffectedInterface>())
			{
				ITileVisionAffectedInterface::Execute_UpdateHiddenByTile(this, TargetTile);
			}
		}
		return;
	}

	// MovePath에 모두 집어넣습니다.
	for (ATile* PathTile : PathTiles)
	{
		if (PathTile)
		{
			MovePath.Add(PathTile);
		}
	}
}

int32 ALetheCharacterBase::GetMoveRange() const
{
	return FMath::Max(0, FMath::FloorToInt(AttributeSet->GetMoveRange()));
}

int32 ALetheCharacterBase::GetMaxMoveRange() const
{
	return FMath::Max(0, FMath::FloorToInt(AttributeSet->GetMaxMoveRange()));
}

void ALetheCharacterBase::OnDamageTaken()
{
	// TODO: 피격 애니메이션 재생 등
}

void ALetheCharacterBase::Die()
{
	GASManagerComponent->OnDied();
}

bool ALetheCharacterBase::IsDead()
{
	return GASManagerComponent->IsDead();
}

UAnimMontage* ALetheCharacterBase::GetMoveAnimation()
{
	return MoveAnimation;
}

void ALetheCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	TArray<UWidgetComponent*> WidgetComponents;
	GetComponents(UWidgetComponent::StaticClass(), WidgetComponents);
	WidgetComponents.RemoveAllSwap([](const UWidgetComponent* Component)
	{
		if (const UUserWidget* Widget = Component->GetWidget())
		{
			return !Widget->IsA(ULetheUserWidget::StaticClass());
		}
		return true;
	});
	AttributeWidgetComponents = MoveTemp(WidgetComponents);

	InitAbilityActorInfo();

	if (ALethePlayerController* PlayerController = Cast<ALethePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->OnCameraHeightChangedDelegate.AddUObject(this, &ALetheCharacterBase::OnCameraHeightChanged);
		if (const ALethePawn* LethePawn = Cast<ALethePawn>(PlayerController->GetPawn()))
		{
			OnCameraHeightChanged(LethePawn->GetAttributeWidgetSize());
		}
	}
}

void ALetheCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 경로에 최소 2개의 타일이 들어있어야 합니다.
	if (MovePath.Num() >= 2)
	{
		// MovePath에 경로가 순서대로 정렬되어 있으므로, 맨 앞에서부터 꺼내 사용합니다.
		const auto& CurrentTargetTile = MovePath[1];
		if (!CurrentTargetTile.IsValid())
		{
			MovePath.Empty();
			return;
		}

		// 타일 속에 캐릭터가 파묻히지 않도록 Z축을 보정합니다.
		const float ZOffset = GetDefaultHalfHeight();
		FVector CurrentTargetLocation = CurrentTargetTile->GetActorLocation();
		CurrentTargetLocation.Z += ZOffset;

		// 이번 프레임 위치를 계산합니다.
		const float Speed = GetCharacterMovement()->MaxWalkSpeed;
		const FVector NewLocation = FMath::VInterpConstantTo(GetActorLocation(), CurrentTargetLocation, DeltaSeconds, Speed);

		// 현재 위치와 목표 위치간 차이가 얼마나 되는지 계산합니다.
		const float DistanceSquaredToTarget = FVector::DistSquared(GetActorLocation(), CurrentTargetLocation);
		
		if (DistanceSquaredToTarget <= FMath::Square(HiddenTolerance))
		{
			// 목표 위치에 Hidden 갱신 임계값까지 가까워졌다면 Hidden 상태를 갱신합니다. PlayerCharacter는 구현되지 않았고, Enemy에서만 동작합니다.
			if (Implements<UTileVisionAffectedInterface>())
			{
				ITileVisionAffectedInterface::Execute_UpdateHiddenByTile(this, CurrentTargetTile.Get());
			}
		}
		
		if (DistanceSquaredToTarget <= FMath::Square(MoveArriveTolerance))
		{
			OnMoveTileChanged(MovePath[0].Get(), CurrentTargetTile.Get());
			
			// 목표 위치에 아주 가까워졌다면 목표 위치를 그대로 사용합니다.
			SetActorLocation(CurrentTargetLocation);
			MovePath.RemoveAt(0);

			// 경로에 1개만 남아있다면 목적지에 도착한 상태이므로, 내용물을 비웁니다.
			if (MovePath.Num() == 1)
			{
				MovePath.Reset();
			}
		}
		else
		{
			SetActorLocation(NewLocation);
		}
	}
	else
	{
		MovePath.Reset();
	}
}

void ALetheCharacterBase::OnMoveTileChanged(ATile* PreviousTile, ATile* CurrentTile)
{
}

void ALetheCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ALethePlayerController* PlayerController = Cast<ALethePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->OnCameraHeightChangedDelegate.RemoveAll(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

void ALetheCharacterBase::HighlightActorByMouse_Implementation()
{
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(OutlineColor);
}

void ALetheCharacterBase::HighlightActorTransparentByMouse_Implementation()
{
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(OutlineColor);
}

void ALetheCharacterBase::UnhighlightActorByMouse_Implementation()
{
	GetMesh()->SetRenderCustomDepth(false);
}

void ALetheCharacterBase::HighlightActorByAbility_Implementation(const int32 InOutlineColor)
{
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(OutlineColor);
}

void ALetheCharacterBase::UnhighlightActorByAbility_Implementation()
{
	GetMesh()->SetRenderCustomDepth(false);
}

void ALetheCharacterBase::InitAbilityActorInfo() const
{
	GASManagerComponent->SetAbilitySystemComponent(AbilitySystemComponent);
	GASManagerComponent->SetAttributeSet(AttributeSet);

	TArray<UUserWidget*> AttributeWidgets;
	for (const UWidgetComponent* WidgetComponent : AttributeWidgetComponents)
	{
		AttributeWidgets.Add(WidgetComponent->GetWidget());
	}
	GASManagerComponent->InitAbilityActorInfo(AttributeWidgets);
}

void ALetheCharacterBase::OnCameraHeightChanged(const float InWidgetSize) const
{
	for (const UWidgetComponent* WidgetComponent : AttributeWidgetComponents)
	{
		if (UUserWidget* Widget = WidgetComponent->GetWidget())
		{
			Widget->SetRenderScale(FVector2D(InWidgetSize));
		}
	}
}

FGameplayTag ALetheCharacterBase::GetCharacterTag()
{
	if (!CharacterTag.IsValid())
	{
		const ULetheAssetManager& LetheAssetManager = ULetheAssetManager::Get();
		LetheAssetManager.TryGetCharacterTagById(CharacterId, CharacterTag);
	}
	return CharacterTag;
}

int64 ALetheCharacterBase::GetCharacterId() const
{
	return CharacterId;
}

ETeamSide ALetheCharacterBase::GetTeamSide() const
{
	return TeamSide;
}
