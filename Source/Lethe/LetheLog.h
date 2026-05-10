#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTile, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogFXManager, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogBGMManager, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogTextManager, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAttrbitueSet, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAbility, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAbilityResolver, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAIController, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogLetheGameState, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogRoomManager, Log, All);

#define LETHE_LOG(Category, Verbosity, Format, ...) UE_LOG(Category, Verbosity, TEXT("[%s: %d] " Format), ANSI_TO_TCHAR(__FUNCTION__), __LINE__, ##__VA_ARGS__);
