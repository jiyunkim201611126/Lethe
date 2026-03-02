// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAssetManager.h"

#include "AbilitySystemGlobals.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Manager/LetheGameplayTags.h"

ULetheAssetManager& ULetheAssetManager::Get()
{
	check(GEngine);
	ULetheAssetManager* LetheAssetManager = Cast<ULetheAssetManager>(GEngine->AssetManager);
	return *LetheAssetManager;
}

void ULetheAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	// 전역으로 선언되어있는 GameplayTags 인스턴스를 초기화합니다.
	FLetheGameplayTags::InitializeNativeGameplayTags();

	// GameplayTags 초기화 직후 Attribute와 Tag를 매핑합니다.
	ULetheAttributeSet::InitializeAttributeTagMap();

	// 커스텀 Context를 사용하기 위해 반드시 호출해줘야 하는 함수입니다.
	UAbilitySystemGlobals::Get().InitGlobalData();
}
