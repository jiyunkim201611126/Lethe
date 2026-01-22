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
	virtual FGameplayTag GetCharacterTag() const override;	

protected:
	// 캐릭터 식별을 위해 사용하는 태그입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FGameplayTag CharacterTag;
};
