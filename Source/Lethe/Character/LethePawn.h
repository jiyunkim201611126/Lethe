// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "LethePawn.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

/**
 * 카메라와 월드 입력을 담당하는 기본 Pawn입니다.
 */
UCLASS()
class LETHE_API ALethePawn : public APawn
{
	GENERATED_BODY()

public:
	ALethePawn();

	/**
	 * PlayerCharacter와 Enemy가 하나씩 갖고 있는 AttributeWidget의 사이즈가 Camera의 위치에 따라 동적으로 계산됩니다.
	 * 게임 시작 직후엔 바인드가 이루어져있지 않기 때문에 Character들이 직접 가져갑니다.
	 */
	float GetAttributeWidgetSize() const;

	void SetPawnStartLocation();

protected:
	//~ Begin APawn Interface
	virtual void PossessedBy(AController* NewController) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	//~ End of APawn Interface

private:
	void Move(const FInputActionValue& InputActionValue);
	void Zoom(const FInputActionValue& InputActionValue);
	void SpaceKeyPressed();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USceneComponent> RootSceneComponent;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MouseWheelAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SpaceAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> UIInputContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float MoveSpeed = 10.f;
};
