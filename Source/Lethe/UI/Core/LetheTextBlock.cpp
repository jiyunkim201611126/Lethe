// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheTextBlock.h"

ULetheTextBlock::ULetheTextBlock()
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	// 처음 배치했을 때 SelfHitTestInvisible로 설정되어 있도록 합니다.
	Visibility = ESlateVisibility::SelfHitTestInvisible;
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
}
