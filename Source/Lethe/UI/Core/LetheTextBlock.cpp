// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheTextBlock.h"

ULetheTextBlock::ULetheTextBlock()
{
	// 처음 배치했을 때 SelfHitTestInvisible로 설정되어 있도록 합니다.
	Visibility = ESlateVisibility::SelfHitTestInvisible;
}
