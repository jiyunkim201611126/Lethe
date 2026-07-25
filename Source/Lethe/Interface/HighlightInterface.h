// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HighlightInterface.generated.h"

/** 플래그를 상속받은 클래스가 직접 선언해 갖고 있고, 플래그 상태에 따라 어떤 색상을 표시할지 결정합니다. */
UENUM(BlueprintType)
enum class EHighlightReason : uint8
{
	None = 0,

	SelectedCharacter = 1 << 0,

	TargetedByAI = 1 << 1,
	SelectCandidate = 1 << 2, // 카드 선택 및 캐릭터 선택 시 선택 가능한 대상
	Source = 1 << 3, // 카드 선택 시 카드 소유자 등 하이라이트 주체
	TargetCandidate = 1 << 4, // 마우스 위치에 따라 실제 발동 범위로 선택된 대상
};
ENUM_CLASS_FLAGS(EHighlightReason)

UINTERFACE()
class UHighlightInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 하이라이트용 OutlineColor는 상속받은 클래스가 직접 선언해 갖고 있거나, Lethe.h에 정의한 값을 사용합니다.
 */
class LETHE_API IHighlightInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Highlight(const EHighlightReason Reason);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Unhighlight(const EHighlightReason Reason);
};
