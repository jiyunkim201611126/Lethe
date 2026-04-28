// Copyright JETBLU, Inc. All Rights Reserved.

#include "BGMTransitionPointCustomization.h"

#include "DetailWidgetRow.h"
#include "Lethe/Manager/FX/BGMThemeDataAsset.h"
#include "PropertyHandle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IPropertyTypeCustomization> FBGMTransitionPointCustomization::MakeInstance()
{
	return MakeShared<FBGMTransitionPointCustomization>();
}

void FBGMTransitionPointCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	const TSharedPtr<IPropertyHandle> TrackAHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBGMTransitionPoint, TrackA));
	const TSharedPtr<IPropertyHandle> TrackATimeHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBGMTransitionPoint, TrackATime));
	const TSharedPtr<IPropertyHandle> TrackBHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBGMTransitionPoint, TrackB));
	const TSharedPtr<IPropertyHandle> TrackBTimeHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FBGMTransitionPoint, TrackBTime));

	const auto CreateValueWidget = [](const TSharedPtr<IPropertyHandle>& PropertyHandle) -> TSharedRef<SWidget>
	{
		return PropertyHandle.IsValid()
			? PropertyHandle->CreatePropertyValueWidget()
			: SNullWidget::NullWidget;
	};

	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(560.f)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.Padding(0.f, 0.f, 4.f, 0.f)
		[
			CreateValueWidget(TrackAHandle)
		]

		+ SHorizontalBox::Slot()
		.FillWidth(0.6f)
		.Padding(0.f, 0.f, 8.f, 0.f)
		[
			CreateValueWidget(TrackATimeHandle)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, 8.f, 0.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("<->")))
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.Padding(0.f, 0.f, 4.f, 0.f)
		[
			CreateValueWidget(TrackBHandle)
		]

		+ SHorizontalBox::Slot()
		.FillWidth(0.6f)
		[
			CreateValueWidget(TrackBTimeHandle)
		]
	];
}

void FBGMTransitionPointCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}
