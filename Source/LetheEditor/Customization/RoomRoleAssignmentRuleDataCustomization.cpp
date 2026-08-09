// Copyright JETBLU, Inc. All Rights Reserved.

#include "RoomRoleAssignmentRuleDataCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Lethe/Data/Stage/RoomRoleAssignmentRuleData.h"
#include "PropertyHandle.h"
#include "RoomRoleAssignmentRuleSlotMap.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IDetailCustomization> FRoomRoleAssignmentRuleDataCustomization::MakeInstance()
{
	return MakeShared<FRoomRoleAssignmentRuleDataCustomization>();
}

void FRoomRoleAssignmentRuleDataCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Room Role Assignment");

	const TSharedRef<IPropertyHandle> RoomRoleHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(URoomRoleAssignmentRuleData, RoomRole));
	const TSharedRef<IPropertyHandle> BFSTypeHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(URoomRoleAssignmentRuleData, RequiredSpaceRangeBFSType));
	const TSharedRef<IPropertyHandle> DistanceHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(URoomRoleAssignmentRuleData, RequiredSpaceRangeDistance));
	const TSharedRef<IPropertyHandle> CoordSlotsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(URoomRoleAssignmentRuleData, CoordSlots));

	Category.AddProperty(RoomRoleHandle);
	Category.AddProperty(BFSTypeHandle);
	Category.AddProperty(DistanceHandle);

	TSharedPtr<SRoomRoleAssignmentRuleSlotMap> SlotMapWidget;
	SAssignNew(SlotMapWidget, SRoomRoleAssignmentRuleSlotMap)
		.CoordSlotsHandle(CoordSlotsHandle)
		.RequiredDistanceHandle(DistanceHandle);

	Category.AddCustomRow(NSLOCTEXT("RoomRoleAssignmentRuleDataCustomization", "SlotMapFilter", "Slot Map"))
	.WholeRowContent()
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 4.f, 0.f, 4.f)
		[
			SNew(STextBlock)
			.Text_Lambda([SlotMapWidget]()
			{
				return SlotMapWidget.IsValid() ? SlotMapWidget->GetSummaryText() : FText::GetEmpty();
			})
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.MinDesiredHeight(180.f)
			.MaxDesiredHeight(520.f)
			[
				SlotMapWidget.ToSharedRef()
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 4.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text_Lambda([SlotMapWidget]()
			{
				return SlotMapWidget.IsValid() ? SlotMapWidget->GetWarningText() : FText::GetEmpty();
			})
			.ColorAndOpacity(FLinearColor(1.f, 0.35f, 0.15f))
			.AutoWrapText(true)
		]
	];

	Category.AddProperty(CoordSlotsHandle);
}
