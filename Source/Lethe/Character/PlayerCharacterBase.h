// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LetheCharacterBase.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "PlayerCharacterBase.generated.h"

UCLASS()
class LETHE_API APlayerCharacterBase : public ALetheCharacterBase, public IPlayerCharacterInterface
{
	GENERATED_BODY()

public:
	APlayerCharacterBase(const FObjectInitializer& ObjectInitializer);
	
	virtual FGameplayTag GetCharacterTag() override;

	//~ Begin IPlayerCharacterInterface
	virtual void SetPersonalColor(const FLinearColor& InColor) override;
	virtual const FLinearColor& GetPersonalColor() const override;
	virtual void SetPlayerOrderIndex(const int32 Index) override;
	virtual int32 GetPlayerOrderIndex() const override;
	//~ End of IPlayerCharacterInterface

protected:
	/**
	 * 캐릭터 식별을 위해 사용하는 Id입니다.
	 * uint64는 Blueprint 미러링을 지원하지 않기 때문에 캐릭터만 예외로 int64를 사용합니다. 이 같은 이유로 양수여야만 합니다.
	 * ※!!  Id는 출시 이후 절대 변경되어선 안 됩니다  !!※
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LetheCharacter")
	int64 CharacterId;
	
	/**
	 * Id로 찾은 Tag가 런타임 중 동적으로 채워집니다.
	 * Id는 출시 이후 절대 변경되지 않으나, CharacterTag는 필요에 따라 변경될 수 있기 때문에 이와 같은 방법을 사용합니다.
	 */
	FGameplayTag CharacterTag;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWidgetComponent> MarkerWidgetComponent;

private:
	FLinearColor PersonalColor;

	int32 PlayerOrderIndex = INDEX_NONE;
};
