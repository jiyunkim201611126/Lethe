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
			if (const ULetheHUD* LetheHUD = LethePlayerController->GetLetheHUD())
			{
				return LetheHUD->GetOverlayWidgetController();
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
			if (const ULetheHUD* LetheHUD = LethePlayerController->GetLetheHUD())
			{
				return LetheHUD->GetCardPanelWidgetController();
			}
		}
	}
	return nullptr;
}

bool ULetheAbilitySystemLibrary::CanUseAbilityByActorAndFloorGap(const AActor* SourceActor, const AActor* TargetActor, const int32 MaxFloorGap)
{
	if (SourceActor && TargetActor)
	{
		if (const UTileManagerSubsystem* TileManagerSubsystem = SourceActor->GetWorld()->GetSubsystem<UTileManagerSubsystem>())
		{
			const ATile* SourceTile = TileManagerSubsystem->GetTileUnderActor(SourceActor);
			const ATile* TargetTile = TileManagerSubsystem->GetTileUnderActor(TargetActor);
			if (SourceTile && TargetTile)
			{
				return CanUseAbilityByTileAndFloorGap(SourceTile, TargetTile, MaxFloorGap);
			}
		}
	}
	return false;
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

void ULetheAbilitySystemLibrary::SetCueContextToEffectContext(const FCueDataContext& CueDataContext, FGameplayEffectContextHandle& OutHandle)
{
	if (FLetheGameplayEffectContext* EffectContext = static_cast<FLetheGameplayEffectContext*>(OutHandle.Get()))
	{
		EffectContext->SetCueDataContext(CueDataContext);
	}
}

bool ULetheAbilitySystemLibrary::GetCueDataContext(const FGameplayEffectContextHandle& EffectContextHandle, FCueDataContext& OutCueDataContext)
{
	if (const FLetheGameplayEffectContext* EffectContext = static_cast<const FLetheGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		OutCueDataContext = EffectContext->GetCueDataContext();
		return true;
	}
	return false;
}
