// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/Data/Stage/RoomData.h"
#include "Subsystems/WorldSubsystem.h"
#include "RoomManagerSubsystem.generated.h"

enum class ETileVisionState : uint8;

UCLASS()
class LETHE_API URoomManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Deinitialize() override;
	//~ End of USubsystem Interface
	
	void SetRoomData(TMap<int32, FRoomData>&& InRoomData);

	void NotifyActorTileChanged(AActor* InActor, const ATile* OldTile, const ATile* NewTile);
	const FRoomData* GetRoomData(const int32 RoomId) const;

private:
	void UpdatePlayerRoomState(const ATile* OldTile, const ATile* NewTile);
	void ChangeTileVisionState(const int32 InRoomId, FRoomData* RoomData, const ETileVisionState VisionState) const;
	void ApplyActorVisibilityByTile(AActor* Actor, const ETileVisionState VisionState) const;

	FRoomData* GetMutableRoomData(const int32 RoomId);

private:
	UPROPERTY()
	TMap<int32, FRoomData> RoomDataMap;
};
