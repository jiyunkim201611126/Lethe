#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFXManager, Log, All);

#define LETHE_LOG(Category, Verbosity, Format, ...) \
	UE_LOG(Category, Verbosity, TEXT("[%s: %d] " Format), TEXT(__FUNCTION), __LINE__, ##__VA_ARGS__);
