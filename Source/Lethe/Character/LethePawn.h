// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "LethePawn.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * 카메라를 담당하는 기본 Pawn입니다.
 */
UCLASS()
class LETHE_API ALethePawn : public APawn
{
	GENERATED_BODY()

public:
	ALethePawn();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USceneComponent> RootSceneComponent;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
};
