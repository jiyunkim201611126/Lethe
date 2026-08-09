// Copyright JETBLU, Inc. All Rights Reserved.

#include "RoomRoleAssignmentRuleSlotMap.h"

#include "Fonts/SlateFontInfo.h"
#include "Lethe/Data/Stage/CubeCoord.h"
#include "Lethe/Data/Stage/RoomRoleAssignmentRuleData.h"
#include "PropertyHandle.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateLayoutTransform.h"
#include "Styling/CoreStyle.h"

namespace RoomRoleAssignmentRuleSlotMap
{
	constexpr float Pi = 3.14159265358979323846f;
	constexpr float SqrtThree = 1.7320508075688772935f;
	constexpr float PreferredCellDistance = 42.f;
	constexpr float MinimumCellDistance = 16.f;
	constexpr float MaximumCellDistance = 54.f;
}

void SRoomRoleAssignmentRuleSlotMap::Construct(const FArguments& InArgs)
{
	CoordSlotsHandle = InArgs._CoordSlotsHandle;
	RequiredDistanceHandle = InArgs._RequiredDistanceHandle;
}

FVector2D SRoomRoleAssignmentRuleSlotMap::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	TArray<FSlotEntry> Entries;
	int32 MaxRadius = 0;
	int32 RequiredDistance = INDEX_NONE;
	ReadSlotEntries(Entries, MaxRadius, RequiredDistance);

	const int32 GridRadius = RequiredDistance == INDEX_NONE ? 0 : FMath::Max(0, RequiredDistance);
	const int32 LayoutRadius = FMath::Max(GridRadius, MaxRadius);
	const float Width = 48.f + (3.f * LayoutRadius + 1.f) * RoomRoleAssignmentRuleSlotMap::PreferredCellDistance;
	const float Height = 48.f + (2.f * RoomRoleAssignmentRuleSlotMap::SqrtThree * LayoutRadius + 1.f) * RoomRoleAssignmentRuleSlotMap::PreferredCellDistance;
	return FVector2D(FMath::Max(260.f, Width), FMath::Max(180.f, Height));
}

