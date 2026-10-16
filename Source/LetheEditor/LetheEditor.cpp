#include "LetheEditor.h"

#include "Customization/CubeCoordCustomization.h"
#include "Customization/RoomRoleAssignmentRuleDataCustomization.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FLetheEditorModule"

void FLetheEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(
		"CubeCoord",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCubeCoordCustomization::MakeInstance));
	PropertyEditorModule.RegisterCustomClassLayout(
		"RoomRoleAssignmentRuleData",
		FOnGetDetailCustomizationInstance::CreateStatic(&FRoomRoleAssignmentRuleDataCustomization::MakeInstance));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FLetheEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.UnregisterCustomPropertyTypeLayout("CubeCoord");
		PropertyEditorModule.UnregisterCustomClassLayout("RoomRoleAssignmentRuleData");
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLetheEditorModule, LetheEditor)
