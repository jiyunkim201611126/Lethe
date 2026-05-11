// Copyright JETBLU, Inc. All Rights Reserved.

#include "CubeCoordCustomization.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Lethe/Data/Stage/CubeCoord.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FCubeCoordCustomization"

namespace CubeCoordCustomization
{
	const FName HexDirectionButtonsMetaName(TEXT("HexDirectionButtons"));
	constexpr float DirectionButtonSize = 24.f;
}

TSharedRef<IPropertyTypeCustomization> FCubeCoordCustomization::MakeInstance()
{
	return MakeShared<FCubeCoordCustomization>();
}

void FCubeCoordCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructHandle = InStructPropertyHandle;
	QHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCubeCoord, Q));
	RHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCubeCoord, R));
	SHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCubeCoord, S));
	bShowDirectionButtons = StructHandle->HasMetaData(CubeCoordCustomization::HexDirectionButtonsMetaName);

	HeaderRow
	.NameContent()
	[
		StructHandle->CreatePropertyNameWidget()
	];

	if (!bShowDirectionButtons)
	{
		return;
	}

	HeaderRow
	.ValueContent()
	.MinDesiredWidth(300.f)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			CreateLabeledValueWidget(LOCTEXT("QLabel", "Q"), QHandle)
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			CreateLabeledValueWidget(LOCTEXT("RLabel", "R"), RHandle)
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.Padding(0.f, 0.f, 10.f, 0.f)
		[
			CreateLabeledValueWidget(LOCTEXT("SLabel", "S"), SHandle)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			CreateDirectionButtonPanel()
		]
	];
}

void FCubeCoordCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	if (bShowDirectionButtons)
	{
		return;
	}

	uint32 NumChildren = 0;
	InStructPropertyHandle->GetNumChildren(NumChildren);

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		if (TSharedPtr<IPropertyHandle> ChildHandle = InStructPropertyHandle->GetChildHandle(ChildIndex))
		{
			ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
		}
	}
}

FReply FCubeCoordCustomization::OnDirectionButtonClicked(const int32 DirectionIndex) const
{
	if (!CanEditDirection())
	{
		return FReply::Handled();
	}

	int32 CurrentQ = 0;
	int32 CurrentR = 0;
	int32 CurrentS = 0;
	if (QHandle->GetValue(CurrentQ) != FPropertyAccess::Success ||
		RHandle->GetValue(CurrentR) != FPropertyAccess::Success ||
		SHandle->GetValue(CurrentS) != FPropertyAccess::Success)
	{
		return FReply::Handled();
	}

	const FCubeCoord DirectionOffset = FCubeCoord::GetDirection(DirectionIndex);
	const FScopedTransaction Transaction(LOCTEXT("MoveCubeCoordTransaction", "Move Cube Coord"));
	QHandle->SetValue(CurrentQ + DirectionOffset.Q);
	RHandle->SetValue(CurrentR + DirectionOffset.R);
	SHandle->SetValue(CurrentS + DirectionOffset.S);

	return FReply::Handled();
}

FReply FCubeCoordCustomization::OnResetButtonClicked() const
{
	if (!CanEditDirection())
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("ResetCubeCoordTransaction", "Reset Cube Coord"));
	QHandle->SetValue(0);
	RHandle->SetValue(0);
	SHandle->SetValue(0);

	return FReply::Handled();
}

bool FCubeCoordCustomization::CanEditDirection() const
{
	return QHandle.IsValid() && RHandle.IsValid() && SHandle.IsValid() &&
		!QHandle->IsEditConst() && !RHandle->IsEditConst() && !SHandle->IsEditConst();
}

TSharedRef<SWidget> FCubeCoordCustomization::CreateLabeledValueWidget(const FText& Label, const TSharedPtr<IPropertyHandle>& PropertyHandle) const
{
	return SNew(SHorizontalBox)

	+ SHorizontalBox::Slot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	.Padding(0.f, 0.f, 3.f, 0.f)
	[
		SNew(STextBlock)
		.Text(Label)
	]

	+ SHorizontalBox::Slot()
	.FillWidth(1.f)
	[
		PropertyHandle.IsValid()
			? PropertyHandle->CreatePropertyValueWidget()
			: SNullWidget::NullWidget
	];
}

TSharedRef<SWidget> FCubeCoordCustomization::CreateDirectionButton(const int32 DirectionIndex, const FText& Tooltip) const
{
	return SNew(SBox)
	.WidthOverride(CubeCoordCustomization::DirectionButtonSize)
	.HeightOverride(CubeCoordCustomization::DirectionButtonSize)
	[
		SNew(SButton)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.ContentPadding(FMargin(0.f))
		.ToolTipText(Tooltip)
		.IsEnabled(this, &FCubeCoordCustomization::CanEditDirection)
		.OnClicked(this, &FCubeCoordCustomization::OnDirectionButtonClicked, DirectionIndex)
		[
			SNullWidget::NullWidget
		]
	];
}

TSharedRef<SWidget> FCubeCoordCustomization::CreateResetButton() const
{
	return SNew(SBox)
	.WidthOverride(CubeCoordCustomization::DirectionButtonSize)
	.HeightOverride(CubeCoordCustomization::DirectionButtonSize)
	[
		SNew(SButton)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.ContentPadding(FMargin(0.f))
		.ToolTipText(LOCTEXT("ResetTooltip", "Reset: Q 0, R 0, S 0"))
		.IsEnabled(this, &FCubeCoordCustomization::CanEditDirection)
		.OnClicked(this, &FCubeCoordCustomization::OnResetButtonClicked)
		[
			SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.Text(LOCTEXT("ResetLabel", "R"))
		]
	];
}

TSharedRef<SWidget> FCubeCoordCustomization::CreateDirectionButtonPanel() const
{
	return SNew(SGridPanel)
	.FillColumn(0, 1.f)
	.FillColumn(1, 1.f)
	.FillColumn(2, 1.f)
	.FillColumn(3, 1.f)
	.FillRow(0, 1.f)
	.FillRow(1, 1.f)
	.FillRow(2, 1.f)

	+ SGridPanel::Slot(1, 0)
	.Padding(1.f)
	[
		CreateDirectionButton(0, LOCTEXT("LeftTopTooltip", "Left Top: Q +0, R -1, S +1"))
	]

	+ SGridPanel::Slot(2, 0)
	.Padding(1.f)
	[
		CreateDirectionButton(5, LOCTEXT("RightTopTooltip", "Right Top: Q +1, R -1, S +0"))
	]

	+ SGridPanel::Slot(0, 1)
	.Padding(1.f)
	[
		CreateDirectionButton(1, LOCTEXT("LeftTooltip", "Left: Q -1, R +0, S +1"))
	]

	+ SGridPanel::Slot(1, 1)
	.ColumnSpan(2)
	.Padding(1.f)
	[
		CreateResetButton()
	]

	+ SGridPanel::Slot(3, 1)
	.Padding(1.f)
	[
		CreateDirectionButton(4, LOCTEXT("RightTooltip", "Right: Q +1, R +0, S -1"))
	]

	+ SGridPanel::Slot(1, 2)
	.Padding(1.f)
	[
		CreateDirectionButton(2, LOCTEXT("LeftBottomTooltip", "Left Bottom: Q -1, R +1, S +0"))
	]

	+ SGridPanel::Slot(2, 2)
	.Padding(1.f)
	[
		CreateDirectionButton(3, LOCTEXT("RightBottomTooltip", "Right Bottom: Q +0, R +1, S -1"))
	];
}

#undef LOCTEXT_NAMESPACE
