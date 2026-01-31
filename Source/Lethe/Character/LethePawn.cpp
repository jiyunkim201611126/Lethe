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
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Cast<APlayerController>(GetController())->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputContext, 0);
	}
}

void ALethePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInputComponent->BindAction(NumberAction, ETriggerEvent::Triggered, this, &ThisClass::NumberKeyPressed);
}

void ALethePawn::Move(const FInputActionValue& InputActionValue)
{
	FVector InputValue = InputActionValue.Get<FVector>();
	InputValue.Normalize();

	AddActorLocalOffset(InputValue * MoveSpeed);
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

