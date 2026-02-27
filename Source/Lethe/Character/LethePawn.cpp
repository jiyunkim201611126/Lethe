// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePawn.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Lethe/Player/PlayerController/LethePlayerController.h"

ALethePawn::ALethePawn()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComponent);

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 1000.f;
	SpringArmComponent->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
	SpringArmComponent->bDoCollisionTest = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void ALethePawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (const APlayerController* PlayerController = Cast<APlayerController>(NewController))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputContext, 0);
		}
	}
}

void ALethePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInputComponent->BindAction(MouseWheelAction, ETriggerEvent::Triggered, this, &ThisClass::Zoom);
	EnhancedInputComponent->BindAction(NumberAction, ETriggerEvent::Triggered, this, &ThisClass::NumberKeyPressed);
	EnhancedInputComponent->BindAction(LeftMouseButtonClickAction, ETriggerEvent::Completed, this, &ThisClass::LeftMouseButtonClicked);
	EnhancedInputComponent->BindAction(RightMouseButtonClickAction, ETriggerEvent::Completed, this, &ThisClass::RightMouseButtonClicked);
}

void ALethePawn::Move(const FInputActionValue& InputActionValue)
{
	FVector InputValue = InputActionValue.Get<FVector>();
	InputValue.Normalize();

	AddActorLocalOffset(InputValue * MoveSpeed);
}

void ALethePawn::Zoom(const FInputActionValue& InputActionValue)
{
	const float InputValue = InputActionValue.Get<float>();

	// TODO: 현재는 SpringArmComponent로 간단하게 구현되어 있으나, 추후 카메라 연출이 필요하면 제거하고 아래 구문도 완전히 수정됩니다.
	const float DesiredTargetArmLength = FMath::Clamp(SpringArmComponent->TargetArmLength + InputValue * MoveSpeed, 400.f, 1000.f);
	SpringArmComponent->TargetArmLength = DesiredTargetArmLength;
	
	if (const ALethePlayerController* PlayerController = Cast<ALethePlayerController>(GetController()))
	{
		PlayerController->OnWheeled(GetAttributeWidgetSize());
	}
}

float ALethePawn::GetAttributeWidgetSize() const
{
	return FMath::Clamp((1000.f - SpringArmComponent->TargetArmLength) / 600.f, 0.4f, 1.f);
}

void ALethePawn::NumberKeyPressed(const FInputActionValue& InputActionValue)
{
	// Input 설정에서 값을 0으로 주면 입력 자체가 발생하지 않는 현상이 있어, 설정에서 최소값으로 1을 준 뒤 여기서 1을 빼고 사용합니다.
	const float RawValue = InputActionValue.Get<float>() - 1;
	const int32 InputIndex = FMath::RoundToInt(RawValue);

	if (const ALethePlayerController* PlayerController = Cast<ALethePlayerController>(GetController()))
	{
		PlayerController->OnNumberPressed(InputIndex);
	}
}

void ALethePawn::LeftMouseButtonClicked()
{
	if (ALethePlayerController* PlayerController = Cast<ALethePlayerController>(GetController()))
	{
		PlayerController->OnLeftMouseButtonClickedOnWorld();
	}
}

void ALethePawn::RightMouseButtonClicked()
{
	if (ALethePlayerController* PlayerController = Cast<ALethePlayerController>(GetController()))
	{
		PlayerController->ResetSelectedCharacter();
	}
}
