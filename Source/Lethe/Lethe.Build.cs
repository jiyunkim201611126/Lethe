// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Lethe : ModuleRules
{
	public Lethe(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			
			"UMG",
			"Slate",
			"SlateCore",
			"CommonUI",
			"CommonInput",
			
			"ModularGameplay",
			
			"GameplayAbilities",
			"StructUtils",
			
			"AIModule",
			"PCG",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"GameplayTags",
			"GameplayTasks",
			"Niagara",

			"StateTreeModule",
			"GameplayStateTreeModule",

			"AssetRegistry",
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
