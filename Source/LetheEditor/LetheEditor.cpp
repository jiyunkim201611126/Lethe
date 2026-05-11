#include "LetheEditor.h"

#include "Customization/BGMTransitionPointCustomization.h"
#include "Customization/CubeCoordCustomization.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FLetheEditorModule"

void FLetheEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(
		"BGMTransitionPoint",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FBGMTransitionPointCustomization::MakeInstance));
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(
		"CubeCoord",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCubeCoordCustomization::MakeInstance));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FLetheEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.UnregisterCustomPropertyTypeLayout("BGMTransitionPoint");
		PropertyEditorModule.UnregisterCustomPropertyTypeLayout("CubeCoord");
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLetheEditorModule, LetheEditor)
