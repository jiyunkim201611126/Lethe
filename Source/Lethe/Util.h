#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

namespace ArrayShuffle
{
	template <typename T, typename AllocatorType>
	FORCEINLINE void ShuffleWithSeed(TArray<T, AllocatorType>& Array, const FRandomStream& Stream)
	{
		const int32 Num = Array.Num();
		if (Num <= 1)
		{
			return;
		}

		for (int32 i = Num - 1; i > 0; --i)
		{
			const int32 j = Stream.RandRange(0, i);
			if (i != j)
			{
				Array.Swap(i, j);
			}
		}
	}
}
