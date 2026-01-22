#include "LetheGameInstance.h"

const FStageData* ULetheGameInstance::GetStageData(const FName& StageName) const
{; 
	return StageDataTable.LoadSynchronous()->FindRow<FStageData>(StageName, TEXT(""));
}