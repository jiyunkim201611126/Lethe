// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAbilitySystemLibrary.h"

#include "LetheAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/LetheAbilityTypes.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"
#include "Lethe/UI/Framework/LetheHUD.h"

class UTileManagerSubsystem;

UOverlayWidgetController* ULetheAbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (const ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
		{
			if (ULetheHUD* LetheHUD = LethePlayerController->GetLetheHUD())
			{
				return LetheHUD->GetOrCreateOverlayWidgetController();
			}
		}
	}
	return nullptr;
}

UCardPanelWidgetController* ULetheAbilitySystemLibrary::GetCardPanelWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (const ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
		{
			if (ULetheHUD* LetheHUD = LethePlayerController->GetLetheHUD())
			{
				return LetheHUD->GetOrCreateCardPanelWidgetController();
			}
		}
	}
	return nullptr;
}

UViewCardDetailWidgetController* ULetheAbilitySystemLibrary::GetViewCardDetailWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (const ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
		{
			if (ULetheHUD* LetheHUD = LethePlayerController->GetLetheHUD())
			{
				return LetheHUD->GetOrCreateViewCardDetailWidgetController();
			}
		}
	}
	return nullptr;
}

bool ULetheAbilitySystemLibrary::CanUseAbilityByTileAndFloorGap(const ATile* SourceTile, const ATile* TargetTile, const int32 MaxFloorGap)
{
	if (SourceTile && TargetTile)
	{
		if (const UTileManagerSubsystem* TileManagerSubsystem = SourceTile->GetWorld()->GetSubsystem<UTileManagerSubsystem>())
		{
			const int32 CurrentFloor = TileManagerSubsystem->GetTileFloor(SourceTile);
			const int32 TargetFloor = TileManagerSubsystem->GetTileFloor(TargetTile);
			const int32 FloorGap = FMath::Abs(CurrentFloor - TargetFloor);
			return FloorGap <= MaxFloorGap;
		}
	}
	return false;
}

void ULetheAbilitySystemLibrary::ResolveDamageRules(const UAbilitySystemComponent* SourceASC, const UAbilitySystemComponent* TargetASC, const float IncomingDamage, TMap<FGameplayAttribute, float>& OutDataForSource, TMap<FGameplayAttribute, float>& OutDataForTarget)
{
	// 절대 Out 데이터 안에 IncomingDamage를 넣어선 안 됩니다.
}

void ULetheAbilitySystemLibrary::SetCueContextToEffectContext(const FCueDataPayload& CueDataContext, FGameplayEffectContextHandle& OutHandle)
{
	if (FLetheGameplayEffectContext* EffectContext = static_cast<FLetheGameplayEffectContext*>(OutHandle.Get()))
	{
		EffectContext->SetCueDataPayload(CueDataContext);
	}
}

bool ULetheAbilitySystemLibrary::GetCueDataContext(const FGameplayEffectContextHandle& EffectContextHandle, FCueDataPayload& OutCueDataContext)
{
	if (const FLetheGameplayEffectContext* EffectContext = static_cast<const FLetheGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		OutCueDataContext = EffectContext->GetCueDataPayload();
		return true;
	}
	return false;
}

TArray<FRotator> ULetheAbilitySystemLibrary::EvenlySpacedRotators(const FVector& Forward, const FVector& Axis, const float Spread, const int32 NumOfRotators)
{
	const FVector LeftOfSpread = Forward.RotateAngleAxis(-Spread / 2.f, Axis);
	const float DeltaSpread = NumOfRotators > 1 ? Spread / (NumOfRotators - 1) : 0.f;

	TArray<FRotator> ResultRotators;
	ResultRotators.Reserve(NumOfRotators);
	for (int32 Index = 0; Index < NumOfRotators; ++Index)
	{
		const FVector Direction = NumOfRotators > 1 ? LeftOfSpread.RotateAngleAxis(DeltaSpread * Index, FVector::UpVector) : Forward;
		ResultRotators.Add(Direction.Rotation());
	}
	return ResultRotators;
}
