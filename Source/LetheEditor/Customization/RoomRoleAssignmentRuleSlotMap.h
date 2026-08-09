// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/Data/Stage/CubeCoord.h"
#include "Widgets/SLeafWidget.h"

class IPropertyHandle;

struct FSlotEntry
{
	int32 Index = INDEX_NONE;
	FCubeCoord Coord;
	FString SlotType;
	FString SpawnActorClass;
	int32 Distance = 0;
	bool bValidCubeCoord = true;
	bool bWithinRequiredDistance = true;
	bool bDuplicateCoord = false;
};

class SRoomRoleAssignmentRuleSlotMap : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SRoomRoleAssignmentRuleSlotMap)
		: _CoordSlotsHandle()
		, _RequiredDistanceHandle()
	{}
		SLATE_ARGUMENT(TSharedPtr<IPropertyHandle>, CoordSlotsHandle)
		SLATE_ARGUMENT(TSharedPtr<IPropertyHandle>, RequiredDistanceHandle)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	FText GetSummaryText() const;
	FText GetWarningText() const;
	
private:
	void ReadSlotEntries(TArray<FSlotEntry>& OutEntries, int32& OutMaxRadius, int32& OutRequiredDistance) const;
	static FString MakeSlotLabel(const TArray<FSlotEntry>& Entries);
	static FVector2D GetCellCenter(const FCubeCoord& Coord, const FVector2D& MapCenter, float CellDistance);
	static void MakeHexagon(const FVector2D& Center, float Radius, TArray<FVector2D>& OutPoints);

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	TSharedPtr<IPropertyHandle> CoordSlotsHandle;
	TSharedPtr<IPropertyHandle> RequiredDistanceHandle;
};