int32 SRoomRoleAssignmentRuleSlotMap::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	TArray<FSlotEntry> Entries;
	int32 MaxRadius = 0;
	int32 RequiredDistance = INDEX_NONE;
	ReadSlotEntries(Entries, MaxRadius, RequiredDistance);
	const int32 GridRadius = RequiredDistance == INDEX_NONE ? 0 : FMath::Max(0, RequiredDistance);
	const int32 LayoutRadius = FMath::Max(GridRadius, MaxRadius);

	TMap<FCubeCoord, TArray<FSlotEntry>> EntriesByCoord;
	for (const FSlotEntry& Entry : Entries)
	{
		if (Entry.bValidCubeCoord)
		{
			EntriesByCoord.FindOrAdd(Entry.Coord).Add(Entry);
		}
	}

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FVector2D MapCenter = Size * 0.5f;
	const float Radius = static_cast<float>(FMath::Max(LayoutRadius, 1));
	const float CellDistance = FMath::Clamp(
		FMath::Min(
			(Size.X - 24.f) / (3.f * Radius + 1.f),
			(Size.Y - 24.f) / (2.f * RoomRoleAssignmentRuleSlotMap::SqrtThree * Radius + 1.f)),
		RoomRoleAssignmentRuleSlotMap::MinimumCellDistance,
		RoomRoleAssignmentRuleSlotMap::MaximumCellDistance);
	const float HexRadius = CellDistance / RoomRoleAssignmentRuleSlotMap::SqrtThree;

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform()),
		WhiteBrush,
		ESlateDrawEffect::None,
		FLinearColor(0.025f, 0.03f, 0.04f, 1.f));

	for (int32 Q = -GridRadius; Q <= GridRadius; ++Q)
	{
		for (int32 R = -GridRadius; R <= GridRadius; ++R)
		{
			const int32 S = -Q - R;
			if (FMath::Max3(FMath::Abs(Q), FMath::Abs(R), FMath::Abs(S)) > GridRadius)
			{
				continue;
			}

			const FCubeCoord Coord(Q, R, S);
			const FVector2D CellCenter = GetCellCenter(Coord, MapCenter, CellDistance);
			TArray<FVector2D> HexPoints;
			MakeHexagon(CellCenter, HexRadius, HexPoints);

			const TArray<FSlotEntry>* CellEntries = EntriesByCoord.Find(Coord);
			const bool bHasSlot = CellEntries != nullptr;
			const bool bHasDuplicate = bHasSlot && CellEntries->Num() > 1;
			const FLinearColor CellColor = bHasDuplicate
				? FLinearColor(1.f, 0.2f, 0.1f, 1.f)
				: bHasSlot
					? FLinearColor(0.15f, 0.75f, 1.f, 1.f)
					: FLinearColor(0.22f, 0.27f, 0.32f, 1.f);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform()),
				HexPoints,
				ESlateDrawEffect::None,
				CellColor,
				true,
				bHasSlot ? 2.f : 1.f);

			if (!bHasSlot)
			{
				continue;
			}

			const FVector2D MarkerSize(6.f, 6.f);
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 2,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(MarkerSize.X, MarkerSize.Y),
					FSlateLayoutTransform(FVector2f(CellCenter.X - MarkerSize.X * 0.5f, CellCenter.Y - MarkerSize.Y * 0.5f))),
				WhiteBrush,
				ESlateDrawEffect::None,
				CellColor);

			const FString Label = MakeSlotLabel(*CellEntries);
			const FSlateFontInfo Font = FCoreStyle::Get().GetFontStyle("SmallFont");
			const FVector2D TextSize(CellDistance * 1.5f, 24.f);
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 3,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(TextSize.X, TextSize.Y),
					FSlateLayoutTransform(FVector2f(CellCenter.X - TextSize.X * 0.5f + CellDistance * 0.5f, CellCenter.Y - TextSize.Y * 0.5f))),
				FText::FromString(Label),
				Font,
				ESlateDrawEffect::None,
				FLinearColor::White);
		}
	}

	for (const TPair<FCubeCoord, TArray<FSlotEntry>>& Pair : EntriesByCoord)
	{
		const FCubeCoord& Coord = Pair.Key;
		if (FMath::Max3(FMath::Abs(Coord.Q), FMath::Abs(Coord.R), FMath::Abs(Coord.S)) <= GridRadius)
		{
			continue;
		}

		const FVector2D CellCenter = GetCellCenter(Coord, MapCenter, CellDistance);
		TArray<FVector2D> HexPoints;
		MakeHexagon(CellCenter, HexRadius, HexPoints);
		const FLinearColor OutOfRangeColor(1.f, 0.2f, 0.1f, 1.f);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform()),
			HexPoints,
			ESlateDrawEffect::None,
			OutOfRangeColor,
			true,
			2.f);

		const FVector2D MarkerSize(6.f, 6.f);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(MarkerSize.X, MarkerSize.Y),
				FSlateLayoutTransform(FVector2f(CellCenter.X - MarkerSize.X * 0.5f, CellCenter.Y - MarkerSize.Y * 0.5f))),
			WhiteBrush,
			ESlateDrawEffect::None,
			OutOfRangeColor);

		const FString Label = MakeSlotLabel(Pair.Value);
		const FSlateFontInfo Font = FCoreStyle::Get().GetFontStyle("SmallFont");
		const FVector2D TextSize(CellDistance * 1.5f, 24.f);
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 3,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(TextSize.X, TextSize.Y),
				FSlateLayoutTransform(FVector2f(CellCenter.X - TextSize.X * 0.5f + CellDistance * 0.5f, CellCenter.Y - TextSize.Y * 0.5f))),
			FText::FromString(Label),
			Font,
			ESlateDrawEffect::None,
			FLinearColor::White);
	}

	return LayerId + 4;
}

