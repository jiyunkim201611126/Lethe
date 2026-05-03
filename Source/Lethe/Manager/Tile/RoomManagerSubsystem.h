// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/Data/Stage/RoomData.h"
#include "Subsystems/WorldSubsystem.h"
#include "RoomManagerSubsystem.generated.h"

UCLASS()
class LETHE_API URoomManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Deinitialize() override;
	//~ End of USubsystem Interface
	
	void SetRoomData(TMap<int32, FRoomData>&& InRoomData);

	const FRoomData* GetRoomData(const int32 RoomId) const;

private:
	UPROPERTY()
	TMap<int32, FRoomData> RoomDataMap;
};
