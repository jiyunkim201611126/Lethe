// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAbilityTypes.h"

void FLetheGameplayEffectContext::SetCueDataContext(const FCueDataContext& InCueDataContext)
{
	CueDataContext = InCueDataContext;
}

const FCueDataContext& FLetheGameplayEffectContext::GetCueDataContext() const
{
	return CueDataContext;
}