FText SRoomRoleAssignmentRuleSlotMap::GetSummaryText() const
{
	TArray<FSlotEntry> Entries;
	int32 MaxRadius = 0;
	int32 RequiredDistance = INDEX_NONE;
	ReadSlotEntries(Entries, MaxRadius, RequiredDistance);

	const FString RequiredDistanceText = RequiredDistance == INDEX_NONE ? TEXT("?") : LexToString(RequiredDistance);
	const int32 GridRadius = RequiredDistance == INDEX_NONE ? 0 : FMath::Max(0, RequiredDistance);
	return FText::FromString(FString::Printf(
		TEXT("슬롯 %d개 · 표시 반경 %d · RequiredSpaceRangeDistance %s · 숫자: 배열 인덱스 / 타입 약어"),
		Entries.Num(), GridRadius, *RequiredDistanceText));
}

FText SRoomRoleAssignmentRuleSlotMap::GetWarningText() const
{
	TArray<FSlotEntry> Entries;
	int32 MaxRadius = 0;
	int32 RequiredDistance = INDEX_NONE;
	ReadSlotEntries(Entries, MaxRadius, RequiredDistance);

	TArray<FString> Warnings;
	for (const FSlotEntry& Entry : Entries)
	{
		if (!Entry.bValidCubeCoord)
		{
			Warnings.Add(FString::Printf(TEXT("#%d (%d, %d, %d): Q + R + S가 0이 아닙니다."), Entry.Index, Entry.Coord.Q, Entry.Coord.R, Entry.Coord.S));
		}
		if (!Entry.bWithinRequiredDistance && RequiredDistance != INDEX_NONE)
		{
			Warnings.Add(FString::Printf(TEXT("#%d (%d, %d, %d): 요구 거리 %d를 초과합니다."), Entry.Index, Entry.Coord.Q, Entry.Coord.R, Entry.Coord.S, RequiredDistance));
		}
		if (Entry.bDuplicateCoord)
		{
			Warnings.Add(FString::Printf(TEXT("#%d (%d, %d, %d): 다른 슬롯과 좌표가 중복됩니다."), Entry.Index, Entry.Coord.Q, Entry.Coord.R, Entry.Coord.S));
		}
	}

	if (RequiredDistance != INDEX_NONE && RequiredDistance < 0)
	{
		Warnings.Add(FString::Printf(TEXT("RequiredSpaceRangeDistance가 음수입니다: %d"), RequiredDistance));
	}

	if (Warnings.IsEmpty())
	{
		return FText::GetEmpty();
	}

	FString WarningText = FString::Printf(TEXT("⚠ 경고 %d건\n"), Warnings.Num());
	for (const FString& Warning : Warnings)
	{
		WarningText += FString::Printf(TEXT("• %s\n"), *Warning);
	}
	return FText::FromString(WarningText);
}

