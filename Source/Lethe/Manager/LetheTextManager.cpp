#include "LetheTextManager.h"

FName FLetheTextManager::GetPath(const EStringTableType Type)
{
	switch (Type)
	{
	case EStringTableType::Card:
		return TEXT("/Game/Data/StringTable/ST_Card");
	default:
		break;
	}

	return FName();
}