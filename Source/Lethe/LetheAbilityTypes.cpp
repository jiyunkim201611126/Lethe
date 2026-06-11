// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAbilityTypes.h"

void FLetheGameplayEffectContext::SetCueDataPayload(const FCueDataPayload& InCueDataPayload)
{
	CueDataPayload = InCueDataPayload;
}

const FCueDataPayload& FLetheGameplayEffectContext::GetCueDataPayload() const
{
	return CueDataPayload;
}
