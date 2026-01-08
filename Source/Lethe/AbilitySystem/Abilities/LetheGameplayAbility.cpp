// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameplayAbility.h"

#include "Lethe/Manager/LetheTextManager.h"

void ULetheGameplayAbility::ApplyAllEffects(AActor* TargetActor)
{
	for (UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		if (EffectApplier && TargetActor)
		{
			EffectApplier->ApplyEffect(this, TargetActor);
		}
	}
}

FText ULetheGameplayAbility::GetCardName() const
{
	return FLetheTextManager::GetText(EStringTableType::Card, CardNameTextKey);
}

FGameplayEffectContextHandle ULetheGameplayAbility::GetContextHandle(const TSubclassOf<UGameplayEffectApplier>& ApplierClass) const
{
	for (const auto EffectApplier : EffectAppliers)
	{
		if (EffectApplier && EffectApplier->GetClass() == ApplierClass)
		{
			return EffectApplier->GetEffectContextHandle();
		}
	}
	return FGameplayEffectContextHandle();
}

void ULetheGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
	for (const auto EffectApplier : EffectAppliers)
	{
		EffectApplier->CancelAbility();
	}
	
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void ULetheGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	for (const auto EffectApplier : EffectAppliers)
	{
		EffectApplier->EndAbility();
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


#if WITH_EDITOR
#include "Lethe/Manager/LetheGameplayTags.h"

void ULetheGameplayAbility::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ULetheGameplayAbility, AbilityTag))
	{
		SyncAbilityTagToAssetTags();
	}
}

void ULetheGameplayAbility::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ActivationBlockedTags.AddTag(FLetheGameplayTags::Get().CharacterState_Dead);
	}
}

void ULetheGameplayAbility::SyncAbilityTagToAssetTags()
{
	// GetAssetTags를 통해 반환받는 변수인 AbilityTags는 GameplayAbility의 멤버 변수입니다.
	// 추후 AssetTags라는 이름으로 변경될 예정이며, 메타데이터 역할을 수행하기 때문에 런타임 중 변경되는 것을 금지하고 있습니다.
	// 따라서 해당 함수는 에디터에서만 호출됩니다.
	if (AbilityTag.IsValid() && !GetAssetTags().HasTag(AbilityTag))
	{
		FGameplayTagContainer NewAbilityTags;
		NewAbilityTags.AddTag(AbilityTag);
		for (auto AssetTag : GetAssetTags())
		{
			NewAbilityTags.AddTag(AssetTag);
		}
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
				// AbilityTag에 값을 할당하면 자동으로 AbilityTags(AssetTags)에도 함께 할당해주는 구문입니다.
				// 에디터에서만 호출되기 때문에 안전합니다.
				AbilityTags = NewAbilityTags;
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
			}
}
#endif
