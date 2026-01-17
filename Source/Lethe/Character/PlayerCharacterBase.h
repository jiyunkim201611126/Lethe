// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LetheCharacterBase.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"
#include "PlayerCharacterBase.generated.h"

UCLASS()
class LETHE_API APlayerCharacterBase : public ALetheCharacterBase, public IPlayableCharacterInterface
{
	GENERATED_BODY()

public:
	virtual FColor GetCardFrontsideColor() override;
	virtual FColor GetCardBacksideColor() override;
	virtual FGameplayTag GetCharacterTag() override;	

protected:
	// 캐릭터마다 다른 카드 색깔을 지정하기 위해 이곳에 선언합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FColor CardFrontsideColor;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FColor CardBacksideColor;

	// 캐릭터 식별을 위해 사용하는 태그입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FGameplayTag CharacterTag;
};
