// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffectTypes.h"
#include "LetheAbilityTypes.Generated.h"

USTRUCT(BlueprintType)
struct FCueDataContext
{
	GENERATED_BODY()

	/**
	 * 이곳에 Cue 재생에 필요한 변수를 추가합니다.
	 */

	/** 사운드, 나이아가라 에셋과 매핑된 태그입니다. */
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag AssetTag;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<FVector> Locations;
};

/**
 * GameplayEffect부여나 GameplayCue 재생 등 다양한 로직에서 데이터를 넘길 때 활용하는 구조체입니다.
 * 주로 직렬화를 위해 사용하지만, Cue 재생에서도 요구하기 때문에 Lethe에서도 필요합니다.
 */
USTRUCT(BlueprintType)
struct FLetheGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:
	void SetCueDataContext(const FCueDataContext& InCueDataContext);
	const FCueDataContext& GetCueDataContext() const;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	virtual FGameplayEffectContext* Duplicate() const override
	{
		FLetheGameplayEffectContext* NewContext = new FLetheGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

protected:
	UPROPERTY()
	FCueDataContext CueDataContext;
};

template<>
struct TStructOpsTypeTraits<FLetheGameplayEffectContext> : TStructOpsTypeTraitsBase2<FLetheGameplayEffectContext>
{
	enum
	{
		WithCopy = true		// 복사 연산 지원
	};
};

