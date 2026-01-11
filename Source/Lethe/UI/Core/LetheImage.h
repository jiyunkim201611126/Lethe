// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Image.h"
#include "LetheImage.generated.h"

/**
 * UImage 배치 시 기본적으로 Visibility가 Visible로 설정되어 있는 걸 해결하기 위해 선언된 클래스입니다.
 * UImage 대신 반드시 이 클래스를 사용해 주세요.
 */
UCLASS()
class LETHE_API ULetheImage : public UImage
{
	GENERATED_BODY()

	ULetheImage();
};
