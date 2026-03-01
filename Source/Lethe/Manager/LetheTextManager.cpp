#include "LetheTextManager.h"

FName FLetheTextManager::GetPath(const EStringTableType Type)
{
	switch (Type)
	{
	case EStringTableType::CardDescription:
		return TEXT("/Game/Data/StringTable/ST_CardDescription");
	default:
		break;
	}

	return FName();
}