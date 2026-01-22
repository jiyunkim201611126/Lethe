// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheRichTextBlock.h"

ULetheRichTextBlock::ULetheRichTextBlock(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	// 처음 배치했을 때 SelfHitTestInvisible로 설정되어 있도록 합니다.
	Visibility = ESlateVisibility::SelfHitTestInvisible;
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
}
