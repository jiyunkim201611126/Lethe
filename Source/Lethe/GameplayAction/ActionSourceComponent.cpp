// Copyright JETBLU, Inc. All Rights Reserved.

#include "ActionSourceComponent.h"

void UActionSourceComponent::Action()
{
	FTriggeredActionContext Context;
	Context.InstigatorActor = GetOwner();
	
	for (UTriggerAction* TriggerAction : Actions)
	{
		TriggerAction->Action(Context);
	}
}
