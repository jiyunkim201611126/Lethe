// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class FCubeCoordCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	FReply OnDirectionButtonClicked(int32 DirectionIndex) const;
	FReply OnResetButtonClicked() const;
	bool CanEditDirection() const;
	TSharedRef<SWidget> CreateLabeledValueWidget(const FText& Label, const TSharedPtr<IPropertyHandle>& PropertyHandle) const;
	TSharedRef<SWidget> CreateDirectionButton(int32 DirectionIndex, const FText& Tooltip) const;
	TSharedRef<SWidget> CreateResetButton() const;
	TSharedRef<SWidget> CreateDirectionButtonPanel() const;

private:
	TSharedPtr<IPropertyHandle> StructHandle;
	TSharedPtr<IPropertyHandle> QHandle;
	TSharedPtr<IPropertyHandle> RHandle;
	TSharedPtr<IPropertyHandle> SHandle;
	bool bShowDirectionButtons = false;
};
