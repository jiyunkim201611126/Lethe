// Copyright JETBLU, Inc. All Rights Reserved.

#include "TriggerActionComponent.h"

void UTriggerActionComponent::Action()
{
	FTriggeredActionContext Context;
	Context.InstigatorActor = GetOwner();
	
	for (UTriggerAction* TriggerAction : Actions)
	{
		TriggerAction->Action(Context);
	}
}