void SRoomRoleAssignmentRuleSlotMap::ReadSlotEntries(TArray<FSlotEntry>& OutEntries, int32& OutMaxRadius, int32& OutRequiredDistance) const
{
	OutEntries.Reset();
	OutMaxRadius = 0;
	OutRequiredDistance = INDEX_NONE;

	if (RequiredDistanceHandle.IsValid())
	{
		RequiredDistanceHandle->GetValue(OutRequiredDistance);
	}

	if (!CoordSlotsHandle.IsValid())
	{
		return;
	}

	uint32 NumSlots = 0;
	CoordSlotsHandle->GetNumChildren(NumSlots);
	OutEntries.Reserve(NumSlots);

	TMap<FCubeCoord, TArray<int32>> IndicesByCoord;
	for (uint32 Index = 0; Index < NumSlots; ++Index)
	{
		const TSharedPtr<IPropertyHandle> SlotHandle = CoordSlotsHandle->GetChildHandle(Index);
		if (!SlotHandle.IsValid())
		{
			continue;
		}

		FSlotEntry& Entry = OutEntries.AddDefaulted_GetRef();
		Entry.Index = static_cast<int32>(Index);

		const TSharedPtr<IPropertyHandle> CoordHandle = SlotHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRoomCoordSlot, SlotCoord));
		const TSharedPtr<IPropertyHandle> QHandle = CoordHandle.IsValid() ? CoordHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCubeCoord, Q)) : nullptr;
		const TSharedPtr<IPropertyHandle> RHandle = CoordHandle.IsValid() ? CoordHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCubeCoord, R)) : nullptr;
		const TSharedPtr<IPropertyHandle> SHandle = CoordHandle.IsValid() ? CoordHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCubeCoord, S)) : nullptr;
		int32 Q = 0;
		int32 R = 0;
		int32 S = 0;
		if (!QHandle.IsValid() || !RHandle.IsValid() || !SHandle.IsValid() ||
			QHandle->GetValue(Q) != FPropertyAccess::Success ||
			RHandle->GetValue(R) != FPropertyAccess::Success ||
			SHandle->GetValue(S) != FPropertyAccess::Success)
		{
			Entry.bValidCubeCoord = false;
		}
		Entry.Coord = FCubeCoord(Q, R, S);

		const TSharedPtr<IPropertyHandle> SlotTypeHandle = SlotHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRoomCoordSlot, SlotType));
		if (SlotTypeHandle.IsValid())
		{
			SlotTypeHandle->GetValueAsFormattedString(Entry.SlotType);
		}

		const TSharedPtr<IPropertyHandle> SpawnActorClassHandle = SlotHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRoomCoordSlot, SpawnActorClass));
		if (SpawnActorClassHandle.IsValid())
		{
			SpawnActorClassHandle->GetValueAsFormattedString(Entry.SpawnActorClass);
		}

		Entry.Distance = FMath::Max3(FMath::Abs(Entry.Coord.Q), FMath::Abs(Entry.Coord.R), FMath::Abs(Entry.Coord.S));
		Entry.bValidCubeCoord = Entry.bValidCubeCoord && (Entry.Coord.Q + Entry.Coord.R + Entry.Coord.S == 0);
		Entry.bWithinRequiredDistance = OutRequiredDistance == INDEX_NONE || Entry.Distance <= OutRequiredDistance;
		OutMaxRadius = FMath::Max(OutMaxRadius, Entry.Distance);

		if (Entry.bValidCubeCoord)
		{
			IndicesByCoord.FindOrAdd(Entry.Coord).Add(Entry.Index);
		}
	}

	for (FSlotEntry& Entry : OutEntries)
	{
		if (const TArray<int32>* Indices = IndicesByCoord.Find(Entry.Coord))
		{
			Entry.bDuplicateCoord = Indices->Num() > 1;
		}
	}
}

FString SRoomRoleAssignmentRuleSlotMap::MakeSlotLabel(const TArray<FSlotEntry>& Entries)
{
	FString Label;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Index > 0)
		{
			Label += TEXT("/");
		}

		FString Type = Entries[Index].SlotType;
		Type.ReplaceInline(TEXT("ERoomCoordSlotType::"), TEXT(""));
		if (Type.Len() > 1)
		{
			Type = Type.Left(1);
		}
		Label += FString::Printf(TEXT("%d:%s"), Entries[Index].Index, *Type);
	}
	return Label;
}

FVector2D SRoomRoleAssignmentRuleSlotMap::GetCellCenter(const FCubeCoord& Coord, const FVector2D& MapCenter, float CellDistance)
{
	return MapCenter + FVector2D(
		CellDistance * (static_cast<float>(Coord.Q) + static_cast<float>(Coord.R) * 0.5f),
		CellDistance * static_cast<float>(Coord.R) * 0.8660254038f);
}

void SRoomRoleAssignmentRuleSlotMap::MakeHexagon(const FVector2D& Center, float Radius, TArray<FVector2D>& OutPoints)
{
	OutPoints.Reset(7);
	for (int32 Index = 0; Index <= 6; ++Index)
	{
		const float Angle = RoomRoleAssignmentRuleSlotMap::Pi / 6.f + static_cast<float>(Index) * RoomRoleAssignmentRuleSlotMap::Pi / 3.f;
		OutPoints.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
	}
}
