// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Lethe/Data/Stage/StageData.h"
#include "BGMThemeDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FBGMTheme
{
	GENERATED_BODY()

	/** 모든 BGM의 길이는 반드시 동일해야 합니다. */
	UPROPERTY(EditDefaultsOnly)
	float BGMTrackLength = 180.f;

	UPROPERTY(EditDefaultsOnly)
	float FadeDuration = 2.f;

	/** 반드시 오름차순으로 정렬되어 있어야 하며, 반드시 BGMTrackLength보다 낮은 수치여야 합니다. */
	UPROPERTY(EditDefaultsOnly)
	TArray<float> TransitionPoints;

	UPROPERTY(EditDefaultsOnly)
	TMap<FName, TObjectPtr<USoundBase>> Tracks;
};

UCLASS()
class LETHE_API UBGMThemeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	const FBGMTheme* GetTheme(const EStageType StageType) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TMap<EStageType, FBGMTheme> BGMThemes;
};
