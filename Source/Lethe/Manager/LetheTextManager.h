#pragma once

#include "CoreMinimal.h"

UENUM()
enum class EStringTableType : uint8
{
	Card,
};

/**
 * String Table에서 Text를 가져오는 전역 함수용 클래스입니다.
 * Localization Dashboard에서 String Table의 경로를 잡으면 Source String들이 자동으로 수집됩니다.
 * 언리얼에선 자동 수집, 언어 1:1 대응 UI, po파일 추출까지 지원하나, 번역할 언어가 많으면 불편합니다.
 * po파일을 모아 엑셀로 변환, 엑셀 파일을 다시 po파일로 변환하는 자동화 스크립트를 만들면 언어 통합 번역 시트를 제작할 수 있습니다.
 */
class FLetheTextManager
{
public:
	template <typename... ArgTypes>
	static FORCEINLINE FText GetText(EStringTableType Type, const FString& Key, ArgTypes... Args)
	{
		const FName TableId = GetPath(Type);
		FTextFormat Text = FText::FromStringTable(TableId, *Key);

		if (Text.GetSourceText().IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("Text가 비어있습니다. Table: %s / Key: %s"), *TableId.ToString(), *Key);
		}

		if constexpr (sizeof...(Args) > 0)
		{
			return FText::Format(MoveTemp(Text), MoveTemp(Args)...);
		}
		else
		{
			return Text.GetSourceText();
		}
	}

private:
	static FName GetPath(const EStringTableType Type);
};
