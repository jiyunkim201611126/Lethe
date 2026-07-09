// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LetheAbilitySystemLibrary.generated.h"

struct FGameplayEffectContextHandle;
struct FCueDataPayload;
class ATile;
class ALetheHUD;
class UAbilitySystemComponent;
class UCardPanelWidgetController;
class UOverlayWidgetController;
class UViewCardDetailWidgetController;
struct FGameplayAttribute;

UCLASS()
class LETHE_API ULetheAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:	
	UFUNCTION(BlueprintPure, Category = "LetheAbilitySystemLibrary | Ability")
	static bool CanUseAbilityByTileAndFloorGap(const ATile* SourceTile, const ATile* TargetTile, const int32 MaxFloorGap);

	/** 반사 데미지, 흡혈 등 ExecCalc만으로는 해결할 수 없는 데미지 규칙의 경우, 실제 적용과 Preview에서 모두 사용하기 위해 공용으로 구현된 함수입니다. */
	static void ResolveDamageRules(const UAbilitySystemComponent* SourceASC, const UAbilitySystemComponent* TargetASC, const float IncomingDamage, TMap<FGameplayAttribute, float>& OutDataForSource, TMap<FGameplayAttribute, float>& OutDataForTarget);

	static void SetCueContextToEffectContext(const FCueDataPayload& CueDataContext, FGameplayEffectContextHandle& OutHandle);
	
	UFUNCTION(BlueprintPure, Category = "LetheAbilitySystemLibrary | Cue")
	static bool GetCueDataContext(UPARAM(ref)const FGameplayEffectContextHandle& EffectContextHandle, FCueDataPayload& OutCueDataContext);

	UFUNCTION(BlueprintCallable, Category = "LetheAbilitySystemLibrary | GameplayMechanics")
	static TArray<FRotator> EvenlySpacedRotators(UPARAM(ref) const FVector& Forward, UPARAM(ref) const FVector& Axis, const float Spread, const int32 NumOfRotators);
};
